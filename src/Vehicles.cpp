#include "Vehicles.h"

#include <iostream>

#include "Exceptions.h"

namespace ReliefOps {
namespace Logistics {

std::ostream& operator<<(std::ostream& os, const GPSBeacon& gps) {
    return os << '(' << detail::num(gps.latitude(), 3) << ", " << detail::num(gps.longitude(), 3) << ')';
}

bool EmergencyLifeSupportUnit::consume(int minutes) {
    if (minutes > oxygenMinutes_) { oxygenMinutes_ = 0; return false; }
    oxygenMinutes_ -= minutes;
    return true;
}

// ------------------------------------------------------------ ReliefVehicle
ReliefVehicle::ReliefVehicle(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat,
                             double lon)
    : TransportVehicle(id, maxPayloadKg), gps_(lat, lon), lifeSupport_(), fuelRangeKm_(fuelRangeKm) {}

ReliefVehicle::~ReliefVehicle() { std::cout << "    [~ReliefVehicle]         " << id() << " released\n"; }

void ReliefVehicle::requireFuel(const Route& route) const {
    const double needed = route.distanceKm * fuelBurnFactor();  // virtual call: uses the real vehicle type
    if (needed > fuelRangeKm_) throw FuelDepletionException(id(), needed, fuelRangeKm_);
}

double ReliefVehicle::executeRoute(const Route& route) {
    const double etaHours = evaluateRoute(route);  // virtual: throws if the leg is not feasible
    fuelRangeKm_ -= route.distanceKm * fuelBurnFactor();
    gps_.moveTo(route.destLat, route.destLon);
    lifeSupport_.consume(static_cast<int>(etaHours * 60.0));
    return etaHours;
}

// ------------------------------------------------------------ AerialTransport
AerialTransport::AerialTransport(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat,
                                 double lon, double ceilingMetres)
    : ReliefVehicle(id, maxPayloadKg, fuelRangeKm, lat, lon), ceilingMetres_(ceilingMetres) {}

AerialTransport::~AerialTransport() { std::cout << "    [~AerialTransport]       " << id() << '\n'; }

double AerialTransport::evaluateRoute(const Route& route) const {
    requireFuel(route);  // damaged roads do not matter in the air
    return travelTimeHours(route.distanceKm, cruiseSpeedKmh()) + 0.15;  // + take-off / landing
}

// ------------------------------------------------------------ GroundConvoy
GroundConvoy::GroundConvoy(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat,
                           double lon, int trucks)
    : ReliefVehicle(id, maxPayloadKg, fuelRangeKm, lat, lon), trucks_(trucks) {}

GroundConvoy::~GroundConvoy() { std::cout << "    [~GroundConvoy]          " << id() << '\n'; }

double GroundConvoy::evaluateRoute(const Route& route) const {
    if (route.damaged || route.waterway) throw DamagedCorridorException(route.name, vehicleClass());
    requireFuel(route);
    return travelTimeHours(route.distanceKm, cruiseSpeedKmh());
}

// ------------------------------------------------------------ AmphibiousCraft
AmphibiousCraft::AmphibiousCraft(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat,
                                 double lon)
    : ReliefVehicle(id, maxPayloadKg, fuelRangeKm, lat, lon) {}

AmphibiousCraft::~AmphibiousCraft() { std::cout << "    [~AmphibiousCraft]       " << id() << '\n'; }

double AmphibiousCraft::evaluateRoute(const Route& route) const {
    if (route.damaged && !route.waterway) throw DamagedCorridorException(route.name, vehicleClass());
    requireFuel(route);
    return travelTimeHours(route.distanceKm, route.waterway ? 25.0 : cruiseSpeedKmh());
}

// ------------------------------------------------------------ AmphibiousMedicalUnit (diamond)
// The most-derived class is responsible for constructing the single virtual base.
AmphibiousMedicalUnit::AmphibiousMedicalUnit(const std::string& id, double maxPayloadKg, double fuelRangeKm,
                                             double lat, double lon, int beds)
    : ReliefVehicle(id, maxPayloadKg, fuelRangeKm, lat, lon),
      GroundConvoy(id, maxPayloadKg, fuelRangeKm, lat, lon, 1),
      AmphibiousCraft(id, maxPayloadKg, fuelRangeKm, lat, lon),
      beds_(beds) {}

AmphibiousMedicalUnit::~AmphibiousMedicalUnit() {
    std::cout << "    [~AmphibiousMedicalUnit] " << id() << '\n';
}

double AmphibiousMedicalUnit::evaluateRoute(const Route& route) const {
    // water or damaged land -> behave like the boat; intact land -> behave like the convoy
    if (route.waterway || route.damaged) return AmphibiousCraft::evaluateRoute(route);
    return GroundConvoy::evaluateRoute(route);
}

bool AmphibiousMedicalUnit::treatPatients(int patients, int minutesEach) {
    if (patients > beds_) return false;
    return lifeSupport().consume(patients * minutesEach);
}

}  // namespace Logistics
}  // namespace ReliefOps
