// Manipulators.h  -  Unit IV: user-defined I/O manipulators for tabular manifest printing
#ifndef RELIEFOPS_MANIPULATORS_H
#define RELIEFOPS_MANIPULATORS_H

#include <iomanip>
#include <ios>
#include <ostream>
#include <string>

namespace ReliefOps {

namespace detail {
/// Per-stream flag (stored with iword) that switches operator<< between the
/// human readable table row and the compact, re-readable "record" format.
inline long& recordSlot(std::ios_base& stream) {
    static const int index = std::ios_base::xalloc();
    return stream.iword(index);
}
}  // namespace detail

enum ColumnWidth { kWBatch = 6, kWType = 16, kWQty = 8, kWUnit = 9, kWTotal = 11, kWPrio = 5 };

/// Manipulator: subsequent cargo / manifest output uses the compact serialisation format.
inline std::ostream& asRecord(std::ostream& os) { detail::recordSlot(os) = 1; return os; }
/// Manipulator: subsequent cargo / manifest output uses the table format (default).
inline std::ostream& asTable(std::ostream& os) { detail::recordSlot(os) = 0; return os; }

/// Manipulator: horizontal rule that matches the manifest table.
inline std::ostream& tableRule(std::ostream& os) {
    const int widths[6] = {kWBatch, kWType, kWQty, kWUnit, kWTotal, kWPrio};
    os << '+';
    for (int i = 0; i < 6; ++i) os << std::string(static_cast<std::size_t>(widths[i] + 2), '-') << '+';
    return os << '\n';
}

/// Manipulator: rule + column titles + rule.
inline std::ostream& manifestHeader(std::ostream& os) {
    os << tableRule;
    std::ios::fmtflags saved = os.flags();
    os << std::left << "| " << std::setw(kWBatch) << "Batch"
       << " | " << std::setw(kWType) << "Supply"
       << " | " << std::setw(kWQty) << "Qty"
       << " | " << std::setw(kWUnit) << "Unit kg"
       << " | " << std::setw(kWTotal) << "Total kg"
       << " | " << std::setw(kWPrio) << "Prio" << " |\n";
    os.flags(saved);
    return os << tableRule;
}

/// Manipulator with an argument: prints a section banner.
struct Banner { std::string text; };
inline Banner banner(const std::string& text) { Banner b; b.text = text; return b; }
inline std::ostream& operator<<(std::ostream& os, const Banner& b) {
    return os << "\n=== " << b.text << " ===\n";
}

}  // namespace ReliefOps

#endif  // RELIEFOPS_MANIPULATORS_H
