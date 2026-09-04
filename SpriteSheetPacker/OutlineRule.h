#ifndef OUTLINERULE_H
#define OUTLINERULE_H

#include <QString>

struct OutlineRule {
    QString pattern;
    float coarseness;
    bool centered = false;
};

#endif // OUTLINERULE_H
