#pragma once

#include "proteus/core/Types.hpp"

#include <set>
#include <string>

namespace proteus {

enum class CanvasPreset {
    A4,
    A3,
    A2,
    Custom
};

struct CanvasSettings {
    double width{1600.0};
    double height{1000.0};
    double gridSpacing{20.0};
    bool gridVisible{true};
    bool snapEnabled{true};
    CanvasPreset preset{CanvasPreset::A4};
};

class CanvasViewport {
public:
    explicit CanvasViewport(CanvasSettings settings = {});

    const CanvasSettings& settings() const noexcept;
    void setSettings(CanvasSettings settings);
    void setPreset(CanvasPreset preset);
    double zoom() const noexcept;
    Point pan() const noexcept;
    Point mouseScenePosition() const noexcept;
    Point sceneFromView(Point viewPoint) const noexcept;
    Point viewFromScene(Point scenePoint) const noexcept;
    Point snap(Point scenePoint) const noexcept;
    void setMouseViewPosition(Point viewPoint) noexcept;
    void zoomAt(double factor, Point viewAnchor) noexcept;
    void panBy(Point viewDelta) noexcept;
    void resetView() noexcept;

private:
    CanvasSettings settings_;
    double zoom_{1.0};
    Point pan_{};
    Point mouseScenePosition_{};
};

class SelectionModel {
public:
    bool contains(ComponentId id) const noexcept;
    const std::set<ComponentId>& items() const noexcept;
    void selectOnly(ComponentId id);
    void add(ComponentId id);
    void toggle(ComponentId id);
    void remove(ComponentId id);
    void clear() noexcept;

private:
    std::set<ComponentId> selected_;
};

std::string canvasPresetName(CanvasPreset preset);

}
