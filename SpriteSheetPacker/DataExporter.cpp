#include "DataExporter.h"

#include "PListSerializer.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

QMap<QString, SpriteFrameInfo> exportedFrames(const QMap<QString, SpriteFrameInfo>& frames,
                                               bool trimSpriteNames,
                                               bool prependSmartFolderName)
{
    QMap<QString, SpriteFrameInfo> result;
    for (auto it = frames.cbegin(); it != frames.cend(); ++it) {
        result.insert(exportedName(it.key(), trimSpriteNames, prependSmartFolderName), it.value());
    }
    return result;
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

QByteArray indentedJson(const QJsonObject& object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Indented);
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
    return {"cocos2d", "cocos2d-old", "godot-anim", "godot-parts", "json", "phaser", "pixijs"};
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
    const QMap<QString, SpriteFrameInfo> frames = exportedFrames(
        spriteFrames, trimSpriteNames, prependSmartFolderName);

    if (format == "json") {
        result->data = exportSimpleJson(frames);
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
