// Logistics.h  -  namespace ReliefOps::Logistics
// Unit I : namespaces, inline utilities, function overloading, call/return by reference, friend class
// Unit II: TransportVehicle (dynamic manifest, rule of three, static analytics, operators)
#ifndef RELIEFOPS_LOGISTICS_H
#define RELIEFOPS_LOGISTICS_H

#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "Inventory.h"

namespace ReliefOps {

class IncidentCommander;  // friend class (declared in IncidentCommander.h)

namespace Logistics {

// ------------------------------------------------------ inline utilities (Unit I)
inline double payloadKg(int units, double unitKg) { return units * unitKg; }
inline double travelTimeHours(double km, double kmh) { return kmh > 0.0 ? km / kmh : 0.0; }
inline double geoDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    const double kmPerDegree = 111.0;
    const double meanLat = (lat1 + lat2) * 0.5 * 3.14159265358979323846 / 180.0;
    const double dy = (lat2 - lat1) * kmPerDegree;
    const double dx = (lon2 - lon1) * kmPerDegree * std::cos(meanLat);
    return std::sqrt(dx * dx + dy * dy);
}

// ------------------------------------------------------ mission parameters (Unit I)
struct MissionParams {
    std::string destination;
    double routeKm;
    int priority;  // 1..5
    double payloadKg;
};

/// Call by reference: the caller's mission is modified in place.
void escalatePriority(MissionParams& mission, int by);
/// Return by reference: returns the *original* more urgent mission, not a copy.
MissionParams& moreUrgent(MissionParams& a, MissionParams& b);

// ------------------------------------------------------ dispatch requests + functors (Unit IV)
struct DispatchRequest {
    int id;
    std::string destination;
    Inventory::SupplyType type;
    int quantity;
    int priority;  // 1..5
    bool isMedical() const { return type == Inventory::SupplyType::TraumaMedicine; }
};

/// Strict-weak "is less urgent than": medical needs outrank everything, then priority, then FIFO.
struct UrgencyLess {
    bool operator()(const DispatchRequest& a, const DispatchRequest& b) const {
        if (a.isMedical() != b.isMedical()) return !a.isMedical();
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.id > b.id;
    }
};

/// Functor for std::sort: most urgent request first.
struct MostUrgentFirst {
    bool operator()(const DispatchRequest& a, const DispatchRequest& b) const { return UrgencyLess()(b, a); }
};

// ------------------------------------------------------ TransportVehicle (Unit II)
class TransportVehicle {
public:
    TransportVehicle();
    /// Parameterised + "dynamic" constructor: allocates the manifest array on the heap.
    TransportVehicle(const std::string& id, double maxPayloadKg, std::size_t initialCapacity = 4);
    TransportVehicle(const TransportVehicle& other);             // deep copy
    TransportVehicle& operator=(const TransportVehicle& other);  // deep assignment
    virtual ~TransportVehicle();

    // loading / unloading
    void load(const Inventory::ReliefCargo& cargo);  // throws Duplicate / Overweight exceptions
    double dispatch();                               // departs: adds tonnage to statistics, clears manifest
    void clearManifest() { size_ = 0; }

    // const-correct queries
    const std::string& id() const { return id_; }
    double maxPayloadKg() const { return maxPayloadKg_; }
    double currentLoadKg() const;
    double remainingCapacityKg() const { return maxPayloadKg_ - currentLoadKg(); }
    std::size_t manifestSize() const { return size_; }
    const Inventory::ReliefCargo& cargoAt(std::size_t index) const;
    bool containsBatch(const Inventory::ReliefCargo& cargo) const;
    std::vector<Inventory::ReliefCargo> manifestSnapshot() const;

    /// Manifest identity: same number of batches and every batch present in the other manifest.
    bool operator==(const TransportVehicle& other) const;
    bool operator!=(const TransportVehicle& other) const { return !(*this == other); }

    // static logistics analytics
    static double totalDispatchedTonnes() { return totalDispatchedTonnes_; }
    static int operationalVehicles() { return operationalVehicles_; }

private:
    void grow();

    std::string id_;
    double maxPayloadKg_;
    Inventory::ReliefCargo* manifest_;  // dynamic array
    std::size_t size_;
    std::size_t capacity_;

    static double totalDispatchedTonnes_;
    static int operationalVehicles_;

    friend class ReliefOps::IncidentCommander;
    friend std::ostream& operator<<(std::ostream& os, const TransportVehicle& v);
    friend std::istream& operator>>(std::istream& is, TransportVehicle& v);
};

std::ostream& operator<<(std::ostream& os, const TransportVehicle& v);
std::istream& operator>>(std::istream& is, TransportVehicle& v);

// ------------------------------------------------------ overloaded allocation routines (Unit I)
/// Volume-based payload: load up to `volumeKg` of one supply type.  Returns kg actually loaded.
double allocateSupplies(Inventory::DistributionDepot& depot, TransportVehicle& vehicle,
                        Inventory::SupplyType type, double volumeKg);

/// Priority emergency medical triage (level 1..5): loads trauma medicine.  Returns units loaded.
int allocateSupplies(Inventory::DistributionDepot& depot, TransportVehicle& vehicle, int triageLevel);

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // RELIEFOPS_LOGISTICS_H
