#include "proteus/ui/CanvasModel.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace proteus {

CanvasViewport::CanvasViewport(CanvasSettings settings) {
    setSettings(settings);
}

const CanvasSettings& CanvasViewport::settings() const noexcept {
    return settings_;
}

void CanvasViewport::setSettings(CanvasSettings settings) {
    if (!std::isfinite(settings.width) || !std::isfinite(settings.height) ||
        !std::isfinite(settings.gridSpacing) ||
        settings.width <= 0.0 || settings.height <= 0.0 ||
        settings.gridSpacing <= 0.0) {
        throw std::invalid_argument(
            "Canvas dimensions and grid spacing must be finite and positive");
    }
    settings_ = settings;
}

void CanvasViewport::setPreset(CanvasPreset preset) {
    settings_.preset = preset;
    switch (preset) {
        case CanvasPreset::A4:
            settings_.width = 1600.0;
            settings_.height = 1000.0;
            break;
        case CanvasPreset::A3:
            settings_.width = 2200.0;
            settings_.height = 1400.0;
            break;
        case CanvasPreset::A2:
            settings_.width = 3000.0;
            settings_.height = 2000.0;
            break;
        case CanvasPreset::Custom:
            break;
    }
}

double CanvasViewport::zoom() const noexcept { return zoom_; }
Point CanvasViewport::pan() const noexcept { return pan_; }
Point CanvasViewport::mouseScenePosition() const noexcept {
    return mouseScenePosition_;
}

Point CanvasViewport::sceneFromView(Point viewPoint) const noexcept {
    return {(viewPoint.x - pan_.x) / zoom_,
            (viewPoint.y - pan_.y) / zoom_};
}

Point CanvasViewport::viewFromScene(Point scenePoint) const noexcept {
    return {scenePoint.x * zoom_ + pan_.x,
            scenePoint.y * zoom_ + pan_.y};
}

Point CanvasViewport::snap(Point scenePoint) const noexcept {
    if (!settings_.snapEnabled) {
        return scenePoint;
    }
    const double spacing = settings_.gridSpacing;
    return {std::round(scenePoint.x / spacing) * spacing,
            std::round(scenePoint.y / spacing) * spacing};
}

void CanvasViewport::setMouseViewPosition(Point viewPoint) noexcept {
    mouseScenePosition_ = sceneFromView(viewPoint);
}

void CanvasViewport::zoomAt(double factor, Point viewAnchor) noexcept {
    const Point sceneAnchor = sceneFromView(viewAnchor);
    zoom_ = std::clamp(zoom_ * factor, 0.2, 5.0);
    pan_.x = viewAnchor.x - sceneAnchor.x * zoom_;
    pan_.y = viewAnchor.y - sceneAnchor.y * zoom_;
    mouseScenePosition_ = sceneFromView(viewAnchor);
}

void CanvasViewport::panBy(Point viewDelta) noexcept {
    pan_.x += viewDelta.x;
    pan_.y += viewDelta.y;
}

void CanvasViewport::resetView() noexcept {
    zoom_ = 1.0;
    pan_ = {};
    mouseScenePosition_ = {};
}

bool SelectionModel::contains(ComponentId id) const noexcept {
    return selected_.contains(id);
}

const std::set<ComponentId>& SelectionModel::items() const noexcept {
    return selected_;
}

void SelectionModel::selectOnly(ComponentId id) {
    selected_.clear();
    selected_.insert(id);
}

void SelectionModel::add(ComponentId id) { selected_.insert(id); }

void SelectionModel::toggle(ComponentId id) {
    if (selected_.contains(id)) {
        selected_.erase(id);
    } else {
        selected_.insert(id);
    }
}

void SelectionModel::remove(ComponentId id) { selected_.erase(id); }
void SelectionModel::clear() noexcept { selected_.clear(); }

std::string canvasPresetName(CanvasPreset preset) {
    switch (preset) {
        case CanvasPreset::A4: return "A4";
        case CanvasPreset::A3: return "A3";
        case CanvasPreset::A2: return "A2";
        case CanvasPreset::Custom: return "Custom";
    }
    return "Custom";
}

}
