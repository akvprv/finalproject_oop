#pragma once

#include "proteus/history/History.hpp"
#include "proteus/library/ComponentCatalog.hpp"
#include "proteus/persistence/ProjectSerializer.hpp"
#include "proteus/simulation/SimulationEngine.hpp"
#include "proteus/ui/CanvasModel.hpp"

#include <QPointF>
#include <QRectF>
#include <QTimer>
#include <QWidget>

#include <map>
#include <memory>
#include <optional>
#include <string>

class CanvasWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CanvasWidget(QWidget* parent = nullptr);

    const proteus::ProjectDocument& document() const noexcept;
    void newDocument(proteus::CanvasSettings settings);
    void loadDocument(proteus::ProjectDocument document);
    void setPlacementType(const std::string& typeName);
    void clearPlacementType();
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;
    bool isModified() const noexcept;
    void markSaved() noexcept;
    bool removeComponent(proteus::ComponentId id);
    void undo();
    void redo();
    void runSimulation();
    void pauseSimulation();
    void stopSimulation();
    void stepSimulation();
    void runDesignRuleCheck();
    bool exportPng(const QString& path);

signals:
    void statusChanged(const QString& text);
    void logMessage(const QString& text);
    void circuitChanged();
    void historyChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QPointF toQPoint(proteus::Point point) const;
    proteus::Point toPoint(QPointF point) const;
    QRectF componentRect(const proteus::Component& component) const;
    std::optional<proteus::ComponentId> componentAt(proteus::Point point) const;
    std::optional<proteus::PinRef> pinAt(proteus::Point point) const;
    std::optional<proteus::WireId> wireAt(proteus::Point point) const;
    QString nextLabel(const std::string& typeName);
    void rebuildLabelCounters();
    void rebuildSimulation();
    void resetInteractionState();
    bool editingAllowed(const QString& action);
    void recordChange();
    void removeSelection();
    void rotateSelection();
    void mirrorSelection(bool horizontal);
    void editComponent(proteus::ComponentId id);
    void drawGrid(QPainter& painter);
    void drawCircuit(QPainter& painter);
    void drawComponent(QPainter& painter, const proteus::Component& component);
    void writeSimulationLog();

    proteus::ProjectDocument document_;
    proteus::CanvasViewport viewport_;
    proteus::ComponentCatalog catalog_;
    proteus::History history_;
    std::unique_ptr<proteus::SimulationEngine> simulation_;
    proteus::SelectionModel selection_;
    std::string placementType_;
    std::optional<proteus::PinRef> pendingWire_;
    std::optional<proteus::ComponentId> draggedComponent_;
    std::optional<proteus::ComponentId> pressedButton_;
    std::optional<proteus::WireId> selectedWire_;
    std::map<std::string, int> labelCounters_;
    QPointF lastViewPoint_;
    QPointF selectionStart_;
    QPointF selectionCurrent_;
    bool panning_{false};
    bool selecting_{false};
    bool componentMoved_{false};
    bool modified_{false};
    QTimer simulationTimer_;
};
