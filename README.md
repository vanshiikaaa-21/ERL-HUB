# ERL-HUB: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub

**Course:** Object-Oriented Programming Using C++ (CIC-211), MAIT, Department of CSE
**Option chosen:** Option 2 (ERL-HUB)
**Author(s):** _<Vanshika Sharma 33014802725, Hiya Sankhla 34114802725>_

A modular standard C++ (C++11-compliant, also builds as C++14/17) simulation of a relief hub that
stocks supplies in depots, loads them onto heterogeneous vehicles (trucks, helicopters, boats and
an amphibious medical unit), validates routes, dispatches convoys and persists logs to disk.

## Build and run

```bash
make          # builds ./erl_hub  (g++ -std=c++11 -Wall -Wextra -pedantic)
make run      # builds and runs the demo
make clean

# without make:
g++ -std=c++11 -Wall -Wextra -pedantic -Iinclude src/*.cpp -o erl_hub && ./erl_hub
```

The demo (`src/main.cpp`) prints one section per syllabus unit. It creates `manifest_audit.bin` and
`transit_log.bin` in the working directory.

## Project layout

| File | Contents |
|------|----------|
| `include/Exceptions.h` | `ReliefException : std::exception` and its subclasses |
| `include/Inventory.h`, `src/Inventory.cpp` | `ReliefOps::Inventory`: `ReliefCargo`, `DistributionDepot`, `SupplyType` |
| `include/Logistics.h`, `src/Logistics.cpp` | `ReliefOps::Logistics`: inline utilities, overloaded `allocateSupplies`, `TransportVehicle`, dispatch functors |
| `include/Vehicles.h`, `src/Vehicles.cpp` | `ReliefVehicle` hierarchy, `GPSBeacon`, `EmergencyLifeSupportUnit`, diamond class |
| `include/IncidentCommander.h`, `src/IncidentCommander.cpp` | friend class for triage overrides |
| `include/PriorityDispatchQueue.h` | generic `PriorityDispatchQueue<T, Compare>` |
| `include/Manipulators.h` | custom I/O manipulators (`manifestHeader`, `tableRule`, `asRecord`, `asTable`, `banner`) |
| `include/Persistence.h`, `src/Persistence.cpp` | binary `std::fstream` persistence with atomic file replacement |
| `src/main.cpp` | demo driver |

## Requirement checklist

### Unit I: core foundations and call semantics
| Requirement | Where |
|---|---|
| Namespaces `ReliefOps::Logistics` and `ReliefOps::Inventory` | `Inventory.h`, `Logistics.h`, `Vehicles.h` |
| Function overloading for supply allocation (volume vs. medical triage) | `Logistics.h` / `Logistics.cpp`: both `allocateSupplies()` overloads |
| Call by reference to update stock and missions | `escalatePriority(MissionParams&, int)`, `allocateSupplies(DistributionDepot&, ...)` |
| Return by reference | `moreUrgent()`, `DistributionDepot::stockRef()`, `operator+=` / `-=` returning `*this` |
| Inline utilities for distance / payload | `payloadKg`, `travelTimeHours`, `geoDistanceKm` in `Logistics.h` |
| Friend class `IncidentCommander` | `IncidentCommander.h/.cpp`, declared `friend` in `ReliefCargo`, `DistributionDepot`, `TransportVehicle` |

### Unit II: encapsulation, dynamic memory, operator overloading
| Requirement | Where |
|---|---|
| `ReliefCargo`, `DistributionDepot`, `TransportVehicle` as ADTs with information hiding | all data members private; const-correct getters |
| Static variables and functions (tonnage, operational vehicles) | `TransportVehicle::totalDispatchedTonnes()`, `operationalVehicles()`; `DistributionDepot::depotCount()` |
| Dynamic allocation for manifests: dynamic constructor, copy constructor, destructor | `TransportVehicle` (heap array `manifest_`, deep copy, copy assignment, `delete[]`) |
| `+=` restock | `DistributionDepot::operator+=` |
| `-=` deduct on departure | `DistributionDepot::operator-=` |
| `==` manifest identity / duplicates | `ReliefCargo::operator==`, `TransportVehicle::operator==`, used by `load()` |
| `<<` and `>>` | `ReliefCargo`, `TransportVehicle` (table form, or serialisation form after `asRecord`) |

### Unit III: inheritance, containership, polymorphism
| Requirement | Where |
|---|---|
| `ReliefVehicle` into `AerialTransport`, `GroundConvoy`, `AmphibiousCraft` | `Vehicles.h` |
| Hybrid vehicle with virtual base classes (diamond) | `AmphibiousMedicalUnit : GroundConvoy, AmphibiousCraft`; both inherit `ReliefVehicle` with `virtual` |
| Containership: `GPSBeacon` and `EmergencyLifeSupportUnit` | private members of `ReliefVehicle` |
| Runtime polymorphism through `ReliefVehicle*`, virtual destructors | `std::vector<ReliefVehicle*>` in `main.cpp`; `FleetOwner` deletes through base pointers |

### Unit IV: streams, persistence, exceptions, STL
| Requirement | Where |
|---|---|
| Exceptions derived from `std::exception` | `CargoOverweightException`, `FuelDepletionException`, `DamagedCorridorException` (plus stock, duplicate and file errors) |
| Binary `std::fstream` save/restore of transit logs and manifest audits | `TransitLog`, `ManifestAudit` in `Persistence.cpp` |
| Atomic file updates | `atomicWrite()`: write to `<file>.tmp`, then rename over the real file; the temp file is removed on failure |
| Template class `PriorityDispatchQueue<T>` | `PriorityDispatchQueue.h` |
| STL containers and algorithms | `std::vector`, `std::unordered_map`, `std::sort`, `std::for_each`, heap algorithms |
| Functors for sorting urgent medical needs | `UrgencyLess`, `MostUrgentFirst` in `Logistics.h` |
| User-defined I/O manipulators for tables | `Manipulators.h` |

## Design notes

* **Why `TransportVehicle` and `ReliefVehicle` are separate:** `TransportVehicle` is the data-holding ADT
  (dynamic manifest, rule of three, statics). `ReliefVehicle` adds the behaviour (routes, fuel, GPS) and is
  the root of the polymorphic hierarchy.
* **Why the intermediate vehicle classes inherit `virtual`:** the hybrid unit would otherwise contain two
  separate `ReliefVehicle` objects (two manifests, two ids). With virtual inheritance there is exactly one,
  and the most-derived class initialises it.
* **Manifest identity:** `ReliefCargo::operator==` compares batch id and supply type, not quantity, so it
  catches duplicate batches. `identicalTo()` compares every field and is used to verify disk restores.
* **Atomic updates:** `std::rename` replaces the file in one step on POSIX systems. Some platforms refuse to
  rename over an existing file, so the code removes the old file and retries as a fallback.
* **Not thread-safe:** the static counters are plain variables, which is fine for this single-threaded program.
