#include "Inventory.h"

#include <algorithm>
#include <istream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Exceptions.h"
#include "Manipulators.h"

namespace ReliefOps {
namespace Inventory {

// ------------------------------------------------------------------ helpers
const char* supplyTypeName(SupplyType type) {
    switch (type) {
        case SupplyType::RationKit:      return "RationKit";
        case SupplyType::TraumaMedicine: return "TraumaMedicine";
        case SupplyType::WaterTank:      return "WaterTank";
        case SupplyType::ThermalBlanket: return "ThermalBlanket";
    }
    return "Unknown";
}

bool parseSupplyType(const std::string& name, SupplyType& out) {
    for (int i = 0; i < 4; ++i) {
        const SupplyType t = static_cast<SupplyType>(i);
        if (name == supplyTypeName(t)) { out = t; return true; }
    }
    return false;
}

int nextBatchId() {
    static int counter = 1000;
    return ++counter;
}

// -------------------------------------------------------------- ReliefCargo
ReliefCargo::ReliefCargo()
    : batchId_(0), type_(SupplyType::RationKit), quantity_(0), unitWeightKg_(1.0), priority_(1) {}

ReliefCargo::ReliefCargo(int batchId, SupplyType type, int quantity, double unitWeightKg, int priority)
    : batchId_(batchId), type_(type), quantity_(quantity), unitWeightKg_(unitWeightKg), priority_(priority) {
    if (quantity < 0) throw std::invalid_argument("ReliefCargo: negative quantity");
    if (unitWeightKg <= 0.0) throw std::invalid_argument("ReliefCargo: unit weight must be positive");
    if (priority < 1 || priority > 5) throw std::invalid_argument("ReliefCargo: priority must be 1..5");
}

ReliefCargo& ReliefCargo::operator+=(int units) {
    if (units < 0) throw std::invalid_argument("ReliefCargo::operator+=: negative units");
    quantity_ += units;
    return *this;
}

ReliefCargo& ReliefCargo::operator-=(int units) {
    if (units < 0) throw std::invalid_argument("ReliefCargo::operator-=: negative units");
    if (units > quantity_) throw InsufficientStockException(supplyTypeName(type_), units, quantity_);
    quantity_ -= units;
    return *this;
}

bool ReliefCargo::operator==(const ReliefCargo& other) const {
    return batchId_ == other.batchId_ && type_ == other.type_;
}

bool ReliefCargo::identicalTo(const ReliefCargo& other) const {
    return *this == other && quantity_ == other.quantity_ && unitWeightKg_ == other.unitWeightKg_ &&
           priority_ == other.priority_;
}

void ReliefCargo::writeBinary(std::ostream& os) const {
    const int type = static_cast<int>(type_);
    os.write(reinterpret_cast<const char*>(&batchId_), sizeof(batchId_));
    os.write(reinterpret_cast<const char*>(&type), sizeof(type));
    os.write(reinterpret_cast<const char*>(&quantity_), sizeof(quantity_));
    os.write(reinterpret_cast<const char*>(&unitWeightKg_), sizeof(unitWeightKg_));
    os.write(reinterpret_cast<const char*>(&priority_), sizeof(priority_));
    if (!os) throw FileIOException("write cargo record to", "stream");
}

void ReliefCargo::readBinary(std::istream& is) {
    int id = 0, type = 0, qty = 0, prio = 1;
    double unit = 1.0;
    is.read(reinterpret_cast<char*>(&id), sizeof(id));
    is.read(reinterpret_cast<char*>(&type), sizeof(type));
    is.read(reinterpret_cast<char*>(&qty), sizeof(qty));
    is.read(reinterpret_cast<char*>(&unit), sizeof(unit));
    is.read(reinterpret_cast<char*>(&prio), sizeof(prio));
    if (!is || type < 0 || type > 3) throw FileIOException("read cargo record from", "stream");
    try {
        *this = ReliefCargo(id, static_cast<SupplyType>(type), qty, unit, prio);
    } catch (const std::invalid_argument&) {
        throw FileIOException("read (corrupt) cargo record from", "stream");
    }
}

std::ostream& operator<<(std::ostream& os, const ReliefCargo& c) {
    if (detail::recordSlot(os) != 0) {  // compact format: "<batch> <type> <qty> <unitKg> <priority>"
        std::ostringstream rec;
        rec << c.batchId_ << ' ' << supplyTypeName(c.type_) << ' ' << c.quantity_ << ' '
            << std::setprecision(10) << c.unitWeightKg_ << ' ' << c.priority_;
        return os << rec.str();
    }
    std::ostringstream row;  // table row (local stream so os formatting state is untouched)
    row << std::left << "| " << std::setw(kWBatch) << c.batchId_ << " | " << std::setw(kWType)
        << supplyTypeName(c.type_) << " | " << std::right << std::setw(kWQty) << c.quantity_ << " | "
        << std::setw(kWUnit) << detail::num(c.unitWeightKg_) << " | " << std::setw(kWTotal)
        << detail::num(c.totalWeightKg()) << " | " << std::setw(kWPrio) << c.priority_ << " |";
    return os << row.str();
}

std::istream& operator>>(std::istream& is, ReliefCargo& c) {
    int id = 0, qty = 0, prio = 1;
    double unit = 1.0;
    std::string typeName;
    SupplyType type = SupplyType::RationKit;
    if (!(is >> id >> typeName >> qty >> unit >> prio)) return is;
    if (!parseSupplyType(typeName, type)) { is.setstate(std::ios::failbit); return is; }
    try {
        c = ReliefCargo(id, type, qty, unit, prio);
    } catch (const std::invalid_argument&) {
        is.setstate(std::ios::failbit);
    }
    return is;
}

// -------------------------------------------------------- DistributionDepot
int DistributionDepot::depotCount_ = 0;

DistributionDepot::DistributionDepot(const std::string& name) : name_(name) { ++depotCount_; }
DistributionDepot::DistributionDepot(const DistributionDepot& other)
    : name_(other.name_), stock_(other.stock_) { ++depotCount_; }
DistributionDepot::~DistributionDepot() { --depotCount_; }

DistributionDepot& DistributionDepot::operator+=(const ReliefCargo& incoming) {
    const int key = static_cast<int>(incoming.type());
    std::unordered_map<int, ReliefCargo>::iterator it = stock_.find(key);
    if (it == stock_.end()) stock_.insert(std::make_pair(key, incoming));
    else it->second += incoming.quantity();
    return *this;
}

DistributionDepot& DistributionDepot::operator-=(const ReliefCargo& outgoing) {
    std::unordered_map<int, ReliefCargo>::iterator it = stock_.find(static_cast<int>(outgoing.type()));
    if (it == stock_.end())
        throw InsufficientStockException(supplyTypeName(outgoing.type()), outgoing.quantity(), 0);
    it->second -= outgoing.quantity();  // throws InsufficientStockException if too little
    return *this;
}

ReliefCargo& DistributionDepot::stockRef(SupplyType type) {
    std::unordered_map<int, ReliefCargo>::iterator it = stock_.find(static_cast<int>(type));
    if (it == stock_.end()) throw InsufficientStockException(supplyTypeName(type), 1, 0);
    return it->second;
}

const ReliefCargo& DistributionDepot::stockRef(SupplyType type) const {
    std::unordered_map<int, ReliefCargo>::const_iterator it = stock_.find(static_cast<int>(type));
    if (it == stock_.end()) throw InsufficientStockException(supplyTypeName(type), 1, 0);
    return it->second;
}

bool DistributionDepot::has(SupplyType type) const { return stock_.count(static_cast<int>(type)) != 0; }

int DistributionDepot::quantityOf(SupplyType type) const {
    return has(type) ? stockRef(type).quantity() : 0;
}

double DistributionDepot::totalWeightKg() const {
    double total = 0.0;
    for (std::unordered_map<int, ReliefCargo>::const_iterator it = stock_.begin(); it != stock_.end(); ++it)
        total += it->second.totalWeightKg();
    return total;
}

void DistributionDepot::print(std::ostream& os) const {
    std::vector<const ReliefCargo*> rows;
    for (std::unordered_map<int, ReliefCargo>::const_iterator it = stock_.begin(); it != stock_.end(); ++it)
        rows.push_back(&it->second);
    std::sort(rows.begin(), rows.end(), [](const ReliefCargo* a, const ReliefCargo* b) {
        return static_cast<int>(a->type()) < static_cast<int>(b->type());
    });
    os << "Depot: " << name_ << " (" << detail::num(totalWeightKg()) << " kg in stock)\n"
       << asTable << manifestHeader;
    for (std::size_t i = 0; i < rows.size(); ++i) os << *rows[i] << '\n';
    os << tableRule;
}

}  // namespace Inventory
}  // namespace ReliefOps
