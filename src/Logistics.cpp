#include "Logistics.h"

#include <algorithm>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "Exceptions.h"
#include "Manipulators.h"

namespace ReliefOps {
namespace Logistics {

using Inventory::DistributionDepot;
using Inventory::ReliefCargo;
using Inventory::SupplyType;

// ------------------------------------------------------------ mission helpers
void escalatePriority(MissionParams& mission, int by) {
    mission.priority = std::max(1, std::min(5, mission.priority + by));
}

MissionParams& moreUrgent(MissionParams& a, MissionParams& b) {
    return (a.priority >= b.priority) ? a : b;
}

// ------------------------------------------------------------ TransportVehicle
double TransportVehicle::totalDispatchedTonnes_ = 0.0;
int TransportVehicle::operationalVehicles_ = 0;

TransportVehicle::TransportVehicle()
    : id_("UNASSIGNED"), maxPayloadKg_(0.0), manifest_(new ReliefCargo[4]), size_(0), capacity_(4) {
    ++operationalVehicles_;
}

TransportVehicle::TransportVehicle(const std::string& id, double maxPayloadKg, std::size_t initialCapacity)
    : id_(id), maxPayloadKg_(maxPayloadKg), manifest_(nullptr), size_(0),
      capacity_(initialCapacity > 0 ? initialCapacity : 1) {
    manifest_ = new ReliefCargo[capacity_];  // dynamic constructor: heap allocation
    ++operationalVehicles_;
}

TransportVehicle::TransportVehicle(const TransportVehicle& other)
    : id_(other.id_), maxPayloadKg_(other.maxPayloadKg_), manifest_(new ReliefCargo[other.capacity_]),
      size_(other.size_), capacity_(other.capacity_) {
    for (std::size_t i = 0; i < size_; ++i) manifest_[i] = other.manifest_[i];  // deep copy
    ++operationalVehicles_;
}

TransportVehicle& TransportVehicle::operator=(const TransportVehicle& other) {
    if (this == &other) return *this;
    ReliefCargo* fresh = new ReliefCargo[other.capacity_];  // allocate first: strong guarantee
    for (std::size_t i = 0; i < other.size_; ++i) fresh[i] = other.manifest_[i];
    delete[] manifest_;
    manifest_ = fresh;
    id_ = other.id_;
    maxPayloadKg_ = other.maxPayloadKg_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    return *this;
}

TransportVehicle::~TransportVehicle() {
    delete[] manifest_;
    --operationalVehicles_;
}

void TransportVehicle::grow() {
    const std::size_t newCapacity = capacity_ * 2;
    ReliefCargo* bigger = new ReliefCargo[newCapacity];
    for (std::size_t i = 0; i < size_; ++i) bigger[i] = manifest_[i];
    delete[] manifest_;
    manifest_ = bigger;
    capacity_ = newCapacity;
}

double TransportVehicle::currentLoadKg() const {
    double total = 0.0;
    for (std::size_t i = 0; i < size_; ++i) total += manifest_[i].totalWeightKg();
    return total;
}

const ReliefCargo& TransportVehicle::cargoAt(std::size_t index) const {
    if (index >= size_) throw std::out_of_range("TransportVehicle::cargoAt: index out of range");
    return manifest_[index];
}

bool TransportVehicle::containsBatch(const ReliefCargo& cargo) const {
    for (std::size_t i = 0; i < size_; ++i)
        if (manifest_[i] == cargo) return true;
    return false;
}

std::vector<ReliefCargo> TransportVehicle::manifestSnapshot() const {
    return std::vector<ReliefCargo>(manifest_, manifest_ + size_);
}

void TransportVehicle::load(const ReliefCargo& cargo) {
    if (containsBatch(cargo)) throw DuplicateCargoException(cargo.batchId());
    const double after = currentLoadKg() + cargo.totalWeightKg();
    if (after > maxPayloadKg_ + 1e-9) throw CargoOverweightException(id_, after, maxPayloadKg_);
    if (size_ == capacity_) grow();
    manifest_[size_++] = cargo;
}

double TransportVehicle::dispatch() {
    const double tonnes = currentLoadKg() / 1000.0;
    totalDispatchedTonnes_ += tonnes;
    size_ = 0;
    return tonnes;
}

bool TransportVehicle::operator==(const TransportVehicle& other) const {
    if (size_ != other.size_) return false;
    for (std::size_t i = 0; i < size_; ++i)
        if (!other.containsBatch(manifest_[i])) return false;
    return true;
}

std::ostream& operator<<(std::ostream& os, const TransportVehicle& v) {
    if (detail::recordSlot(os) != 0) {  // serialisation: "<count>\n" followed by one record per line
        os << v.size_ << '\n';
        for (std::size_t i = 0; i < v.size_; ++i) os << v.manifest_[i] << '\n';
        return os;
    }
    os << "Manifest of " << v.id_ << " (" << detail::num(v.currentLoadKg()) << " / "
       << detail::num(v.maxPayloadKg_) << " kg)\n" << manifestHeader;
    for (std::size_t i = 0; i < v.size_; ++i) os << v.manifest_[i] << '\n';
    return os << tableRule;
}

std::istream& operator>>(std::istream& is, TransportVehicle& v) {
    std::size_t count = 0;
    if (!(is >> count) || count > 100000) { is.setstate(std::ios::failbit); return is; }
    std::vector<ReliefCargo> incoming(count);
    for (std::size_t i = 0; i < count; ++i)
        if (!(is >> incoming[i])) return is;
    TransportVehicle staged(v);  // stage the change so a failure leaves v untouched
    staged.clearManifest();
    try {
        for (std::size_t i = 0; i < count; ++i) staged.load(incoming[i]);
    } catch (const ReliefException&) {
        is.setstate(std::ios::failbit);
        return is;
    }
    v = staged;
    return is;
}

// ------------------------------------------------------------ allocation overloads
namespace {
/// Moves up to `wanted` units of `type` from the depot onto the vehicle.  Returns units moved.
int moveUnits(DistributionDepot& depot, TransportVehicle& vehicle, SupplyType type, int wanted, int priority) {
    if (wanted <= 0 || !depot.has(type)) return 0;
    const ReliefCargo& stock = depot.stockRef(type);
    const double unit = stock.unitWeightKg();
    const int byWeight = static_cast<int>(vehicle.remainingCapacityKg() / unit);
    const int units = std::min(std::min(wanted, stock.quantity()), byWeight);
    if (units <= 0) return 0;
    ReliefCargo batch(Inventory::nextBatchId(), type, units, unit, priority);
    vehicle.load(batch);  // may throw: depot is untouched in that case
    depot -= batch;       // deduct cargo upon departure
    return units;
}
}  // namespace

double allocateSupplies(DistributionDepot& depot, TransportVehicle& vehicle, SupplyType type, double volumeKg) {
    if (!depot.has(type) || volumeKg <= 0.0) return 0.0;
    const double unit = depot.stockRef(type).unitWeightKg();
    const int wanted = static_cast<int>(volumeKg / unit);
    const int moved = moveUnits(depot, vehicle, type, wanted, depot.stockRef(type).priority());
    return payloadKg(moved, unit);
}

int allocateSupplies(DistributionDepot& depot, TransportVehicle& vehicle, int triageLevel) {
    const int level = std::max(1, std::min(5, triageLevel));
    return moveUnits(depot, vehicle, SupplyType::TraumaMedicine, 40 * level, 5);
}

}  // namespace Logistics
}  // namespace ReliefOps
