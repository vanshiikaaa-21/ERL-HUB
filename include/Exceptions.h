// Exceptions.h  -  Unit IV: custom exception framework derived from std::exception
#ifndef RELIEFOPS_EXCEPTIONS_H
#define RELIEFOPS_EXCEPTIONS_H

#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace ReliefOps {

namespace detail {
/// Formats a number with fixed precision (used for diagnostic messages and tables).
inline std::string num(double value, int precision = 1) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(precision) << value;
    return os.str();
}
}  // namespace detail

/// Common root of every error raised by the relief hub. Derives from std::exception.
class ReliefException : public std::exception {
public:
    explicit ReliefException(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

class CargoOverweightException : public ReliefException {
public:
    CargoOverweightException(const std::string& vehicleId, double attemptedKg, double limitKg)
        : ReliefException("CargoOverweightException: vehicle " + vehicleId + " cannot carry " +
                          detail::num(attemptedKg) + " kg (limit " + detail::num(limitKg) + " kg)") {}
};

class FuelDepletionException : public ReliefException {
public:
    FuelDepletionException(const std::string& vehicleId, double requiredKm, double remainingKm)
        : ReliefException("FuelDepletionException: vehicle " + vehicleId + " needs " +
                          detail::num(requiredKm) + " km of fuel range but has " +
                          detail::num(remainingKm) + " km") {}
};

class DamagedCorridorException : public ReliefException {
public:
    DamagedCorridorException(const std::string& routeName, const std::string& vehicleClass)
        : ReliefException("DamagedCorridorException: route '" + routeName +
                          "' is impassable for " + vehicleClass) {}
};

class InsufficientStockException : public ReliefException {
public:
    InsufficientStockException(const std::string& item, int requested, int available)
        : ReliefException("InsufficientStockException: requested " + std::to_string(requested) + " x " +
                          item + " but only " + std::to_string(available) + " available") {}
};

class DuplicateCargoException : public ReliefException {
public:
    explicit DuplicateCargoException(int batchId)
        : ReliefException("DuplicateCargoException: batch #" + std::to_string(batchId) +
                          " is already on the manifest") {}
};

class FileIOException : public ReliefException {
public:
    FileIOException(const std::string& operation, const std::string& path)
        : ReliefException("FileIOException: could not " + operation + " '" + path + "'") {}
};

}  // namespace ReliefOps

#endif  // RELIEFOPS_EXCEPTIONS_H
