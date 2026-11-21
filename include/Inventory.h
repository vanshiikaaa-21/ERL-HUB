// Inventory.h  -  namespace ReliefOps::Inventory
// Unit II: abstract data types (ReliefCargo, DistributionDepot), operator overloading,
//          static data members, const correctness.
#ifndef RELIEFOPS_INVENTORY_H
#define RELIEFOPS_INVENTORY_H

#include <iosfwd>
#include <string>
#include <unordered_map>

namespace ReliefOps {

class IncidentCommander;  // friend class (declared in IncidentCommander.h)

namespace Inventory {

enum class SupplyType { RationKit = 0, TraumaMedicine = 1, WaterTank = 2, ThermalBlanket = 3 };

const char* supplyTypeName(SupplyType type);
bool parseSupplyType(const std::string& name, SupplyType& out);
int nextBatchId();  // hands out unique batch numbers

// ---------------------------------------------------------------------------
// ReliefCargo: one batch of a single supply type. All data is private.
// ---------------------------------------------------------------------------
class ReliefCargo {
public:
    ReliefCargo();
    ReliefCargo(int batchId, SupplyType type, int quantity, double unitWeightKg, int priority);

    // const-correct read access
    int batchId() const { return batchId_; }
    SupplyType type() const { return type_; }
    int quantity() const { return quantity_; }
    double unitWeightKg() const { return unitWeightKg_; }
    int priority() const { return priority_; }
    double totalWeightKg() const { return quantity_ * unitWeightKg_; }  // inline utility

    ReliefCargo& operator+=(int units);  // add units to this batch
    ReliefCargo& operator-=(int units);  // remove units (throws if not enough)

    /// Manifest identity: two cargo records are "the same" if batch id and supply type match.
    bool operator==(const ReliefCargo& other) const;
    bool operator!=(const ReliefCargo& other) const { return !(*this == other); }
    /// Field-by-field equality (used to verify a restore from disk).
    bool identicalTo(const ReliefCargo& other) const;

    // binary persistence helpers
    void writeBinary(std::ostream& os) const;
    void readBinary(std::istream& is);

private:
    int batchId_;
    SupplyType type_;
    int quantity_;
    double unitWeightKg_;
    int priority_;  // 1 (routine) .. 5 (life-critical)

    friend class ReliefOps::IncidentCommander;
    friend std::ostream& operator<<(std::ostream& os, const ReliefCargo& cargo);
    friend std::istream& operator>>(std::istream& is, ReliefCargo& cargo);
};

std::ostream& operator<<(std::ostream& os, const ReliefCargo& cargo);
std::istream& operator>>(std::istream& is, ReliefCargo& cargo);

// ---------------------------------------------------------------------------
// DistributionDepot: keeps one aggregate stock record per supply type.
// ---------------------------------------------------------------------------
class DistributionDepot {
public:
    explicit DistributionDepot(const std::string& name);
    DistributionDepot(const DistributionDepot& other);
    DistributionDepot& operator=(const DistributionDepot&) = default;
    ~DistributionDepot();

    DistributionDepot& operator+=(const ReliefCargo& incoming);  // restock
    DistributionDepot& operator-=(const ReliefCargo& outgoing);  // deduct on vehicle departure

    // return-by-reference access to the stock record (throws if the type was never stocked)
    ReliefCargo& stockRef(SupplyType type);
    const ReliefCargo& stockRef(SupplyType type) const;

    bool has(SupplyType type) const;
    int quantityOf(SupplyType type) const;
    double totalWeightKg() const;
    const std::string& name() const { return name_; }
    void print(std::ostream& os) const;

    static int depotCount() { return depotCount_; }

private:
    std::string name_;
    std::unordered_map<int, ReliefCargo> stock_;  // key: static_cast<int>(SupplyType)
    static int depotCount_;

    friend class ReliefOps::IncidentCommander;
};

}  // namespace Inventory
}  // namespace ReliefOps

#endif  // RELIEFOPS_INVENTORY_H
