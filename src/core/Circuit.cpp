#include "proteus/core/Circuit.hpp"

#include "proteus/wiring/OrthogonalRouter.hpp"

#include <algorithm>
#include <numeric>
#include <set>
#include <unordered_set>

namespace proteus {
namespace {

class DisjointSet {
public:
    explicit DisjointSet(std::size_t size) : parent_(size), rank_(size, 0) {
        std::iota(parent_.begin(), parent_.end(), 0);
    }

    std::size_t find(std::size_t item) {
        if (parent_[item] != item) {
            parent_[item] = find(parent_[item]);
        }
        return parent_[item];
    }

    void unite(std::size_t first, std::size_t second) {
        first = find(first);
        second = find(second);
        if (first == second) {
            return;
        }
        if (rank_[first] < rank_[second]) {
            std::swap(first, second);
        }
        parent_[second] = first;
        if (rank_[first] == rank_[second]) {
            ++rank_[first];
        }
    }

private:
    std::vector<std::size_t> parent_;
    std::vector<unsigned int> rank_;
};

}

Component& Circuit::component(ComponentId id) {
    const auto iterator = components_.find(id);
    if (iterator == components_.end()) {
        throw std::out_of_range("Component id does not exist in the circuit");
    }
    return *iterator->second;
}

const Component& Circuit::component(ComponentId id) const {
    const auto iterator = components_.find(id);
    if (iterator == components_.end()) {
        throw std::out_of_range("Component id does not exist in the circuit");
    }
    return *iterator->second;
}

Pin& Circuit::pin(PinRef reference) {
    validatePinRef(reference);
    return component(reference.componentId).pin(reference.pinIndex);
}

const Pin& Circuit::pin(PinRef reference) const {
    validatePinRef(reference);
    return component(reference.componentId).pin(reference.pinIndex);
}

std::vector<ComponentId> Circuit::componentIds() const {
    std::vector<ComponentId> ids;
    ids.reserve(components_.size());
    for (const auto& [id, ignored] : components_) {
        (void)ignored;
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::vector<PinRef> Circuit::allPins() const {
    std::vector<PinRef> references;
    for (const ComponentId id : componentIds()) {
        const auto& current = component(id);
        for (std::size_t index = 0; index < current.pinCount(); ++index) {
            references.push_back({id, index});
        }
    }
    return references;
}

void Circuit::removeComponent(ComponentId id) {
    if (!components_.contains(id)) {
        throw std::out_of_range("Component id does not exist in the circuit");
    }
    const auto attachedWires = wiresForComponent(id);
    for (const WireId wireId : attachedWires) {
        removeWire(wireId);
    }
    components_.erase(id);
    rebuildNets();
}

void Circuit::moveComponent(ComponentId id, Point newPosition) {
    component(id).moveTo(newPosition);
    refreshWireRoutes(id);
}

Wire& Circuit::connect(PinRef first, PinRef second) {
    validatePinRef(first);
    validatePinRef(second);
    if (first == second) {
        throw std::invalid_argument("A pin cannot be wired to itself");
    }
    for (const auto& [ignored, existing] : wires_) {
        (void)ignored;
        const bool sameDirection =
            existing.first() == first && existing.second() == second;
        const bool reverseDirection =
            existing.first() == second && existing.second() == first;
        if (sameDirection || reverseDirection) {
            throw std::invalid_argument("The selected pins are already connected");
        }
    }

    const WireId id = nextWireId_++;
    const Point start = component(first.componentId).pinPosition(first.pinIndex);
    const Point end = component(second.componentId).pinPosition(second.pinIndex);
    auto [iterator, inserted] = wires_.emplace(
        id,
        Wire{id, first, second, OrthogonalRouter::route(start, end)});
    (void)inserted;
    rebuildNets();
    return iterator->second;
}

void Circuit::removeWire(WireId id) {
    if (wires_.erase(id) == 0) {
        throw std::out_of_range("Wire id does not exist in the circuit");
    }
    for (auto iterator = junctions_.begin(); iterator != junctions_.end();) {
        auto& connected = iterator->second.connectedWires;
        std::erase(connected, id);
        if (connected.size() < 2) {
            iterator = junctions_.erase(iterator);
        } else {
            ++iterator;
        }
    }
    rebuildNets();
}

Wire& Circuit::wire(WireId id) {
    const auto iterator = wires_.find(id);
    if (iterator == wires_.end()) {
        throw std::out_of_range("Wire id does not exist in the circuit");
    }
    return iterator->second;
}

const Wire& Circuit::wire(WireId id) const {
    const auto iterator = wires_.find(id);
    if (iterator == wires_.end()) {
        throw std::out_of_range("Wire id does not exist in the circuit");
    }
    return iterator->second;
}

std::vector<WireId> Circuit::wireIds() const {
    std::vector<WireId> ids;
    ids.reserve(wires_.size());
    for (const auto& [id, ignored] : wires_) {
        (void)ignored;
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::vector<WireId> Circuit::wiresForComponent(ComponentId id) const {
    std::vector<WireId> result;
    for (const auto& [wireId, currentWire] : wires_) {
        if (currentWire.first().componentId == id ||
            currentWire.second().componentId == id) {
            result.push_back(wireId);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

Junction& Circuit::addJunction(
    Point position,
    const std::vector<WireId>& connectedWires) {
    std::set<WireId> uniqueWires(connectedWires.begin(), connectedWires.end());
    if (uniqueWires.size() < 2) {
        throw std::invalid_argument("A junction must connect at least two wires");
    }
    for (const WireId id : uniqueWires) {
        if (!wires_.contains(id)) {
            throw std::out_of_range("Junction references a missing wire");
        }
        if (!wire(id).containsPoint(position)) {
            throw std::invalid_argument(
                "Junction position must lie on every connected wire");
        }
    }
    for (const auto& [ignored, existing] : junctions_) {
        (void)ignored;
        const std::set<WireId> existingWires(existing.connectedWires.begin(),
                                             existing.connectedWires.end());
        if (existing.position.approximatelyEquals(position) &&
            existingWires == uniqueWires) {
            throw std::invalid_argument("This wire junction already exists");
        }
    }
    const JunctionId id = nextJunctionId_++;
    auto [iterator, inserted] = junctions_.emplace(
        id,
        Junction{id, position,
                 std::vector<WireId>(uniqueWires.begin(), uniqueWires.end())});
    (void)inserted;
    rebuildNets();
    return iterator->second;
}

void Circuit::removeJunction(JunctionId id) {
    if (junctions_.erase(id) == 0) {
        throw std::out_of_range("Junction id does not exist in the circuit");
    }
    rebuildNets();
}

std::vector<JunctionId> Circuit::junctionIds() const {
    std::vector<JunctionId> ids;
    ids.reserve(junctions_.size());
    for (const auto& [id, ignored] : junctions_) {
        (void)ignored;
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

const Junction& Circuit::junction(JunctionId id) const {
    const auto iterator = junctions_.find(id);
    if (iterator == junctions_.end()) {
        throw std::out_of_range("Junction id does not exist in the circuit");
    }
    return iterator->second;
}

void Circuit::refreshWireRoutes(ComponentId movedComponent) {
    for (const WireId id : wiresForComponent(movedComponent)) {
        auto& current = wire(id);
        const Point start = component(current.first().componentId)
                                .pinPosition(current.first().pinIndex);
        const Point end = component(current.second().componentId)
                              .pinPosition(current.second().pinIndex);
        current.setRoute(OrthogonalRouter::route(start, end));
    }
    removeInvalidJunctions();
    rebuildNets();
}

void Circuit::removeInvalidJunctions() {
    for (auto iterator = junctions_.begin(); iterator != junctions_.end();) {
        const auto& current = iterator->second;
        const bool remainsValid = std::all_of(
            current.connectedWires.begin(),
            current.connectedWires.end(),
            [this, &current](WireId wireId) {
                const auto wireIterator = wires_.find(wireId);
                return wireIterator != wires_.end() &&
                       wireIterator->second.containsPoint(current.position);
            });
        if (!remainsValid) {
            iterator = junctions_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void Circuit::rebuildNets() {
    const auto references = allPins();
    if (references.empty()) {
        netCount_ = 0;
        return;
    }

    std::unordered_map<PinRef, std::size_t, PinRefHash> pinToIndex;
    for (std::size_t index = 0; index < references.size(); ++index) {
        pinToIndex.emplace(references[index], index);
    }
    DisjointSet sets(references.size());
    for (const auto& [ignored, currentWire] : wires_) {
        (void)ignored;
        sets.unite(pinToIndex.at(currentWire.first()),
                   pinToIndex.at(currentWire.second()));
    }
    for (const auto& [ignored, currentJunction] : junctions_) {
        (void)ignored;
        if (currentJunction.connectedWires.empty()) {
            continue;
        }
        const auto& firstWire = wire(currentJunction.connectedWires.front());
        const std::size_t representative = pinToIndex.at(firstWire.first());
        for (const WireId id : currentJunction.connectedWires) {
            const auto& connectedWire = wire(id);
            sets.unite(representative, pinToIndex.at(connectedWire.first()));
            sets.unite(representative, pinToIndex.at(connectedWire.second()));
        }
    }

    std::unordered_map<std::size_t, NetId> rootToNet;
    NetId nextNet = 0;
    for (std::size_t index = 0; index < references.size(); ++index) {
        const std::size_t root = sets.find(index);
        const auto [iterator, inserted] = rootToNet.emplace(root, nextNet);
        if (inserted) {
            ++nextNet;
        }
        pin(references[index]).setNetId(iterator->second);
    }
    netCount_ = static_cast<std::size_t>(nextNet);
}

std::size_t Circuit::netCount() const noexcept { return netCount_; }

void Circuit::validatePinRef(PinRef reference) const {
    const auto componentIterator = components_.find(reference.componentId);
    if (componentIterator == components_.end()) {
        throw std::out_of_range("Pin reference uses a missing component id");
    }
    if (reference.pinIndex >= componentIterator->second->pinCount()) {
        throw std::out_of_range("Pin reference uses a missing pin index");
    }
}

}
