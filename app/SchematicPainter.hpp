#pragma once

#include <QRectF>
#include <QString>

class QPainter;

namespace schematic {

void drawSymbol(QPainter& painter,
                const QString& typeName,
                const QRectF& bounds);

}
