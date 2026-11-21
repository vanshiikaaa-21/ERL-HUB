// IncidentCommander.h  -  Unit I: friend class that overrides access barriers during critical triage.
// IncidentCommander is declared a friend of ReliefCargo, DistributionDepot and TransportVehicle,
// so it may touch their private members directly (no getters/setters needed).
#ifndef RELIEFOPS_INCIDENT_COMMANDER_H
#define RELIEFOPS_INCIDENT_COMMANDER_H

#include <iosfwd>
#include <string>

#include "Inventory.h"
#include "Logistics.h"

namespace ReliefOps {

class IncidentCommander {
public:
    explicit IncidentCommander(const std::string& name) : name_(name) {}
    const std::string& name() const { return name_; }

    /// Loads cargo onto a vehicle even if it exceeds the payload limit (emergency override).
    void overrideLoad(Logistics::TransportVehicle& vehicle, const Inventory::ReliefCargo& cargo) const;

    /// Takes up to `quantity` units straight out of a depot's private stock. Returns units taken.
    int commandeerStock(Inventory::DistributionDepot& depot, Inventory::SupplyType type, int quantity) const;

    /// Prints the raw private records of a depot.
    void auditDepot(const Inventory::DistributionDepot& depot, std::ostream& os) const;

    static int overridesIssued() { return overridesIssued_; }

private:
    std::string name_;
    static int overridesIssued_;
};

}  // namespace ReliefOps

#endif  // RELIEFOPS_INCIDENT_COMMANDER_H
