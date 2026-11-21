// Vehicles.h  -  Unit III: inheritance, containership, virtual base classes, runtime polymorphism
//
//                 TransportVehicle (Unit II data holder)
//                        |
//                   ReliefVehicle  (abstract; composes GPSBeacon + EmergencyLifeSupportUnit)
//                  /      |       \     <- all three inherit VIRTUALLY
//        AerialTransport GroundConvoy AmphibiousCraft
//                          \         /
//                     AmphibiousMedicalUnit   (the diamond)
#ifndef RELIEFOPS_VEHICLES_H
#define RELIEFOPS_VEHICLES_H

#include <iosfwd>
#include <string>

#include "Logistics.h"

namespace ReliefOps {
namespace Logistics {

/// A leg of a relief run.
struct Route {
    std::string name;
    double distanceKm;
    bool damaged;   // infrastructure destroyed
    bool waterway;  // river / flooded corridor
    double destLat;
    double destLon;
    Route(const std::string& n, double km, bool isDamaged, bool isWaterway, double lat = 0.0, double lon = 0.0)
        : name(n), distanceKm(km), damaged(isDamaged), waterway(isWaterway), destLat(lat), destLon(lon) {}
};

// ------------------------------------------------------------ contained (composed) parts
class GPSBeacon {
public:
    GPSBeacon(double lat = 0.0, double lon = 0.0) : lat_(lat), lon_(lon) {}
    void moveTo(double lat, double lon) { lat_ = lat; lon_ = lon; }
    double latitude() const { return lat_; }
    double longitude() const { return lon_; }
    double distanceTo(double lat, double lon) const { return geoDistanceKm(lat_, lon_, lat, lon); }

private:
    double lat_, lon_;
};
std::ostream& operator<<(std::ostream& os, const GPSBeacon& gps);

class EmergencyLifeSupportUnit {
public:
    explicit EmergencyLifeSupportUnit(int oxygenMinutes = 240) : oxygenMinutes_(oxygenMinutes) {}
    /// Uses oxygen; returns false (and drains the tank) if the supply ran out.
    bool consume(int minutes);
    int oxygenMinutes() const { return oxygenMinutes_; }

private:
    int oxygenMinutes_;
};

// ------------------------------------------------------------ abstract base
class ReliefVehicle : public TransportVehicle {
public:
    ReliefVehicle(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat, double lon);
    ~ReliefVehicle() override;
    ReliefVehicle(const ReliefVehicle&) = delete;
    ReliefVehicle& operator=(const ReliefVehicle&) = delete;

    // pure virtual interface (runtime polymorphism)
    virtual std::string vehicleClass() const = 0;
    virtual double cruiseSpeedKmh() const = 0;
    /// Returns the ETA in hours or throws DamagedCorridorException / FuelDepletionException.
    virtual double evaluateRoute(const Route& route) const = 0;
    virtual double fuelBurnFactor() const { return 1.0; }

    /// Template-method style: validates via the virtual evaluateRoute(), then moves the vehicle.
    double executeRoute(const Route& route);

    double fuelRangeKm() const { return fuelRangeKm_; }
    const GPSBeacon& gps() const { return gps_; }
    EmergencyLifeSupportUnit& lifeSupport() { return lifeSupport_; }

protected:
    void requireFuel(const Route& route) const;

private:
    GPSBeacon gps_;                         // containership (HAS-A), not inheritance
    EmergencyLifeSupportUnit lifeSupport_;  // containership (HAS-A)
    double fuelRangeKm_;
};

// ------------------------------------------------------------ intermediate classes (virtual inheritance)
class AerialTransport : virtual public ReliefVehicle {
public:
    AerialTransport(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat, double lon,
                    double ceilingMetres = 4500.0);
    ~AerialTransport() override;
    std::string vehicleClass() const override { return "AerialTransport"; }
    double cruiseSpeedKmh() const override { return 220.0; }
    double fuelBurnFactor() const override { return 1.6; }
    double evaluateRoute(const Route& route) const override;
    double ceilingMetres() const { return ceilingMetres_; }

private:
    double ceilingMetres_;
};

class GroundConvoy : virtual public ReliefVehicle {
public:
    GroundConvoy(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat, double lon,
                 int trucks = 3);
    ~GroundConvoy() override;
    std::string vehicleClass() const override { return "GroundConvoy"; }
    double cruiseSpeedKmh() const override { return 45.0; }
    double fuelBurnFactor() const override { return 1.0; }
    double evaluateRoute(const Route& route) const override;
    int convoyTrucks() const { return trucks_; }

private:
    int trucks_;
};

class AmphibiousCraft : virtual public ReliefVehicle {
public:
    AmphibiousCraft(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat, double lon);
    ~AmphibiousCraft() override;
    std::string vehicleClass() const override { return "AmphibiousCraft"; }
    double cruiseSpeedKmh() const override { return 30.0; }
    double fuelBurnFactor() const override { return 1.3; }
    double evaluateRoute(const Route& route) const override;
};

// ------------------------------------------------------------ the diamond
/// Amphibious mobile medical unit: IS-A GroundConvoy and IS-A AmphibiousCraft.
/// Because both inherit ReliefVehicle *virtually*, there is exactly one ReliefVehicle
/// (and one TransportVehicle manifest) inside every AmphibiousMedicalUnit.
class AmphibiousMedicalUnit : public GroundConvoy, public AmphibiousCraft {
public:
    AmphibiousMedicalUnit(const std::string& id, double maxPayloadKg, double fuelRangeKm, double lat,
                          double lon, int beds);
    ~AmphibiousMedicalUnit() override;

    // Both parents override these, so the hybrid must resolve the ambiguity itself.
    std::string vehicleClass() const override { return "AmphibiousMedicalUnit"; }
    double cruiseSpeedKmh() const override { return 35.0; }
    double fuelBurnFactor() const override { return 1.4; }
    double evaluateRoute(const Route& route) const override;

    /// Treats patients on board; returns false if life support ran out.
    bool treatPatients(int patients, int minutesEach);
    int beds() const { return beds_; }

private:
    int beds_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // RELIEFOPS_VEHICLES_H
