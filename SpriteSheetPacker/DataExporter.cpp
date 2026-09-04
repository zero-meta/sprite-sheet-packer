#include "DataExporter.h"

#include "PListSerializer.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QVariantList>
#include <QVariantMap>

namespace {

QString exportedName(QString name, bool trimSpriteNames, bool prependSmartFolderName)
{
    name = QDir::fromNativeSeparators(name);
    if (!prependSmartFolderName) {
        const qsizetype separator = name.indexOf('/');
        if (separator >= 0) {
            name.remove(0, separator + 1);
        }
    }
    if (trimSpriteNames) {
        const QFileInfo info(name);
        name = QDir::fromNativeSeparators(info.path() + QDir::separator() + info.baseName());
    }
    return name;
}

bool exportedFrames(const QMap<QString, SpriteFrameInfo>& frames,
                    bool trimSpriteNames,
                    bool prependSmartFolderName,
                    QMap<QString, SpriteFrameInfo>* result,
                    QString* errorMessage)
{
    result->clear();
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        const QString name = exportedName(it.key(), trimSpriteNames, prependSmartFolderName);
        if (result->contains(name)) {
            if (errorMessage) {
                *errorMessage = QString("Multiple source images produce the frame name '%1'.")
                    .arg(name);
            }
            return false;
        }
        result->insert(name, it.value());
    }
    return true;
}

QJsonObject rectJson(const QRect& rect, bool shortSizeNames = false)
{
    return {
        {"x", rect.x()},
        {"y", rect.y()},
        {shortSizeNames ? "w" : "width", rect.width()},
        {shortSizeNames ? "h" : "height", rect.height()}
    };
}

QJsonObject sizeJson(const QSize& size, bool shortNames = false)
{
    return {
        {shortNames ? "w" : "width", size.width()},
        {shortNames ? "h" : "height", size.height()}
    };
}

QJsonArray outlineJson(const QVector<QPoint>& outline)
{
    QJsonArray result;
    for (const QPoint& point : outline) {
        result.append(point.x());
        result.append(point.y());
    }
    return result;
}

QByteArray indentedJson(const QJsonObject& object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Indented);
}

QString coronaFileName(const QString& filePath)
{
    return QDir::fromNativeSeparators(filePath).section('/', -1);
}

QString coronaFrameName(QString key, bool keepPath)
{
    if (!keepPath) return coronaFileName(key);

    key = QDir::fromNativeSeparators(key);
    while (key.startsWith("./")) key.remove(0, 2);
    key.remove(QRegularExpression("\\.[^./]+$"));
    return key;
}

bool exportCorona(const QMap<QString, SpriteFrameInfo>& frames,
                  const QSize& textureSize,
                  bool keepPath,
                  QByteArray* output,
                  QString* errorMessage)
{
    QJsonArray jsonFrames;
    QJsonObject frameIndex;
    QSet<QString> frameNames;
    int index = 1;

    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        const SpriteFrameInfo& frame = it.value();
        const QString name = coronaFrameName(it.key(), keepPath);
        if (frameNames.contains(name)) {
            if (errorMessage) {
                *errorMessage = QString("Multiple source images produce the Corona frame name '%1'.")
                    .arg(name);
            }
            return false;
        }
        frameNames.insert(name);

        QJsonObject value{
            {"x", frame.frame.x()},
            {"y", frame.frame.y()},
            {"width", frame.rotated ? frame.frame.height() : frame.frame.width()},
            {"height", frame.rotated ? frame.frame.width() : frame.frame.height()},
            {"name", name}
        };
        if (frame.rotated) value.insert("rotated", true);
        if (!frame.outline.isEmpty()) value.insert("outline", outlineJson(frame.outline));
        jsonFrames.append(value);
        frameIndex.insert(name, index++);
    }

    QJsonObject sheet{
        {"sheetContentWidth", textureSize.width()},
        {"sheetContentHeight", textureSize.height()},
        {"frames", jsonFrames}
    };
    *output = QJsonDocument(QJsonObject{
        {"sheet", sheet},
        {"frameIndex", frameIndex}
    }).toJson(QJsonDocument::Compact);
    if (output->endsWith('\n')) output->chop(1);
    return true;
}

QByteArray exportSimpleJson(const QMap<QString, SpriteFrameInfo>& frames)
{
    QJsonObject jsonFrames;
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        const SpriteFrameInfo& frame = it.value();
        jsonFrames.insert(it.key(), QJsonObject{
            {"frame", rectJson(frame.frame)},
            {"sourceSize", sizeJson(frame.sourceSize)},
            {"rotated", frame.rotated}
        });
    }
    return indentedJson(jsonFrames);
}

QJsonObject atlasJsonFrame(const SpriteFrameInfo& frame)
{
    const bool trimmed = frame.sourceSize.width() != frame.sourceColorRect.width()
                      || frame.sourceSize.height() != frame.sourceColorRect.height();
    return {
        {"frame", rectJson(frame.frame, true)},
        {"rotated", frame.rotated},
        {"spriteSourceSize", rectJson(frame.sourceColorRect, true)},
        {"sourceSize", sizeJson(frame.sourceSize, true)},
        {"trimmed", trimmed}
    };
}

