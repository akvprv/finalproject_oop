#include "proteus/components/Passive.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace proteus {

Resistor::Resistor(ComponentId id,
                   std::string label,
                   double resistanceOhms,
                   Point position)
    : Component(id, std::move(label), position),
      resistanceOhms_(resistanceOhms) {
    setResistanceOhms(resistanceOhms);
    addPin("A", PinDirection::Passive, {-20.0, 0.0});
    addPin("B", PinDirection::Passive, {20.0, 0.0});
}
std::string Resistor::typeName() const { return "Resistor"; }
double Resistor::resistanceOhms() const noexcept { return resistanceOhms_; }
void Resistor::setResistanceOhms(double resistance) {
    if (!std::isfinite(resistance) || resistance <= 0.0) {
        throw std::invalid_argument("Resistance must be finite and positive");
    }
    resistanceOhms_ = resistance;
}

Capacitor::Capacitor(ComponentId id,
                     std::string label,
                     double capacitanceFarads,
                     Point position)
    : Component(id, std::move(label), position),
      capacitanceFarads_(capacitanceFarads) {
    setCapacitanceFarads(capacitanceFarads);
    addPin("A", PinDirection::Passive, {-20.0, 0.0});
    addPin("B", PinDirection::Passive, {20.0, 0.0});
}
std::string Capacitor::typeName() const { return "Capacitor"; }
double Capacitor::capacitanceFarads() const noexcept {
    return capacitanceFarads_;
}
void Capacitor::setCapacitanceFarads(double capacitance) {
    if (!std::isfinite(capacitance) || capacitance <= 0.0) {
        throw std::invalid_argument("Capacitance must be finite and positive");
    }
    capacitanceFarads_ = capacitance;
}

Inductor::Inductor(ComponentId id,
                   std::string label,
                   double inductanceHenrys,
                   Point position)
    : Component(id, std::move(label), position),
      inductanceHenrys_(inductanceHenrys) {
    setInductanceHenrys(inductanceHenrys);
    addPin("A", PinDirection::Passive, {-20.0, 0.0});
    addPin("B", PinDirection::Passive, {20.0, 0.0});
}
std::string Inductor::typeName() const { return "Inductor"; }
double Inductor::inductanceHenrys() const noexcept {
    return inductanceHenrys_;
}
void Inductor::setInductanceHenrys(double inductance) {
    if (!std::isfinite(inductance) || inductance <= 0.0) {
        throw std::invalid_argument("Inductance must be finite and positive");
    }
    inductanceHenrys_ = inductance;
}

}
