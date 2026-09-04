#ifndef SPRITEATLAS_H
#define SPRITEATLAS_H

#include <QtCore>
#include <QImage>

#include "OutlineRule.h"
#include "PolygonImage.h"

struct SpriteFrameInfo {
public:
    QRect   frame;
    QPoint  offset;
    bool    rotated;
    QRect   sourceColorRect;
    QSize   sourceSize;
    QVector<QPoint> outline;
    bool outlineCentered = false;

    Triangles triangles;
};

class PackContent {
public:
    PackContent();
    PackContent(const QString& name, const QImage& image);

    bool isIdentical(const PackContent& other);
    void trim(int alpha);
    void setTriangles(const Triangles& triangles) { _triangles = triangles; }
    void setPolygons(const Polygons& polygons) { _polygons = polygons; }
    void setExtrude(int pixels) { _extrude = pixels; }

    const QString& name() const { return _name; }
    const QImage& image() const { return _image; }
    const QRect& rect() const { return _rect; }
    const Triangles& triangles() const { return _triangles; }
    const Polygons& polygons() const { return _polygons; }
    int extrude() const { return _extrude; }

private:
    QString _name;
    QImage  _image;
    QRect   _rect;
    Triangles _triangles;
    Polygons  _polygons;
    int _extrude = 0;
};

class SpriteAtlasGenerateProgress
{
public:
    virtual ~SpriteAtlasGenerateProgress() = default;

    virtual void setProgressText(const QString&) { }
};

class SpriteAtlas
{
public:
    struct OutputData {
        QImage _atlasImage;
        QMap<QString, SpriteFrameInfo> _spriteFrames;
    };

public:
    SpriteAtlas(const QStringList& sourceList = QStringList(),
                int textureBorder = 0,
                int spriteBorder = 1,
                int trim = 1,
                bool heuristicMask = false,
                bool pow2 = false,
                bool forceSquared = false,
                int maxSize = 8192,
                float scale = 1);

    void setAlgorithm(const QString& algorithm) { _algorithm = algorithm; }
    void enablePolygonMode(bool enable, float epsilon = 2.f);

    void setRotateSprites(bool value) { _rotateSprites = value; }
    void setExtrude(int pixels) { _extrude = pixels; }
    void setExtrudeRules(const QVector<QPair<QString, int>>& rules) { _extrudeRules = rules; }
    void setOutlineCoarseness(float value) { _outlineCoarseness = value; }
    void setOutlineRules(const QVector<OutlineRule>& rules) { _outlineRules = rules; }
    void setOutlineCenteredOverride(bool centered)
    {
        _hasOutlineCenteredOverride = true;
        _outlineCenteredOverride = centered;
    }

    bool generate(SpriteAtlasGenerateProgress* progress = nullptr);
    void abortGeneration() { _aborted = true; }

    QString algorithm() const { return _algorithm; }
    float scale() const { return _scale; }

    const QVector<OutputData>& outputData() const { return _outputData; }
    const QMap<QString, QVector<QString>>& identicalFrames() const { return _identicalFrames; }

protected:
    bool packWithRect(const QVector<PackContent>& content);
    bool packWithPolygon(const QVector<PackContent>& content);

    void onPlaceCallback(int current, int count);

private:
    QStringList _sourceList;
    QString _algorithm;
    int _trim;
    int _textureBorder;
    int _spriteBorder;
    int _extrude;
    QVector<QPair<QString, int>> _extrudeRules;
    float _outlineCoarseness;
    QVector<OutlineRule> _outlineRules;
    bool _hasOutlineCenteredOverride;
    bool _outlineCenteredOverride;
    bool _heuristicMask;
    bool _pow2;
    bool _forceSquared;
    int _maxTextureSize;
    float _scale;
    bool _rotateSprites;
    // polygon mode
    struct TPolygonMode{
        bool enable;
        float epsilon;
    } _polygonMode;

    SpriteAtlasGenerateProgress* _progress;

    // output data
    QVector<OutputData> _outputData;
    QMap<QString, QVector<QString>> _identicalFrames;
    QMap<QString, QVector<QPoint>> _outlines;
    QMap<QString, bool> _outlineCentered;

    bool _aborted;
};

#endif // SPRITEATLAS_H
