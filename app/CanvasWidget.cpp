#include "CanvasWidget.hpp"
#include "SchematicPainter.hpp"

#include "proteus/components/Digital.hpp"
#include "proteus/components/Interactive.hpp"
#include "proteus/components/Passive.hpp"
#include "proteus/components/Sources.hpp"
#include "proteus/drc/DesignRuleChecker.hpp"

#include <QInputDialog>
#include <QImage>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <stdexcept>

CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent), viewport_(document_.canvas), history_(100) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(720, 480);
    history_.reset(document_);
    rebuildSimulation();
    simulationTimer_.setInterval(40);
    connect(&simulationTimer_, &QTimer::timeout, this, [this] {
        if (simulation_ && simulation_->state() == proteus::SimulationState::Running) {
            simulation_->advance(0.04);
            update();
        }
    });
}

const proteus::ProjectDocument& CanvasWidget::document() const noexcept {
    return document_;
}

void CanvasWidget::newDocument(proteus::CanvasSettings settings) {
    simulationTimer_.stop();
    document_ = {};
    document_.canvas = settings;
    viewport_.setSettings(settings);
    viewport_.resetView();
    resetInteractionState();
    labelCounters_.clear();
    history_.reset(document_);
    rebuildSimulation();
    modified_ = false;
    emit circuitChanged();
    emit historyChanged();
    update();
}

void CanvasWidget::loadDocument(proteus::ProjectDocument document) {
    simulationTimer_.stop();
    document_ = std::move(document);
    viewport_.setSettings(document_.canvas);
    viewport_.resetView();
    resetInteractionState();
    rebuildLabelCounters();
    history_.reset(document_);
    rebuildSimulation();
    modified_ = false;
    emit circuitChanged();
    emit historyChanged();
    update();
}

void CanvasWidget::setPlacementType(const std::string& typeName) {
    if (!editingAllowed("place components")) {
        return;
    }
    placementType_ = typeName;
    pendingWire_.reset();
    emit statusChanged(QString("Click the canvas to place %1").arg(
        QString::fromStdString(typeName)));
}

void CanvasWidget::clearPlacementType() {
    placementType_.clear();
    emit statusChanged("Selection mode");
}

bool CanvasWidget::canUndo() const noexcept { return history_.canUndo(); }
bool CanvasWidget::canRedo() const noexcept { return history_.canRedo(); }
bool CanvasWidget::isModified() const noexcept { return modified_; }
void CanvasWidget::markSaved() noexcept { modified_ = false; }

bool CanvasWidget::removeComponent(proteus::ComponentId id) {
    if (!editingAllowed("remove components")) {
        return false;
    }
    try {
        document_.circuit.removeComponent(id);
    } catch (const std::out_of_range&) {
        return false;
    }
    selection_.remove(id);
    selectedWire_.reset();
    recordChange();
    return true;
}

void CanvasWidget::undo() {
    if (!editingAllowed("undo changes") || !history_.canUndo()) {
        return;
    }
    document_ = history_.undo();
    viewport_.setSettings(document_.canvas);
    resetInteractionState();
    rebuildLabelCounters();
    rebuildSimulation();
    modified_ = true;
    emit circuitChanged();
    emit historyChanged();
    update();
}

void CanvasWidget::redo() {
    if (!editingAllowed("redo changes") || !history_.canRedo()) {
        return;
    }
    document_ = history_.redo();
    viewport_.setSettings(document_.canvas);
    resetInteractionState();
    rebuildLabelCounters();
    rebuildSimulation();
    modified_ = true;
    emit circuitChanged();
    emit historyChanged();
    update();
}

void CanvasWidget::runSimulation() {
    clearPlacementType();
    pendingWire_.reset();
    if (simulation_->run()) {
        simulationTimer_.start();
    }
    writeSimulationLog();
    update();
}

void CanvasWidget::pauseSimulation() {
    simulation_->pause();
    simulationTimer_.stop();
    writeSimulationLog();
    update();
}

void CanvasWidget::stopSimulation() {
    simulation_->stop();
    simulationTimer_.stop();
    writeSimulationLog();
    update();
}

void CanvasWidget::stepSimulation() {
    simulationTimer_.stop();
    simulation_->stepToNextEvent(0.01);
    writeSimulationLog();
    update();
}

