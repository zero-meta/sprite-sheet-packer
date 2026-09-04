#ifndef SPRITEPACKERPROJECTFILE_H
#define SPRITEPACKERPROJECTFILE_H

#include <QtCore>
#include "ImageFormat.h"
#include "OutlineRule.h"
#include "TextureScaleVariant.h"

struct ScalingVariant{
    QString name;
    float   scale;
    int     maxTextureSize;
    bool    pow2;
    bool    forceSquared;
};

class SpritePackerProjectFile
{
public:
    SpritePackerProjectFile();
    virtual ~SpritePackerProjectFile();

    void setAlgorithm(const QString& algorithm) { _algorithm = algorithm; }
    QString algorithm() const { return _algorithm; }

    void setTrimMode(const QString& trimMode) { _trimMode = trimMode; }
    QString trimMode() const { return _trimMode; }

    void setTrimThreshold(int trimThreshold) { _trimThreshold = trimThreshold; }
    int trimThreshold() const { return _trimThreshold; }

    void setEpsilon(float epsilon) { _epsilon = epsilon; }
    float epsilon() const { return _epsilon; }

    void setHeuristicMask(bool heuristicMask) { _heuristicMask = heuristicMask; }
    bool heuristicMask() const { return _heuristicMask; }

    void setRotateSprites(bool rotate) { _rotateSprites = rotate; }
    bool rotateSprites() const { return _rotateSprites; }

    void setTextureBorder(int textureBorder) { _textureBorder = textureBorder; }
    int textureBorder() const { return _textureBorder; }

    void setSpriteBorder(int spriteBorder) { _spriteBorder = spriteBorder; }
    int spriteBorder() const { return _spriteBorder; }

    void setExtrude(int pixels) { _extrude = pixels; }
    int extrude() const { return _extrude; }
    void setExtrudeRules(const QVector<QPair<QString, int>>& rules) { _extrudeRules = rules; }
    const QVector<QPair<QString, int>>& extrudeRules() const { return _extrudeRules; }

    void setOutlineCoarseness(float value) { _outlineCoarseness = value; }
    float outlineCoarseness() const { return _outlineCoarseness; }
    void setOutlineRules(const QVector<OutlineRule>& rules) { _outlineRules = rules; }
    const QVector<OutlineRule>& outlineRules() const { return _outlineRules; }

    void setImageFormat(ImageFormat imageFormat) { _imageFormat = imageFormat; }
    ImageFormat imageFormat() const { return _imageFormat; }

    void setPixelFormat(PixelFormat pixelFormat) { _pixelFormat = pixelFormat; }
    PixelFormat pixelFormat() const { return _pixelFormat; }

    void setPremultiplied(bool premultiplied) { _premultiplied = premultiplied; }
    bool premultiplied() const { return _premultiplied; }

    void setPngOptMode(const QString& optMode) { _pngOptMode = optMode; }
    const QString& pngOptMode() const { return _pngOptMode; }

    void setPngOptLevel(int optLevel) { _pngOptLevel = optLevel; }
    int pngOptLevel() const { return _pngOptLevel; }

    void setPngQuantQuality(const QString& quality) { _pngQuantQuality = quality; }
    const QString& pngQuantQuality() const { return _pngQuantQuality; }

    void setWebpQuality(int quality) { _webpQuality = quality; }
    int webpQuality() const { return _webpQuality; }

    void setJpgQuality(int quality) { _jpgQuality = quality; }
    int jpgQuality() const { return _jpgQuality; }

    void setScalingVariants(const QVector<ScalingVariant>& scalingVariants) { _scalingVariants = scalingVariants; }
    const QVector<ScalingVariant>& scalingVariants() const { return _scalingVariants; }

    void setTextureScaleVariants(const QVector<TextureScaleVariant>& variants) { _textureScaleVariants = variants; }
    const QVector<TextureScaleVariant>& textureScaleVariants() const { return _textureScaleVariants; }

    void setDataFormat(const QString& dataFormat) { _dataFormat = dataFormat; }
    const QString& dataFormat() const { return _dataFormat; }

    void setDestPath(const QString& destPath) { _destPath = destPath; }
    const QString& destPath() const { return _destPath; }

    void setSpriteSheetName(const QString& spriteSheetName) { _spriteSheetName = spriteSheetName; }
    const QString& spriteSheetName() const { return _spriteSheetName; }

    void setSrcList(const QStringList& srcList) { _srcList = srcList; }
    const QStringList& srcList() const { return _srcList; }

    void setTrimSpriteNames(bool trimSpriteNames) { _trimSpriteNames = trimSpriteNames; }
    bool trimSpriteNames() const { return _trimSpriteNames; }

    void setPrependSmartFolderName(bool prependSmartFolderName) { _prependSmartFolderName = prependSmartFolderName; }
    bool prependSmartFolderName() const { return _prependSmartFolderName; }

    void setEncryptionKey(const QString& key) { _encryptionKey = key; }
    const QString& encryptionKey() const { return _encryptionKey; }

    virtual bool write(const QString& fileName);
    virtual bool read(const QString& fileName);

protected:
    QString     _algorithm;
    QString     _trimMode;
    int         _trimThreshold;
    float       _epsilon;
    bool        _heuristicMask;
    bool        _rotateSprites;
    int         _textureBorder;
    int         _spriteBorder;
    int         _extrude;
    QVector<QPair<QString, int>> _extrudeRules;
    float       _outlineCoarseness;
    QVector<OutlineRule> _outlineRules;
    ImageFormat _imageFormat;
    PixelFormat _pixelFormat;
    bool        _premultiplied;

    QString     _pngOptMode;
    int         _pngOptLevel;
    QString     _pngQuantQuality;
    int         _webpQuality;
    int         _jpgQuality;

    QVector<ScalingVariant> _scalingVariants;
    QVector<TextureScaleVariant> _textureScaleVariants;

    QString     _dataFormat;
    QString     _destPath;
    QString     _spriteSheetName;
    QStringList _srcList;

    bool        _trimSpriteNames;
    bool        _prependSmartFolderName;
    QString     _encryptionKey;

};

class SpritePackerProjectFileTPS: public SpritePackerProjectFile {
public:
    virtual bool write(const QString&) { return false; }
    virtual bool read(const QString& fileName);
};


#endif // SPRITEPACKERPROJECTFILE_H
