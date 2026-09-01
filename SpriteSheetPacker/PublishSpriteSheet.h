#ifndef PUBLISHSPRITESHEET_H
#define PUBLISHSPRITESHEET_H

#include <QList>
#include <QString>
#include <QStringList>

#include "ImageFormat.h"
#include "SpriteAtlas.h"

class PublishSpriteSheet
{
public:
    PublishSpriteSheet();

    void addSpriteSheet(const SpriteAtlas& atlas, const QString& fileName);
    void setPixelFormat(PixelFormat pixelFormat) { _pixelFormat = pixelFormat; }
    void setPremultiplied(bool premultiplied) { _premultiplied = premultiplied; }
    void setTrimSpriteNames(bool value) { _trimSpriteNames = value; }
    void setPrependSmartFolderName(bool value) { _prependSmartFolderName = value; }
    void setPngCompression(int value) { _pngCompression = value; }
    void setPngQuant(bool enabled, const QString& quality)
    {
        _pngQuantEnabled = enabled;
        _pngQuantQuality = quality;
    }

    bool publish(const QString& format, QString* errorMessage = nullptr);

private:
    QList<SpriteAtlas> _spriteAtlases;
    QStringList _fileNames;
    PixelFormat _pixelFormat;
    bool _premultiplied;
    bool _trimSpriteNames;
    bool _prependSmartFolderName;
    int _pngCompression;
    bool _pngQuantEnabled;
    QString _pngQuantQuality;
};

#endif
