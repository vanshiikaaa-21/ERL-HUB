#include "Persistence.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>

#include "Exceptions.h"

namespace ReliefOps {

namespace {

/// Writes through `writer` into "<path>.tmp", then renames the temp file over `path`.
template <typename Writer>
void atomicWrite(const std::string& path, Writer writer) {
    const std::string tmp = path + ".tmp";
    {
        std::fstream out(tmp.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out) throw FileIOException("open for writing", tmp);
        try {
            writer(out);
        } catch (...) {
            out.close();
            std::remove(tmp.c_str());  // roll back: the real file was never touched
            throw;
        }
        out.flush();
        if (!out) {
            out.close();
            std::remove(tmp.c_str());
            throw FileIOException("write", tmp);
        }
    }  // stream closed here
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::remove(path.c_str());  // some platforms refuse to rename over an existing file
        if (std::rename(tmp.c_str(), path.c_str()) != 0) {
            std::remove(tmp.c_str());
            throw FileIOException("replace", path);
        }
    }
}

const char kLogMagic[4] = {'E', 'R', 'L', 'T'};
const char kManifestMagic[4] = {'E', 'R', 'L', 'M'};

}  // namespace

TransitRecord makeTransitRecord(const std::string& vehicleId, const std::string& destination, double tonnes,
                                double etaHours, int priority) {
    TransitRecord r;
    std::memset(&r, 0, sizeof(r));
    std::strncpy(r.vehicleId, vehicleId.c_str(), sizeof(r.vehicleId) - 1);
    std::strncpy(r.destination, destination.c_str(), sizeof(r.destination) - 1);
    r.tonnes = tonnes;
    r.etaHours = etaHours;
    r.priority = priority;
    r.timestamp = static_cast<long long>(std::time(nullptr));
    return r;
}

// ------------------------------------------------------------ TransitLog
void TransitLog::saveAll(const std::vector<TransitRecord>& records) const {
    atomicWrite(path_, [&records](std::ostream& out) {
        const std::uint32_t count = static_cast<std::uint32_t>(records.size());
        out.write(kLogMagic, 4);
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        if (count > 0)
            out.write(reinterpret_cast<const char*>(records.data()),
                      static_cast<std::streamsize>(sizeof(TransitRecord) * count));
    });
}

std::vector<TransitRecord> TransitLog::loadAll() const {
    std::fstream in(path_.c_str(), std::ios::in | std::ios::binary);
    if (!in) return std::vector<TransitRecord>();  // no log yet
    char magic[4] = {0, 0, 0, 0};
    std::uint32_t count = 0;
    in.read(magic, 4);
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in || std::memcmp(magic, kLogMagic, 4) != 0 || count > 1000000u)
        throw FileIOException("read (bad header in)", path_);
    std::vector<TransitRecord> records(count);
    if (count > 0)
        in.read(reinterpret_cast<char*>(records.data()),
                static_cast<std::streamsize>(sizeof(TransitRecord) * count));
    if (!in) throw FileIOException("read (truncated)", path_);
    return records;
}

void TransitLog::append(const TransitRecord& record) const {
    std::vector<TransitRecord> all = loadAll();
    all.push_back(record);
    saveAll(all);  // atomic: either the old log or the new log survives, never a mixture
}

// ------------------------------------------------------------ ManifestAudit
void ManifestAudit::save(const std::string& vehicleId, const std::vector<Inventory::ReliefCargo>& manifest) const {
    atomicWrite(path_, [&](std::ostream& out) {
        const std::uint32_t idLen = static_cast<std::uint32_t>(vehicleId.size());
        const std::uint32_t count = static_cast<std::uint32_t>(manifest.size());
        out.write(kManifestMagic, 4);
        out.write(reinterpret_cast<const char*>(&idLen), sizeof(idLen));
        out.write(vehicleId.data(), static_cast<std::streamsize>(idLen));
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (std::size_t i = 0; i < manifest.size(); ++i) manifest[i].writeBinary(out);
    });
}

std::vector<Inventory::ReliefCargo> ManifestAudit::restore(std::string& vehicleIdOut) const {
    std::fstream in(path_.c_str(), std::ios::in | std::ios::binary);
    if (!in) throw FileIOException("open for reading", path_);
    char magic[4] = {0, 0, 0, 0};
    std::uint32_t idLen = 0, count = 0;
    in.read(magic, 4);
    in.read(reinterpret_cast<char*>(&idLen), sizeof(idLen));
    if (!in || std::memcmp(magic, kManifestMagic, 4) != 0 || idLen > 256)
        throw FileIOException("read (bad header in)", path_);
    std::string id(idLen, '\0');
    if (idLen > 0) in.read(&id[0], static_cast<std::streamsize>(idLen));
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in || count > 100000u) throw FileIOException("read (bad header in)", path_);
    std::vector<Inventory::ReliefCargo> items(count);
    for (std::uint32_t i = 0; i < count; ++i) items[i].readBinary(in);
    vehicleIdOut = id;
    return items;
}

}  // namespace ReliefOps
