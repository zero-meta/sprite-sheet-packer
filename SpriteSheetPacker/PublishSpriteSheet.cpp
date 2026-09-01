#include "PublishSpriteSheet.h"

#include "DataExporter.h"

#include <QImageWriter>
#include <QSaveFile>

PublishSpriteSheet::PublishSpriteSheet()
    : _pixelFormat(kARGB8888)
    , _premultiplied(true)
    , _trimSpriteNames(true)
    , _prependSmartFolderName(true)
    , _pngCompression(9)
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
            writer.setCompression(_pngCompression);
            const QImage image = convertImage(output._atlasImage, _pixelFormat, _premultiplied);
            if (!writer.write(image)) {
                if (errorMessage) {
                    *errorMessage = QString("Cannot write PNG '%1': %2")
                        .arg(imageFilePath, writer.errorString());
                }
                return false;
            }
        }
    }

    _spriteAtlases.clear();
    _fileNames.clear();
    return true;
}
