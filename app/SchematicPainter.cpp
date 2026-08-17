#include "SchematicPainter.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <algorithm>

namespace schematic {
namespace {

void drawLeads(QPainter& painter, const QRectF& bounds, double inset = 12.0) {
    const double centerY = bounds.center().y();
    painter.drawLine(QPointF(bounds.left(), centerY),
                     QPointF(bounds.left() + inset, centerY));
    painter.drawLine(QPointF(bounds.right() - inset, centerY),
                     QPointF(bounds.right(), centerY));
}

void drawGate(QPainter& painter, const QString& typeName, const QRectF& bounds) {
    const QRectF body = bounds.adjusted(12.0, 3.0, -13.0, -3.0);
    drawLeads(painter, bounds);

    if (typeName == "NotGate") {
        painter.drawPolygon(QPolygonF{
            QPointF(body.left(), body.top()),
            QPointF(body.left(), body.bottom()),
            QPointF(body.right() - 3.0, body.center().y())});
        painter.drawEllipse(QPointF(body.right(), body.center().y()), 3.0, 3.0);
        return;
    }

    QPainterPath path;
    if (typeName == "OrGate" || typeName == "XorGate") {
        path.moveTo(body.left(), body.top());
        path.quadTo(body.center().x(), body.top(), body.right(), body.center().y());
        path.quadTo(body.center().x(), body.bottom(), body.left(), body.bottom());
        path.quadTo(body.left() + 8.0, body.center().y(), body.left(), body.top());
        painter.drawPath(path);
        if (typeName == "XorGate") {
            QPainterPath extra;
            extra.moveTo(body.left() - 4.0, body.top());
            extra.quadTo(body.left() + 4.0, body.center().y(),
                         body.left() - 4.0, body.bottom());
            painter.drawPath(extra);
        }
    } else {
        path.moveTo(body.left(), body.top());
        path.lineTo(body.center().x(), body.top());
        path.arcTo(QRectF(body.center().x() - body.height() / 2.0,
                          body.top(), body.height(), body.height()),
                   90.0, -180.0);
        path.lineTo(body.left(), body.bottom());
        path.closeSubpath();
        painter.drawPath(path);
    }
    if (typeName == "NandGate") {
        painter.drawEllipse(QPointF(body.right() + 1.0, body.center().y()), 3.0, 3.0);
    }
}

void drawSevenSegment(QPainter& painter, const QRectF& bounds) {
    const QRectF display = bounds.adjusted(10.0, 4.0, -10.0, -4.0);
    painter.drawRoundedRect(display, 3.0, 3.0);
    QPen segmentPen(QColor("#c62828"), 2.4, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(segmentPen);
    const double left = display.left() + 7.0;
    const double right = display.right() - 7.0;
    const double top = display.top() + 5.0;
    const double middle = display.center().y();
    const double bottom = display.bottom() - 5.0;
    painter.drawLine(QPointF(left, top), QPointF(right, top));
    painter.drawLine(QPointF(left, middle), QPointF(right, middle));
    painter.drawLine(QPointF(left, bottom), QPointF(right, bottom));
    painter.drawLine(QPointF(left, top), QPointF(left, middle));
    painter.drawLine(QPointF(right, middle), QPointF(right, bottom));
}

}

void drawSymbol(QPainter& painter,
                const QString& typeName,
                const QRectF& bounds) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor("#263238"), 1.7,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    const QPointF center = bounds.center();
    if (typeName == "Ground") {
        painter.drawLine(QPointF(center.x(), bounds.top()),
                         QPointF(center.x(), center.y() + 5.0));
        painter.drawLine(QPointF(center.x() - 12.0, center.y() + 5.0),
                         QPointF(center.x() + 12.0, center.y() + 5.0));
        painter.drawLine(QPointF(center.x() - 8.0, center.y() + 10.0),
                         QPointF(center.x() + 8.0, center.y() + 10.0));
        painter.drawLine(QPointF(center.x() - 4.0, center.y() + 15.0),
                         QPointF(center.x() + 4.0, center.y() + 15.0));
    } else if (typeName == "DcVoltageSource" || typeName == "ClockGenerator") {
        painter.drawEllipse(center, 15.0, 15.0);
        painter.drawLine(QPointF(center.x(), bounds.top()),
                         QPointF(center.x(), center.y() - 15.0));
        painter.drawLine(QPointF(center.x(), center.y() + 15.0),
                         QPointF(center.x(), bounds.bottom()));
        if (typeName == "DcVoltageSource") {
            painter.drawText(QRectF(center.x() - 8.0, center.y() - 13.0, 16.0, 12.0),
                             Qt::AlignCenter, "+");
            painter.drawText(QRectF(center.x() - 8.0, center.y() + 1.0, 16.0, 10.0),
                             Qt::AlignCenter, "-");
        } else {
            QPainterPath wave;
            wave.moveTo(center.x() - 9.0, center.y() + 5.0);
            wave.lineTo(center.x() - 9.0, center.y() - 5.0);
            wave.lineTo(center.x(), center.y() - 5.0);
            wave.lineTo(center.x(), center.y() + 5.0);
            wave.lineTo(center.x() + 9.0, center.y() + 5.0);
            painter.drawPath(wave);
        }
    } else if (typeName == "Battery") {
        painter.drawLine(QPointF(center.x(), bounds.top()),
                         QPointF(center.x(), center.y() - 8.0));
        painter.drawLine(QPointF(center.x(), center.y() + 8.0),
                         QPointF(center.x(), bounds.bottom()));
        painter.drawLine(QPointF(center.x() - 14.0, center.y() - 8.0),
                         QPointF(center.x() + 14.0, center.y() - 8.0));
        painter.drawLine(QPointF(center.x() - 8.0, center.y() + 8.0),
                         QPointF(center.x() + 8.0, center.y() + 8.0));
    } else if (typeName == "Resistor") {
        drawLeads(painter, bounds, 9.0);
        QPainterPath path;
        path.moveTo(bounds.left() + 9.0, center.y());
        const double step = (bounds.width() - 18.0) / 8.0;
        for (int index = 1; index < 8; ++index) {
            path.lineTo(bounds.left() + 9.0 + index * step,
                        center.y() + (index % 2 == 0 ? -7.0 : 7.0));
        }
        path.lineTo(bounds.right() - 9.0, center.y());
        painter.drawPath(path);
    } else if (typeName == "Capacitor") {
        drawLeads(painter, bounds, bounds.width() / 2.0 - 5.0);
        painter.drawLine(QPointF(center.x() - 5.0, center.y() - 13.0),
                         QPointF(center.x() - 5.0, center.y() + 13.0));
        painter.drawLine(QPointF(center.x() + 5.0, center.y() - 13.0),
                         QPointF(center.x() + 5.0, center.y() + 13.0));
    } else if (typeName == "Inductor") {
        drawLeads(painter, bounds, 10.0);
        const double coilWidth = (bounds.width() - 20.0) / 4.0;
        for (int index = 0; index < 4; ++index) {
            painter.drawArc(QRectF(bounds.left() + 10.0 + index * coilWidth,
                                   center.y() - 7.0, coilWidth, 14.0),
                            0, 180 * 16);
        }
    } else if (typeName == "ToggleSwitch" || typeName == "PushButton") {
        drawLeads(painter, bounds, 12.0);
        painter.drawEllipse(QPointF(bounds.left() + 12.0, center.y()), 2.5, 2.5);
        painter.drawEllipse(QPointF(bounds.right() - 12.0, center.y()), 2.5, 2.5);
        if (typeName == "ToggleSwitch") {
            painter.drawLine(QPointF(bounds.left() + 14.0, center.y() - 1.0),
                             QPointF(bounds.right() - 14.0, center.y() - 10.0));
        } else {
            painter.drawLine(QPointF(center.x() - 10.0, center.y() - 8.0),
                             QPointF(center.x() + 10.0, center.y() - 8.0));
            painter.drawLine(QPointF(center.x(), center.y() - 17.0),
                             QPointF(center.x(), center.y() - 8.0));
        }
    } else if (typeName == "Led") {
        drawLeads(painter, bounds, 16.0);
        painter.drawPolygon(QPolygonF{
            QPointF(center.x() - 10.0, center.y() - 11.0),
            QPointF(center.x() - 10.0, center.y() + 11.0),
            QPointF(center.x() + 7.0, center.y())});
        painter.drawLine(QPointF(center.x() + 8.0, center.y() - 12.0),
                         QPointF(center.x() + 8.0, center.y() + 12.0));
        painter.drawLine(QPointF(center.x() + 9.0, center.y() - 13.0),
                         QPointF(center.x() + 17.0, center.y() - 20.0));
        painter.drawLine(QPointF(center.x() + 13.0, center.y() - 8.0),
                         QPointF(center.x() + 21.0, center.y() - 15.0));
    } else if (typeName == "SevenSegment") {
        drawSevenSegment(painter, bounds);
    } else if (typeName.endsWith("Gate")) {
        drawGate(painter, typeName, bounds);
    } else if (typeName == "DFlipFlop") {
        const QRectF body = bounds.adjusted(10.0, 2.0, -10.0, -2.0);
        drawLeads(painter, bounds, 10.0);
        painter.drawRect(body);
        painter.drawText(body.adjusted(3.0, 2.0, -3.0, -2.0),
                         Qt::AlignLeft | Qt::AlignTop, "D");
        painter.drawText(body.adjusted(3.0, 2.0, -3.0, -2.0),
                         Qt::AlignRight | Qt::AlignTop, "Q");
        painter.drawPolygon(QPolygonF{
            QPointF(body.left(), body.center().y() + 5.0),
            QPointF(body.left() + 6.0, body.center().y() + 9.0),
            QPointF(body.left(), body.center().y() + 13.0)});
    } else {
        painter.drawRoundedRect(bounds.adjusted(8.0, 3.0, -8.0, -3.0), 4.0, 4.0);
        painter.drawText(bounds, Qt::AlignCenter, typeName);
    }
    painter.restore();
}

}
