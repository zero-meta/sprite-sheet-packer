#include "PublishSpriteSheet.h"

#include "DataExporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageWriter>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtMath>

namespace {

QString imageExtension(ImageFormat format)
{
    switch (format) {
        case kWEBP: return ".webp";
        case kJPG: return ".jpg";
        default: return ".png";
    }
}

QByteArray writerFormat(ImageFormat format)
{
    switch (format) {
        case kWEBP: return "webp";
        case kJPG: return "jpeg";
        default: return "png";
    }
}

bool isUnitScale(float scale)
{
    return qAbs(scale - 1.0f) <= 0.000001f;
}

QString pageOutputPath(QString baseFilePath, qsizetype page, qsizetype pageCount)
{
    if (baseFilePath.contains("{n}")) {
        baseFilePath.replace("{n}", QString::number(page));
    } else if (baseFilePath.contains("{n1}")) {
        baseFilePath.replace("{n1}", QString::number(page + 1));
    } else if (pageCount > 1) {
        baseFilePath += "_" + QString::number(page);
    }
    return baseFilePath;
}

bool framesContainTransparency(const QImage& image,
                               const QMap<QString, SpriteFrameInfo>& frames)
{
    if (!image.hasAlphaChannel()) return false;

    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    for (const SpriteFrameInfo& frame : frames) {
        const QRect bounds = frame.frame.intersected(argb.rect());
        for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
            const QRgb* pixels = reinterpret_cast<const QRgb*>(argb.constScanLine(y));
            for (int x = bounds.left(); x <= bounds.right(); ++x) {
                if (qAlpha(pixels[x]) != 255) return true;
            }
        }
    }
    return false;
}

bool replaceWithFile(const QString& destinationPath, const QString& sourcePath)
{
    QFile source(sourcePath);
    QSaveFile destination(destinationPath);
    if (!source.open(QIODevice::ReadOnly) || !destination.open(QIODevice::WriteOnly)) {
        return false;
    }

    char buffer[64 * 1024];
    while (!source.atEnd()) {
        const qint64 bytesRead = source.read(buffer, sizeof(buffer));
        if (bytesRead <= 0 || destination.write(buffer, bytesRead) != bytesRead) {
            return false;
        }
    }
    return destination.commit();
}

void optimizeWithPngQuant(const QString& imageFilePath, const QString& quality)
{
    const QString executable = QStandardPaths::findExecutable("pngquant");
    if (executable.isEmpty()) {
        qDebug() << "pngquant is not installed; keeping the original PNG.";
        return;
    }

    const QFileInfo originalInfo(imageFilePath);
    QTemporaryFile temporary(originalInfo.absolutePath()
        + "/." + originalInfo.fileName() + ".pngquant-XXXXXX.png");
    if (!temporary.open()) {
        qDebug() << "Cannot create pngquant temporary file; keeping the original PNG.";
        return;
    }
    temporary.close();

    QProcess process;
    process.start(executable, {
        "--force",
        "--quality", quality,
        "--output", temporary.fileName(),
        "--",
        imageFilePath
    });
    if (!process.waitForStarted() || !process.waitForFinished(-1)
        || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug().noquote() << "pngquant did not produce an output; keeping the original PNG:"
                           << process.readAllStandardError().trimmed();
        return;
    }

    const qint64 originalSize = originalInfo.size();
    const qint64 optimizedSize = QFileInfo(temporary.fileName()).size();
    if (optimizedSize <= 0 || optimizedSize >= originalSize) {
        qDebug() << "pngquant output is not smaller; keeping the original PNG.";
        return;
    }
    if (!replaceWithFile(imageFilePath, temporary.fileName())) {
        qDebug() << "Cannot replace PNG with pngquant output; keeping the original PNG.";
        return;
    }
    qDebug() << "pngquant reduced" << imageFilePath << "from" << originalSize << "to" << optimizedSize << "bytes.";
}

} // namespace

PublishSpriteSheet::PublishSpriteSheet()
    : _imageFormat(kPNG)
    , _pixelFormat(kARGB8888)
    , _premultiplied(true)
    , _trimSpriteNames(true)
    , _prependSmartFolderName(true)
    , _pngCompression(9)
    , _webpQuality(80)
    , _jpgQuality(80)
    , _pngQuantEnabled(false)
    , _pngQuantQuality("80-95")
{
}

