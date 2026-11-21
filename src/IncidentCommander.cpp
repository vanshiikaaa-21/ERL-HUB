#include "IncidentCommander.h"

#include <algorithm>
#include <ostream>
#include <vector>

#include "Exceptions.h"

namespace ReliefOps {

using Inventory::DistributionDepot;
using Inventory::ReliefCargo;
using Inventory::SupplyType;

int IncidentCommander::overridesIssued_ = 0;

void IncidentCommander::overrideLoad(Logistics::TransportVehicle& vehicle, const ReliefCargo& cargo) const {
    if (vehicle.size_ == vehicle.capacity_) vehicle.grow();  // private access (friend)
    vehicle.manifest_[vehicle.size_++] = cargo;              // bypasses payload + duplicate checks
    ++overridesIssued_;
}

int IncidentCommander::commandeerStock(DistributionDepot& depot, SupplyType type, int quantity) const {
    std::unordered_map<int, ReliefCargo>::iterator it = depot.stock_.find(static_cast<int>(type));
    if (it == depot.stock_.end() || quantity <= 0) return 0;
    const int taken = std::min(quantity, it->second.quantity_);  // private field of ReliefCargo
    it->second.quantity_ -= taken;
    ++overridesIssued_;
    return taken;
}

void IncidentCommander::auditDepot(const DistributionDepot& depot, std::ostream& os) const {
    std::vector<const ReliefCargo*> rows;
    for (std::unordered_map<int, ReliefCargo>::const_iterator it = depot.stock_.begin();
         it != depot.stock_.end(); ++it)
        rows.push_back(&it->second);
    std::sort(rows.begin(), rows.end(), [](const ReliefCargo* a, const ReliefCargo* b) {
        return static_cast<int>(a->type()) < static_cast<int>(b->type());
    });
    os << "  [audit by " << name_ << "] raw private records of '" << depot.name_ << "':\n";
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const ReliefCargo& c = *rows[i];
        os << "    " << Inventory::supplyTypeName(c.type_) << ": qty=" << c.quantity_
           << ", unit=" << detail::num(c.unitWeightKg_) << " kg, batch #" << c.batchId_
           << ", priority " << c.priority_ << '\n';
    }
}

}  // namespace ReliefOps
