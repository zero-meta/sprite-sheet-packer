#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include <QByteArray>
#include <QMap>
#include <QSize>
#include <QString>
#include <QStringList>

#include "SpriteAtlas.h"

struct DataExportResult {
    QByteArray data;
    QString extension;
};

class DataExporter
{
public:
    static QStringList supportedFormats();

    static bool exportData(const QString& format,
                           const QString& imageFilePath,
                           const QMap<QString, SpriteFrameInfo>& spriteFrames,
                           const QSize& textureSize,
                           bool trimSpriteNames,
                           bool prependSmartFolderName,
                           DataExportResult* result,
                           QString* errorMessage);
};

#endif