void CanvasWidget::runDesignRuleCheck() {
    if (simulation_->state() != proteus::SimulationState::Stopped) {
        simulation_->stop();
    }
    simulationTimer_.stop();
    const auto issues = proteus::DesignRuleChecker::inspect(document_.circuit);
    for (const auto& issue : issues) {
        emit logMessage(QString("[%1] %2").arg(
            QString::fromStdString(issue.code),
            QString::fromStdString(issue.message)));
    }
    rebuildSimulation();
    update();
}

bool CanvasWidget::exportPng(const QString& path) {
    QImage image(size() * devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatioF());
    image.fill(Qt::white);
    QPainter painter(&image);
    render(&painter);
    return image.save(path, "PNG");
}

QPointF CanvasWidget::toQPoint(proteus::Point point) const {
    return {point.x, point.y};
}

proteus::Point CanvasWidget::toPoint(QPointF point) const {
    return {point.x(), point.y()};
}

QRectF CanvasWidget::componentRect(const proteus::Component& component) const {
    double width = component.typeName() == "SevenSegment" ? 72.0 : 58.0;
    double height = component.typeName() == "SevenSegment" ? 76.0 : 42.0;
    if (component.rotation() == proteus::Rotation::Deg90 ||
        component.rotation() == proteus::Rotation::Deg270) {
        std::swap(width, height);
    }
    return QRectF(component.position().x - width / 2.0,
                  component.position().y - height / 2.0,
                  width,
                  height);
}

std::optional<proteus::ComponentId> CanvasWidget::componentAt(
    proteus::Point point) const {
    const auto ids = document_.circuit.componentIds();
    for (auto iterator = ids.rbegin(); iterator != ids.rend(); ++iterator) {
        if (componentRect(document_.circuit.component(*iterator)).contains(toQPoint(point))) {
            return *iterator;
        }
    }
    return std::nullopt;
}

std::optional<proteus::PinRef> CanvasWidget::pinAt(proteus::Point point) const {
    for (const auto reference : document_.circuit.allPins()) {
        const auto& pin = document_.circuit.pin(reference);
        if (pin.isMouseOver(point,
                            document_.circuit.component(reference.componentId)
                                .pinPosition(reference.pinIndex))) {
            return reference;
        }
    }
    return std::nullopt;
}

std::optional<proteus::WireId> CanvasWidget::wireAt(proteus::Point point) const {
    const auto ids = document_.circuit.wireIds();
    for (auto iterator = ids.rbegin(); iterator != ids.rend(); ++iterator) {
        if (document_.circuit.wire(*iterator).containsPoint(point, 6.0 / viewport_.zoom())) {
            return *iterator;
        }
    }
    return std::nullopt;
}

QString CanvasWidget::nextLabel(const std::string& typeName) {
    rebuildLabelCounters();
    const std::string prefix = typeName == "Resistor" ? "R" :
        typeName == "Capacitor" ? "C" :
        typeName == "Inductor" ? "L" :
        typeName == "Ground" ? "GND" :
        typeName == "Led" ? "LED" :
        typeName == "ToggleSwitch" ? "SW" :
        typeName == "PushButton" ? "BTN" :
        typeName == "DcVoltageSource" ? "V" :
        typeName == "Battery" ? "BAT" :
        typeName == "ClockGenerator" ? "CLK" : "U";
    const int index = ++labelCounters_[prefix];
    return QString::fromStdString(prefix) + QString::number(index);
}

void CanvasWidget::rebuildLabelCounters() {
    labelCounters_.clear();
    for (const auto id : document_.circuit.componentIds()) {
        const std::string& label = document_.circuit.component(id).label();
        std::size_t digit = label.size();
        while (digit > 0 &&
               std::isdigit(static_cast<unsigned char>(label[digit - 1])) != 0) {
            --digit;
        }
        if (digit == label.size()) {
            continue;
        }
        try {
            const int suffix = std::stoi(label.substr(digit));
            labelCounters_[label.substr(0, digit)] =
                std::max(labelCounters_[label.substr(0, digit)], suffix);
        } catch (const std::exception&) {
            // An unusually large numeric suffix is still a valid user label.
        }
    }
}

void CanvasWidget::rebuildSimulation() {
    simulationTimer_.stop();
    pressedButton_.reset();
    simulation_ = std::make_unique<proteus::SimulationEngine>(document_.circuit);
}

void CanvasWidget::resetInteractionState() {
    selection_.clear();
    placementType_.clear();
    pendingWire_.reset();
    draggedComponent_.reset();
    pressedButton_.reset();
    selectedWire_.reset();
    panning_ = false;
    selecting_ = false;
    componentMoved_ = false;
    unsetCursor();
}

