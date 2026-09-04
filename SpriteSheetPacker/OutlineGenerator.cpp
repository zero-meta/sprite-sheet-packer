#include "OutlineGenerator.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

enum Direction {
    DirectionNone,
    DirectionUp,
    DirectionLeft,
    DirectionDown,
    DirectionRight
};

class AlphaView
{
public:
    AlphaView(const QImage& image, const QRect& sourceRect)
        : _image(image)
        , _sourceRect(sourceRect)
    {
    }

    int width() const { return _sourceRect.width() + 1; }
    int height() const { return _sourceRect.height() + 1; }

    bool isValid(int x, int y) const
    {
        return x >= 0 && x < width() && y >= 0 && y < height();
    }

    bool isSolid(int x, int y) const
    {
        if (x < 0 || x >= _sourceRect.width()
            || y < 0 || y >= _sourceRect.height()) {
            return false;
        }
        const int imageX = _sourceRect.x() + x;
        const int imageY = _sourceRect.y() + y;
        if (imageX < 0 || imageX >= _image.width()
            || imageY < 0 || imageY >= _image.height()) {
            return false;
        }
        return qAlpha(_image.pixel(imageX, imageY)) > 0;
    }

private:
    const QImage& _image;
    QRect _sourceRect;
};

Direction nextDirection(const AlphaView& view, int x, int y, Direction previous)
{
    int state = 0;
    if (view.isSolid(x - 1, y - 1)) state |= 1;
    if (view.isSolid(x, y - 1)) state |= 2;
    if (view.isSolid(x - 1, y)) state |= 4;
    if (view.isSolid(x, y)) state |= 8;

    switch (state) {
        case 1: return DirectionUp;
        case 2:
        case 3: return DirectionRight;
        case 4: return DirectionLeft;
        case 5: return DirectionUp;
        case 6: return previous == DirectionUp ? DirectionLeft : DirectionRight;
        case 7: return DirectionRight;
        case 8: return DirectionDown;
        case 9: return previous == DirectionRight ? DirectionUp : DirectionDown;
        case 10:
        case 11: return DirectionDown;
        case 12: return DirectionLeft;
        case 13: return DirectionUp;
        case 14: return DirectionLeft;
        default: return DirectionNone;
    }
}

QVector<QPoint> traceOutline(const AlphaView& view)
{
    int startX = -1;
    int startY = -1;
    for (int y = 0; y < view.height() && startX < 0; ++y) {
        for (int x = 0; x < view.width(); ++x) {
            if (view.isSolid(x, y)) {
                startX = x;
                startY = y;
                break;
            }
        }
    }
    if (startX < 0) return {};

    QVector<QPoint> points;
    points.reserve(2 * (view.width() + view.height()));

    int x = startX;
    int y = startY;
    Direction next = DirectionNone;
    const qint64 maximumSteps = qMax<qint64>(16,
        static_cast<qint64>(view.width()) * view.height() * 4);

    for (qint64 step = 0; step < maximumSteps; ++step) {
        const Direction previous = next;
        next = nextDirection(view, x, y, previous);
        if (next == DirectionNone) return {};

        if (view.isValid(x, y)) points.append(QPoint(x, y));

        switch (next) {
            case DirectionUp: --y; break;
            case DirectionLeft: --x; break;
            case DirectionDown: ++y; break;
            case DirectionRight: ++x; break;
            default: break;
        }
        if (x == startX && y == startY) return points;
    }

    return {};
}

double perpendicularDistance(const QPoint& point,
                             const QPoint& first,
                             const QPoint& last)
{
    if (first.x() == last.x()) {
        return std::abs(point.x() - first.x());
    }
    const double slope = static_cast<double>(last.y() - first.y())
                       / static_cast<double>(last.x() - first.x());
    const double intercept = first.y() - slope * first.x();
    return std::abs(slope * point.x() - point.y() + intercept)
         / std::sqrt(slope * slope + 1.0);
}

QVector<QPoint> simplifyOutline(const QVector<QPoint>& points, float epsilon)
{
    if (points.size() < 3) return points;

    QVector<bool> keep(points.size(), false);
    keep[0] = true;
    keep[points.size() - 1] = true;

    QVector<std::pair<qsizetype, qsizetype>> pending;
    pending.append({0, points.size() - 1});
    while (!pending.isEmpty()) {
        const auto range = pending.takeLast();
        qsizetype index = -1;
        double distance = 0.0;
        for (qsizetype i = range.first + 1; i < range.second; ++i) {
            const double candidate = perpendicularDistance(points[i],
                                                           points[range.first],
                                                           points[range.second]);
            if (candidate > distance) {
                distance = candidate;
                index = i;
            }
        }
        if (index >= 0 && distance > epsilon) {
            keep[index] = true;
            pending.append({range.first, index});
            pending.append({index, range.second});
        }
    }

    QVector<QPoint> result;
    result.reserve(points.size());
    for (qsizetype i = 0; i < points.size(); ++i) {
        if (keep[i]) result.append(points[i]);
    }
    return result;
}

} // namespace

QVector<QPoint> OutlineGenerator::generate(const QImage& image,
                                           const QRect& sourceRect,
                                           float coarseness)
{
    if (image.isNull() || sourceRect.isEmpty() || coarseness <= 0.0f) return {};

    QVector<QPoint> result = simplifyOutline(traceOutline(AlphaView(image, sourceRect)),
                                             coarseness);
    std::reverse(result.begin(), result.end());
    if (result.size() < 3) return {};
    return result;
}