void PublishSpriteSheet::addSpriteSheet(const SpriteAtlas& atlas, const QString& fileName)
{
    _spriteAtlases.append(atlas);
    _fileNames.append(fileName);
}

bool PublishSpriteSheet::writeTexture(const QImage& image,
                                      const QString& imageFilePath,
                                      QString* errorMessage)
{
    QImageWriter writer(imageFilePath, writerFormat(_imageFormat));
    writer.setOptimizedWrite(true);
    if (_imageFormat == kPNG) {
        // Qt's PNG handler expects a 0..100 compression ratio, while
        // the CLI exposes the familiar PNG/zlib levels 0..9.
        writer.setCompression((_pngCompression * 100 + 4) / 9);
    } else if (_imageFormat == kWEBP) {
        writer.setQuality(_webpQuality);
    } else if (_imageFormat == kJPG) {
        writer.setQuality(_jpgQuality);
    }
    if (!writer.write(image)) {
        if (errorMessage) {
            *errorMessage = QString("Cannot write texture '%1': %2")
                .arg(imageFilePath, writer.errorString());
        }
        return false;
    }
    if (_imageFormat == kPNG && _pngQuantEnabled) {
        optimizeWithPngQuant(imageFilePath, _pngQuantQuality);
    }
    return true;
}