QByteArray exportPixi(const QString& imageFilePath, const QMap<QString, SpriteFrameInfo>& frames)
{
    QJsonObject jsonFrames;
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        jsonFrames.insert(it.key(), atlasJsonFrame(it.value()));
    }
    return indentedJson({
        {"frames", jsonFrames},
        {"meta", QJsonObject{{"image", QFileInfo(imageFilePath).fileName()}}}
    });
}

QByteArray exportPhaser(const QString& imageFilePath, const QMap<QString, SpriteFrameInfo>& frames)
{
    QJsonArray jsonFrames;
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        QJsonObject frame = atlasJsonFrame(it.value());
        QString name = it.key();
        if (name.startsWith("./")) {
            name.remove(0, 2);
        }
        frame.insert("filename", name);
        jsonFrames.append(frame);
    }
    return indentedJson({
        {"frames", jsonFrames},
        {"meta", QJsonObject{{"image", QFileInfo(imageFilePath).fileName()}}}
    });
}

QVariantMap exportCocos(const QString& imageFilePath,
                        const QMap<QString, SpriteFrameInfo>& frames,
                        const QSize& textureSize,
                        bool legacy)
{
    QVariantMap metadata;
    metadata["format"] = legacy ? 2 : 3;
    metadata["textureFileName"] = QFileInfo(imageFilePath).fileName();
    if (!legacy) {
        metadata["size"] = QString("{%1,%2}").arg(textureSize.width()).arg(textureSize.height());
    }

    QVariantMap plistFrames;
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        const SpriteFrameInfo& frame = it.value();
        QVariantMap value;
        if (legacy) {
            value["frame"] = QString("{{%1,%2},{%3,%4}}")
                .arg(frame.frame.x()).arg(frame.frame.y())
                .arg(frame.frame.width()).arg(frame.frame.height());
            value["offset"] = QString("{%1,%2}").arg(frame.offset.x()).arg(frame.offset.y());
            value["sourceSize"] = QString("{%1,%2}")
                .arg(frame.sourceSize.width()).arg(frame.sourceSize.height());
            value["rotated"] = frame.rotated;
        } else {
            value["aliases"] = QVariantList();
            value["spriteSize"] = QString("{%1,%2}")
                .arg(frame.frame.width()).arg(frame.frame.height());
            value["spriteOffset"] = QString("{%1,%2}").arg(frame.offset.x()).arg(frame.offset.y());
            value["spriteSourceSize"] = QString("{%1,%2}")
                .arg(frame.sourceSize.width()).arg(frame.sourceSize.height());
            value["textureRect"] = QString("{{%1,%2},{%3,%4}}")
                .arg(frame.frame.x()).arg(frame.frame.y())
                .arg(frame.frame.width()).arg(frame.frame.height());
            value["textureRotated"] = frame.rotated;

            QStringList triangles;
            for (unsigned short index : frame.triangles.indices) {
                triangles.append(QString::number(index));
            }
            QStringList vertices;
            QStringList verticesUv;
            for (const QPoint& vertex : frame.triangles.verts) {
                vertices.append(QString::number(vertex.x() + frame.offset.x()));
                vertices.append(QString::number(vertex.y() + frame.offset.y()));
                verticesUv.append(QString::number(frame.frame.x() + vertex.x()));
                verticesUv.append(QString::number(frame.frame.y() + vertex.y()));
            }
            if (!triangles.isEmpty()) value["triangles"] = triangles.join(' ');
            if (!vertices.isEmpty()) value["vertices"] = vertices.join(' ');
            if (!verticesUv.isEmpty()) value["verticesUV"] = verticesUv.join(' ');
        }
        plistFrames[it.key()] = value;
    }

    return {{"frames", plistFrames}, {"metadata", metadata}};
}

QString godotEscape(QString value)
{
    value.replace('\\', "\\\\");
    value.replace('"', "\\\"");
    return value;
}

QByteArray exportGodotParts(const QString& imageFilePath, const QMap<QString, SpriteFrameInfo>& frames)
{
    QString contents;
    QTextStream out(&contents);
    out << "[gd_scene load_steps=" << frames.size() + 2 << " format=2]\n\n";
    out << "[ext_resource path=\"res://" << godotEscape(QFileInfo(imageFilePath).fileName())
        << "\" type=\"Texture\" id=1]\n\n";

    int id = 1;
    for (const SpriteFrameInfo& frame : frames) {
        out << "[sub_resource type=\"AtlasTexture\" id=" << id++ << "]\n";
        out << "atlas = ExtResource( 1 )\n";
        out << "region = Rect2( " << frame.frame.x() << ", " << frame.frame.y() << ", "
            << frame.frame.width() << ", " << frame.frame.height() << " )\n";
        out << "margin = Rect2( " << frame.sourceColorRect.x() << ", " << frame.sourceColorRect.y() << ", "
            << frame.sourceSize.width() - frame.frame.width() << ", "
            << frame.sourceSize.height() - frame.frame.height() << " )\n\n";
    }

    out << "[node name=\"" << godotEscape(QFileInfo(imageFilePath).baseName()) << "\" type=\"Sprite\"]\n\n";
    id = 1;
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        out << "[node name=\"" << godotEscape(QFileInfo(it.key()).baseName())
            << "\" type=\"Sprite\" parent=\".\"]\n";
        out << "texture = SubResource(" << id++ << ")\n\n";
    }
    return contents.toUtf8();
}

