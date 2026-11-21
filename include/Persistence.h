// Persistence.h  -  Unit IV: binary std::fstream persistence with atomic file updates.
// Every save writes "<file>.tmp" first and only then renames it over the real file, so a crash
// in the middle of a write can never leave a half-written log or manifest behind.
#ifndef RELIEFOPS_PERSISTENCE_H
#define RELIEFOPS_PERSISTENCE_H

#include <string>
#include <vector>

#include "Inventory.h"

namespace ReliefOps {

/// Fixed-size POD record so it can be written to disk with a single binary write().
struct TransitRecord {
    char vehicleId[24];
    char destination[32];
    double tonnes;
    double etaHours;
    int priority;
    long long timestamp;
};

TransitRecord makeTransitRecord(const std::string& vehicleId, const std::string& destination, double tonnes,
                                double etaHours, int priority);

class TransitLog {
public:
    explicit TransitLog(const std::string& path) : path_(path) {}
    void saveAll(const std::vector<TransitRecord>& records) const;  // atomic replace
    std::vector<TransitRecord> loadAll() const;                     // empty if no log exists yet
    void append(const TransitRecord& record) const;                 // load + push + atomic save

private:
    std::string path_;
};

class ManifestAudit {
public:
    explicit ManifestAudit(const std::string& path) : path_(path) {}
    void save(const std::string& vehicleId, const std::vector<Inventory::ReliefCargo>& manifest) const;
    std::vector<Inventory::ReliefCargo> restore(std::string& vehicleIdOut) const;  // throws FileIOException

private:
    std::string path_;
};

}  // namespace ReliefOps

#endif  // RELIEFOPS_PERSISTENCE_H