bool PublishSpriteSheet::publish(const QString& format, QString* errorMessage)
{
    if (_spriteAtlases.size() != _fileNames.size()) {
        if (errorMessage) *errorMessage = "Internal error: atlas and output counts differ.";
        return false;
    }

    qsizetype textureVariantSource = -1;
    if (!_textureScaleVariants.isEmpty()) {
        QSet<QString> suffixes;
        for (const TextureScaleVariant& variant : _textureScaleVariants) {
            if (!qIsFinite(variant.scale) || variant.scale <= 0.0f
                || isUnitScale(variant.scale)) {
                if (errorMessage) {
                    *errorMessage = "textureScaleVariants scale must be greater than 0 and different from 1.";
                }
                return false;
            }
            if (variant.suffix.isEmpty()
                || variant.suffix != variant.suffix.trimmed()
                || variant.suffix.contains('/')
                || variant.suffix.contains('\\')) {
                if (errorMessage) {
                    *errorMessage = "textureScaleVariants suffix must be non-empty and cannot contain whitespace at its ends or path separators.";
                }
                return false;
            }
            const QString suffixKey = variant.suffix.toCaseFolded();
            if (suffixes.contains(suffixKey)) {
                if (errorMessage) {
                    *errorMessage = QString("Duplicate textureScaleVariants suffix '%1'.")
                        .arg(variant.suffix);
                }
                return false;
            }
            suffixes.insert(suffixKey);
        }

        for (qsizetype i = 0; i < _spriteAtlases.size(); ++i) {
            if (isUnitScale(_spriteAtlases.at(i).scale())) {
                if (textureVariantSource >= 0) {
                    if (errorMessage) {
                        *errorMessage = "textureScaleVariants require exactly one scale=1 atlas output.";
                    }
                    return false;
                }
                textureVariantSource = i;
            }
        }
        if (textureVariantSource < 0) {
            if (errorMessage) {
                *errorMessage = "textureScaleVariants require exactly one scale=1 atlas output.";
            }
            return false;
        }

        QSet<QString> outputPaths;
        for (qsizetype i = 0; i < _spriteAtlases.size(); ++i) {
            const SpriteAtlas& atlas = _spriteAtlases.at(i);
            for (qsizetype page = 0; page < atlas.outputData().size(); ++page) {
                const QString outputPath = pageOutputPath(_fileNames.at(i),
                                                         page,
                                                         atlas.outputData().size());
                outputPaths.insert(QDir::cleanPath(outputPath + imageExtension(_imageFormat))
                                       .toCaseFolded());
            }
        }
        const SpriteAtlas& sourceAtlas = _spriteAtlases.at(textureVariantSource);
        for (qsizetype page = 0; page < sourceAtlas.outputData().size(); ++page) {
            const QString outputPath = pageOutputPath(_fileNames.at(textureVariantSource),
                                                     page,
                                                     sourceAtlas.outputData().size());
            for (const TextureScaleVariant& variant : _textureScaleVariants) {
                const QString variantPath = outputPath + variant.suffix
                                          + imageExtension(_imageFormat);
                const QString variantPathKey = QDir::cleanPath(variantPath).toCaseFolded();
                if (outputPaths.contains(variantPathKey)) {
                    if (errorMessage) {
                        *errorMessage = QString("Texture output path collision: '%1'.")
                            .arg(variantPath);
                    }
                    return false;
                }
                outputPaths.insert(variantPathKey);
            }
        }
    }

    for (qsizetype i = 0; i < _spriteAtlases.size(); ++i) {
        const SpriteAtlas& atlas = _spriteAtlases.at(i);
        const QString& baseFilePath = _fileNames.at(i);

        for (qsizetype page = 0; page < atlas.outputData().size(); ++page) {
            const SpriteAtlas::OutputData& output = atlas.outputData().at(page);
            const QString outputFilePath = pageOutputPath(baseFilePath,
                                                          page,
                                                          atlas.outputData().size());

            const QString imageFilePath = outputFilePath + imageExtension(_imageFormat);
            QImage image = convertImage(output._atlasImage, _pixelFormat, _premultiplied);
            if (_imageFormat == kJPG) {
                if (framesContainTransparency(image, output._spriteFrames)) {
                    if (errorMessage) {
                        *errorMessage = QString(
                            "Cannot write JPEG '%1': sprite pixels contain transparency; use PNG or WebP.")
                            .arg(imageFilePath);
                    }
                    return false;
                }
                image = image.convertToFormat(QImage::Format_RGB888);
            }

            if (!format.isEmpty() && format != "none") {
                DataExportResult exportResult;
                if (!DataExporter::exportData(format,
                                              imageFilePath,
                                              output._spriteFrames,
                                              output._atlasImage.size(),
                                              _trimSpriteNames,
                                              _prependSmartFolderName,
                                              &exportResult,
                                              errorMessage)) {
                    return false;
                }

                QSaveFile dataFile(outputFilePath + "." + exportResult.extension);
                if (!dataFile.open(QIODevice::WriteOnly)) {
                    if (errorMessage) {
                        *errorMessage = QString("Cannot open output file '%1': %2")
                            .arg(dataFile.fileName(), dataFile.errorString());
                    }
                    return false;
                }
                if (dataFile.write(exportResult.data) != exportResult.data.size() || !dataFile.commit()) {
                    if (errorMessage) {
                        *errorMessage = QString("Cannot write output file '%1': %2")
                            .arg(dataFile.fileName(), dataFile.errorString());
                    }
                    return false;
                }
            }

            if (!writeTexture(image, imageFilePath, errorMessage)) return false;

            if (i == textureVariantSource) {
                const QImage scalingSource = output._atlasImage.convertToFormat(
                    QImage::Format_ARGB32_Premultiplied);
                for (const TextureScaleVariant& variant : _textureScaleVariants) {
                    const QSize scaledSize(qMax(1, qRound(scalingSource.width() * variant.scale)),
                                           qMax(1, qRound(scalingSource.height() * variant.scale)));
                    QImage scaledImage = scalingSource.scaled(scaledSize,
                                                              Qt::IgnoreAspectRatio,
                                                              Qt::SmoothTransformation);
                    scaledImage = convertImage(scaledImage, _pixelFormat, _premultiplied);
                    if (_imageFormat == kJPG) {
                        scaledImage = scaledImage.convertToFormat(QImage::Format_RGB888);
                    }
                    const QString variantPath = outputFilePath + variant.suffix
                                              + imageExtension(_imageFormat);
                    if (!writeTexture(scaledImage, variantPath, errorMessage)) return false;
                    qDebug() << "texture scale variant:" << variantPath
                             << variant.scale << scaledSize;
                }
            }
        }
    }

    _spriteAtlases.clear();
    _fileNames.clear();
    _textureScaleVariants.clear();
    return true;
}