QByteArray exportGodotAnimations(const QString& imageFilePath, const QMap<QString, SpriteFrameInfo>& frames)
{
    QString contents;
    QTextStream out(&contents);
    out << "[gd_scene load_steps=" << frames.size() + 3 << " format=2]\n\n";
    out << "[ext_resource path=\"res://" << godotEscape(QFileInfo(imageFilePath).fileName())
        << "\" type=\"Texture\" id=1]\n\n";

    QMap<QString, QStringList> animations;
    int id = 1;
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        const SpriteFrameInfo& frame = it.value();
        out << "[sub_resource type=\"AtlasTexture\" id=" << id << "]\n";
        out << "atlas = ExtResource( 1 )\n";
        out << "region = Rect2( " << frame.frame.x() << ", " << frame.frame.y() << ", "
            << frame.frame.width() << ", " << frame.frame.height() << " )\n";
        out << "margin = Rect2( " << frame.sourceColorRect.x() << ", " << frame.sourceColorRect.y() << ", "
            << frame.sourceSize.width() - frame.frame.width() << ", "
            << frame.sourceSize.height() - frame.frame.height() << " )\n\n";

        QString animation = QFileInfo(it.key()).path();
        animation = QFileInfo(animation).fileName();
        if (animation.isEmpty() || animation == ".") animation = "default";
        animations[animation].append(QString("SubResource( %1 )").arg(id));
        ++id;
    }

    out << "[sub_resource type=\"SpriteFrames\" id=" << frames.size() + 1 << "]\n";
    out << "animations = [ ";
    bool first = true;
    for (auto it = animations.cbegin(); it != animations.cend(); ++it) {
        if (!first) out << ", ";
        first = false;
        out << "{\n\"frames\": [ " << it.value().join(", ") << " ],\n"
            << "\"loop\": true,\n\"name\": \"" << godotEscape(it.key())
            << "\",\n\"speed\": 5.0\n}";
    }
    out << " ]\n\n";
    out << "[node name=\"AnimatedSprite\" type=\"AnimatedSprite\"]\n";
    out << "frames = SubResource( " << frames.size() + 1 << " )\nframe = 0\n\n";
    return contents.toUtf8();
}

} // namespace

QStringList DataExporter::supportedFormats()
{
    return {"cocos2d", "cocos2d-old", "corona", "corona2", "godot-anim", "godot-parts", "json", "phaser", "pixijs"};
}

bool DataExporter::exportData(const QString& format,
                              const QString& imageFilePath,
                              const QMap<QString, SpriteFrameInfo>& spriteFrames,
                              const QSize& textureSize,
                              bool trimSpriteNames,
                              bool prependSmartFolderName,
                              DataExportResult* result,
                              QString* errorMessage)
{
    if (!result) return false;
    QMap<QString, SpriteFrameInfo> frames;
    if (!exportedFrames(spriteFrames,
                        trimSpriteNames,
                        prependSmartFolderName,
                        &frames,
                        errorMessage)) {
        return false;
    }

    if (format == "json") {
        result->data = exportSimpleJson(frames);
        result->extension = "json";
    } else if (format == "corona" || format == "corona2") {
        if (!exportCorona(frames,
                          textureSize,
                          format == "corona2",
                          &result->data,
                          errorMessage)) {
            return false;
        }
        result->extension = "json";
    } else if (format == "pixijs") {
        result->data = exportPixi(imageFilePath, frames);
        result->extension = "json";
    } else if (format == "phaser") {
        result->data = exportPhaser(imageFilePath, frames);
        result->extension = "json";
    } else if (format == "cocos2d" || format == "cocos2d-old") {
        result->data = PListSerializer::toPList(
            exportCocos(imageFilePath, frames, textureSize, format == "cocos2d-old")).toUtf8();
        result->extension = "plist";
    } else if (format == "godot-parts") {
        result->data = exportGodotParts(imageFilePath, frames);
        result->extension = "tscn";
    } else if (format == "godot-anim") {
        result->data = exportGodotAnimations(imageFilePath, frames);
        result->extension = "tscn";
    } else {
        if (errorMessage) {
            *errorMessage = QString("Unsupported data format '%1'. Supported formats: %2")
                .arg(format, supportedFormats().join(", "));
        }
        return false;
    }
    return true;
}
