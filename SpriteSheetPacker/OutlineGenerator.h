#ifndef OUTLINEGENERATOR_H
#define OUTLINEGENERATOR_H

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QVector>

class OutlineGenerator
{
public:
    static QVector<QPoint> generate(const QImage& image,
                                    const QRect& sourceRect,
                                    float coarseness);
};

#endif // OUTLINEGENERATOR_H