bool CanvasWidget::editingAllowed(const QString& action) {
    if (!simulation_ ||
        simulation_->state() == proteus::SimulationState::Stopped) {
        return true;
    }
    const QString message = QString("Stop the simulation before you %1.").arg(action);
    emit statusChanged(message);
    emit logMessage(message);
    return false;
}

void CanvasWidget::recordChange() {
    history_.record(document_);
    rebuildSimulation();
    modified_ = true;
    emit circuitChanged();
    emit historyChanged();
    update();
}

void CanvasWidget::removeSelection() {
    if (!editingAllowed("delete circuit items")) {
        return;
    }
    if (selectedWire_.has_value()) {
        document_.circuit.removeWire(*selectedWire_);
        selectedWire_.reset();
        recordChange();
        return;
    }
    if (selection_.items().empty()) {
        return;
    }
    const auto selected = selection_.items();
    for (const auto id : selected) {
        document_.circuit.removeComponent(id);
    }
    selection_.clear();
    recordChange();
}

void CanvasWidget::rotateSelection() {
    if (!editingAllowed("rotate components")) {
        return;
    }
    for (const auto id : selection_.items()) {
        document_.circuit.component(id).rotateClockwise();
        document_.circuit.refreshWireRoutes(id);
    }
    if (!selection_.items().empty()) {
        recordChange();
    }
}

void CanvasWidget::mirrorSelection(bool horizontal) {
    if (!editingAllowed("mirror components")) {
        return;
    }
    for (const auto id : selection_.items()) {
        auto& component = document_.circuit.component(id);
        if (horizontal) {
            component.mirrorHorizontal();
        } else {
            component.mirrorVertical();
        }
        document_.circuit.refreshWireRoutes(id);
    }
    if (!selection_.items().empty()) {
        recordChange();
    }
}

void CanvasWidget::editComponent(proteus::ComponentId id) {
    if (!editingAllowed("edit component properties")) {
        return;
    }
    auto& component = document_.circuit.component(id);
    bool accepted = false;
    const QString label = QInputDialog::getText(
        this, "Component Properties", "Label", QLineEdit::Normal,
        QString::fromStdString(component.label()), &accepted);
    if (!accepted) {
        return;
    }
    if (auto* value = dynamic_cast<proteus::DcVoltageSource*>(&component)) {
        value->setVoltage(QInputDialog::getDouble(
            this, "Component Properties", "Voltage", value->voltage(), -100000.0, 100000.0, 6));
    } else if (auto* value = dynamic_cast<proteus::Battery*>(&component)) {
        value->setVoltage(QInputDialog::getDouble(
            this, "Component Properties", "Voltage", value->voltage(), -100000.0, 100000.0, 6));
        value->setInternalResistanceOhms(QInputDialog::getDouble(
            this, "Component Properties", "Internal resistance (ohm)",
            value->internalResistanceOhms(), 0.0, 1e12, 6));
    } else if (auto* value = dynamic_cast<proteus::Resistor*>(&component)) {
        value->setResistanceOhms(QInputDialog::getDouble(
            this, "Component Properties", "Resistance (ohm)", value->resistanceOhms(), 1e-9, 1e15, 6));
    } else if (auto* value = dynamic_cast<proteus::Capacitor*>(&component)) {
        value->setCapacitanceFarads(QInputDialog::getDouble(
            this, "Component Properties", "Capacitance (F)", value->capacitanceFarads(), 1e-15, 1e6, 12));
    } else if (auto* value = dynamic_cast<proteus::Inductor*>(&component)) {
        value->setInductanceHenrys(QInputDialog::getDouble(
            this, "Component Properties", "Inductance (H)", value->inductanceHenrys(), 1e-15, 1e6, 12));
    } else if (auto* value = dynamic_cast<proteus::ClockGenerator*>(&component)) {
        value->setPeriodSeconds(QInputDialog::getDouble(
            this, "Component Properties", "Period (s)", value->periodSeconds(), 1e-9, 1e9, 9));
        value->setDutyCycle(QInputDialog::getDouble(
            this, "Component Properties", "Duty cycle (0 to 1)",
            value->dutyCycle(), 0.001, 0.999, 3));
    } else if (auto* value = dynamic_cast<proteus::ToggleSwitch*>(&component)) {
        const QString state = QInputDialog::getItem(
            this, "Component Properties", "Initial state",
            {"Open", "Closed"}, value->isClosed() ? 1 : 0, false);
        if (!state.isEmpty()) {
            value->setClosed(state == "Closed");
        }
    } else if (auto* value = dynamic_cast<proteus::Led*>(&component)) {
        value->setThresholdVoltage(QInputDialog::getDouble(
            this, "Component Properties", "Forward threshold (V)",
            value->thresholdVoltage(), 0.001, 1000.0, 3));
        const QString color = QInputDialog::getText(
            this, "Component Properties", "Color name or #RRGGBB",
            QLineEdit::Normal, QString::fromStdString(value->color()));
        if (!color.trimmed().isEmpty()) {
            value->setColor(color.trimmed().toStdString());
        }
    } else if (auto* value = dynamic_cast<proteus::SevenSegment*>(&component)) {
        const QString decimalPoint = QInputDialog::getItem(
            this, "Component Properties", "Decimal point pin",
            {"Disabled", "Enabled"}, value->includesDecimalPoint() ? 1 : 0,
            false);
        if (!decimalPoint.isEmpty()) {
            value->setIncludesDecimalPoint(decimalPoint == "Enabled");
        }
    } else if (auto* value = dynamic_cast<proteus::LogicGate*>(&component)) {
        value->setPropagationDelaySeconds(QInputDialog::getDouble(
            this, "Component Properties", "Propagation delay (s)",
            value->propagationDelaySeconds(), 0.0, 1e6, 9));
    } else if (auto* value = dynamic_cast<proteus::DFlipFlop*>(&component)) {
        value->setPropagationDelaySeconds(QInputDialog::getDouble(
            this, "Component Properties", "Propagation delay (s)",
            value->propagationDelaySeconds(), 0.0, 1e6, 9));
    }
    component.setLabel(label.trimmed().isEmpty()
                           ? component.label()
                           : label.trimmed().toStdString());
    recordChange();
}

void CanvasWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#e9edf2"));
    painter.save();
    painter.translate(viewport_.pan().x, viewport_.pan().y);
    painter.scale(viewport_.zoom(), viewport_.zoom());
    painter.fillRect(QRectF(0.0, 0.0, document_.canvas.width, document_.canvas.height), Qt::white);
    drawGrid(painter);
    drawCircuit(painter);
    painter.restore();
    if (selecting_) {
        painter.setPen(QPen(QColor("#1565c0"), 1.0, Qt::DashLine));
        painter.setBrush(QColor(21, 101, 192, 35));
        painter.drawRect(QRectF(selectionStart_, selectionCurrent_).normalized());
    }
}

void CanvasWidget::drawGrid(QPainter& painter) {
    if (!document_.canvas.gridVisible) {
        return;
    }
    painter.setPen(QPen(QColor("#d9dee5"), 0.0));
    const double spacing = document_.canvas.gridSpacing;
    for (double x = 0.0; x <= document_.canvas.width; x += spacing) {
        for (double y = 0.0; y <= document_.canvas.height; y += spacing) {
            painter.drawPoint(QPointF(x, y));
        }
    }
}

void CanvasWidget::drawCircuit(QPainter& painter) {
    for (const auto wireId : document_.circuit.wireIds()) {
        const auto& wire = document_.circuit.wire(wireId);
        const QString color = simulation_ ?
            QString::fromStdString(simulation_->wireColor(wireId)) : "#404040";
        const bool selected = selectedWire_.has_value() && *selectedWire_ == wireId;
        painter.setPen(QPen(selected ? QColor("#00acc1") : QColor(color),
                            (selected ? 5.0 : 3.0) / viewport_.zoom(), Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        QPainterPath path;
        if (!wire.route().empty()) {
            path.moveTo(toQPoint(wire.route().front()));
            for (std::size_t index = 1; index < wire.route().size(); ++index) {
                path.lineTo(toQPoint(wire.route()[index]));
            }
            painter.drawPath(path);
        }
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#263238"));
    for (const auto junctionId : document_.circuit.junctionIds()) {
        painter.drawEllipse(toQPoint(document_.circuit.junction(junctionId).position), 5.0, 5.0);
    }
    for (const auto id : document_.circuit.componentIds()) {
        drawComponent(painter, document_.circuit.component(id));
    }
}

void CanvasWidget::drawComponent(QPainter& painter,
                                 const proteus::Component& component) {
    const QRectF bounds = componentRect(component);
    const bool selected = selection_.contains(component.id());
    if (selected) {
        painter.setPen(QPen(QColor("#1565c0"), 1.5 / viewport_.zoom(),
                            Qt::DashLine));
        painter.setBrush(QColor(21, 101, 192, 24));
        painter.drawRoundedRect(bounds.adjusted(-4.0, -4.0, 4.0, 4.0),
                                5.0, 5.0);
    }
    if (const auto* led = dynamic_cast<const proteus::Led*>(&component);
        led && led->isIlluminated()) {
        QColor glow(QString::fromStdString(led->color()));
        if (!glow.isValid()) {
            glow = QColor("#ef5350");
        }
        glow.setAlpha(70);
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(componentRect(component).center(), 24.0, 24.0);
    }

    painter.save();
    painter.translate(component.position().x, component.position().y);
    painter.rotate(static_cast<int>(component.rotation()));
    painter.scale(component.mirroredVertically() ? -1.0 : 1.0,
                  component.mirroredHorizontally() ? -1.0 : 1.0);
    const double symbolWidth =
        component.typeName() == "SevenSegment" ? 72.0 : 58.0;
    const double symbolHeight =
        component.typeName() == "SevenSegment" ? 76.0 : 42.0;
    const QRectF localBounds(-symbolWidth / 2.0, -symbolHeight / 2.0,
                             symbolWidth, symbolHeight);
    schematic::drawSymbol(painter,
                          QString::fromStdString(component.typeName()),
                          localBounds);
    painter.restore();

    painter.setPen(QColor("#263238"));
    QFont labelFont = painter.font();
    labelFont.setBold(true);
    labelFont.setPointSizeF(8.0);
    painter.setFont(labelFont);
    painter.drawText(QPointF(bounds.left(), bounds.bottom() + 14.0),
                     QString::fromStdString(component.label()));
    for (std::size_t index = 0; index < component.pinCount(); ++index) {
        const QPointF position = toQPoint(component.pinPosition(index));
        painter.setPen(QPen(QColor("#263238"), 1.0));
        painter.setBrush(pendingWire_.has_value() &&
                                 pendingWire_->componentId == component.id() &&
                                 pendingWire_->pinIndex == index
                             ? QColor("#ff9800")
                             : QColor("#d32f2f"));
        painter.drawEllipse(position, 4.5, 4.5);
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent* event) {
    setFocus();
    lastViewPoint_ = event->position();
    if (event->button() == Qt::MiddleButton) {
        panning_ = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const proteus::Point scene = viewport_.sceneFromView(toPoint(event->position()));
    if (!placementType_.empty()) {
        if (!editingAllowed("place components")) {
            clearPlacementType();
            return;
        }
        const proteus::Point position = viewport_.snap(scene);
        catalog_.create(document_.circuit, placementType_,
                        nextLabel(placementType_).toStdString(), position);
        recordChange();
        return;
    }
    if (const auto pin = pinAt(scene); pin.has_value()) {
        if (!editingAllowed("draw wires")) {
            pendingWire_.reset();
            return;
        }
        selectedWire_.reset();
        if (!pendingWire_.has_value()) {
            pendingWire_ = pin;
            emit statusChanged("Select a second pin to complete the wire");
        } else if (*pendingWire_ != *pin) {
            try {
                document_.circuit.connect(*pendingWire_, *pin);
                pendingWire_.reset();
                recordChange();
            } catch (const std::exception& error) {
                emit logMessage(QString::fromUtf8(error.what()));
            }
        }
        update();
        return;
    }
    if (const auto id = componentAt(scene); id.has_value()) {
        if (simulation_->state() != proteus::SimulationState::Stopped) {
            auto& component = document_.circuit.component(*id);
            if (auto* value = dynamic_cast<proteus::ToggleSwitch*>(&component)) {
                value->toggle();
                modified_ = true;
                if (simulation_->state() == proteus::SimulationState::Running) {
                    simulation_->advance(0.0);
                }
                update();
                return;
            }
            if (auto* value = dynamic_cast<proteus::PushButton*>(&component)) {
                value->press();
                pressedButton_ = *id;
                if (simulation_->state() == proteus::SimulationState::Running) {
                    simulation_->advance(0.0);
                }
                update();
                return;
            }
            editingAllowed("move or select components");
            return;
        }
        selectedWire_.reset();
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            selection_.toggle(*id);
        } else if (!selection_.contains(*id)) {
            selection_.selectOnly(*id);
        }
        draggedComponent_ = id;
        componentMoved_ = false;
        update();
        return;
    }
    if (const auto wire = wireAt(scene); wire.has_value()) {
        if (!editingAllowed("select wires")) {
            return;
        }
        selection_.clear();
        selectedWire_ = wire;
        update();
        return;
    }
    if (!editingAllowed("change the selection")) {
        return;
    }
    selection_.clear();
    selectedWire_.reset();
    selecting_ = true;
    selectionStart_ = event->position();
    selectionCurrent_ = event->position();
    update();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    viewport_.setMouseViewPosition(toPoint(event->position()));
    const auto mouse = viewport_.mouseScenePosition();
    emit statusChanged(QString("X: %1   Y: %2   Zoom: %3%")
                           .arg(mouse.x, 0, 'f', 1)
                           .arg(mouse.y, 0, 'f', 1)
                           .arg(viewport_.zoom() * 100.0, 0, 'f', 0));
    if (panning_) {
        const QPointF delta = event->position() - lastViewPoint_;
        viewport_.panBy(toPoint(delta));
        lastViewPoint_ = event->position();
        update();
        return;
    }
    if (draggedComponent_.has_value()) {
        if (!editingAllowed("move components")) {
            draggedComponent_.reset();
            return;
        }
        const proteus::Point scene = viewport_.snap(
            viewport_.sceneFromView(toPoint(event->position())));
        document_.circuit.moveComponent(*draggedComponent_, scene);
        componentMoved_ = true;
        update();
        return;
    }
    if (selecting_) {
        selectionCurrent_ = event->position();
        update();
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        panning_ = false;
        unsetCursor();
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (pressedButton_.has_value()) {
        if (auto* value = dynamic_cast<proteus::PushButton*>(
                &document_.circuit.component(*pressedButton_))) {
            value->release();
            if (simulation_->state() == proteus::SimulationState::Running) {
                simulation_->advance(0.0);
            }
        }
        pressedButton_.reset();
        update();
        return;
    }
    if (draggedComponent_.has_value()) {
        draggedComponent_.reset();
        if (componentMoved_) {
            recordChange();
        }
    }
    if (selecting_) {
        const QRectF viewRect(selectionStart_, selectionCurrent_);
        const QRectF normalized = viewRect.normalized();
        const auto topLeft = viewport_.sceneFromView(toPoint(normalized.topLeft()));
        const auto bottomRight = viewport_.sceneFromView(toPoint(normalized.bottomRight()));
        const QRectF sceneRect(toQPoint(topLeft), toQPoint(bottomRight));
        for (const auto id : document_.circuit.componentIds()) {
            if (sceneRect.intersects(componentRect(document_.circuit.component(id)))) {
                selection_.add(id);
            }
        }
        selecting_ = false;
        update();
    }
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    const auto scene = viewport_.sceneFromView(toPoint(event->position()));
    const auto id = componentAt(scene);
    if (!id.has_value()) {
        return;
    }
    if (simulation_->state() != proteus::SimulationState::Stopped) {
        editingAllowed("edit component properties");
        return;
    }
    editComponent(*id);
}

void CanvasWidget::wheelEvent(QWheelEvent* event) {
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    viewport_.zoomAt(factor, toPoint(event->position()));
    update();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        removeSelection();
    } else if (event->key() == Qt::Key_R) {
        rotateSelection();
    } else if (event->key() == Qt::Key_H) {
        mirrorSelection(true);
    } else if (event->key() == Qt::Key_V) {
        mirrorSelection(false);
    } else if (event->key() == Qt::Key_J) {
        if (!editingAllowed("add wire junctions")) {
            return;
        }
        std::vector<proteus::WireId> wires;
        const auto position = viewport_.snap(viewport_.mouseScenePosition());
        for (const auto wireId : document_.circuit.wireIds()) {
            if (document_.circuit.wire(wireId).containsPoint(position, 5.0)) {
                wires.push_back(wireId);
            }
        }
        if (wires.size() >= 2) {
            try {
                document_.circuit.addJunction(position, wires);
                recordChange();
            } catch (const std::exception& error) {
                emit logMessage(QString::fromUtf8(error.what()));
            }
        } else {
            emit logMessage("Move the pointer to a wire crossing and press J.");
        }
    } else if (event->key() == Qt::Key_Escape) {
        clearPlacementType();
        pendingWire_.reset();
        selection_.clear();
        selectedWire_.reset();
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void CanvasWidget::writeSimulationLog() {
    for (const auto& message : simulation_->messages()) {
        emit logMessage(QString("[%1] t=%2 s  %3")
                            .arg(QString::fromStdString(message.code))
                            .arg(message.timeSeconds, 0, 'g', 6)
                            .arg(QString::fromStdString(message.text)));
    }
    simulation_->clearMessages();
}
