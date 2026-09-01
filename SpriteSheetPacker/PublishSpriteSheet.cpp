#include "PublishSpriteSheet.h"

#include "DataExporter.h"

#include <QFile>
#include <QFileInfo>
#include <QImageWriter>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace {

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
    : _pixelFormat(kARGB8888)
    , _premultiplied(true)
    , _trimSpriteNames(true)
    , _prependSmartFolderName(true)
    , _pngCompression(9)
    , _pngQuantEnabled(false)
    , _pngQuantQuality("80-95")
{
}

void PublishSpriteSheet::addSpriteSheet(const SpriteAtlas& atlas, const QString& fileName)
{
    _spriteAtlases.append(atlas);
    _fileNames.append(fileName);
}

bool PublishSpriteSheet::publish(const QString& format, QString* errorMessage)
{
    if (_spriteAtlases.size() != _fileNames.size()) {
        if (errorMessage) *errorMessage = "Internal error: atlas and output counts differ.";
        return false;
    }

    for (qsizetype i = 0; i < _spriteAtlases.size(); ++i) {
        const SpriteAtlas& atlas = _spriteAtlases.at(i);
        const QString& baseFilePath = _fileNames.at(i);

        for (qsizetype page = 0; page < atlas.outputData().size(); ++page) {
            const SpriteAtlas::OutputData& output = atlas.outputData().at(page);
            QString outputFilePath = baseFilePath;
            if (outputFilePath.contains("{n}")) {
                outputFilePath.replace("{n}", QString::number(page));
            } else if (outputFilePath.contains("{n1}")) {
                outputFilePath.replace("{n1}", QString::number(page + 1));
            } else if (atlas.outputData().size() > 1) {
                outputFilePath += "_" + QString::number(page);
            }

            const QString imageFilePath = outputFilePath + ".png";
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

            QImageWriter writer(imageFilePath, "png");
            writer.setOptimizedWrite(true);
            // Qt's PNG handler expects a 0..100 compression ratio, while the
            // CLI exposes the familiar PNG/zlib levels 0..9.
            writer.setCompression((_pngCompression * 100 + 4) / 9);
            const QImage image = convertImage(output._atlasImage, _pixelFormat, _premultiplied);
            if (!writer.write(image)) {
                if (errorMessage) {
                    *errorMessage = QString("Cannot write PNG '%1': %2")
                        .arg(imageFilePath, writer.errorString());
                }
                return false;
            }
            if (_pngQuantEnabled) {
                optimizeWithPngQuant(imageFilePath, _pngQuantQuality);
            }
        }
    }

    _spriteAtlases.clear();
    _fileNames.clear();
    return true;
}
