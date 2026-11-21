// main.cpp  -  ERL-HUB: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
// CIC-211  Object-Oriented Programming Using C++
//
// The demo walks through all four syllabus units in order. Every section prints what it is
// demonstrating so the output can be matched against the assignment checklist (see README.md).

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Exceptions.h"
#include "IncidentCommander.h"
#include "Inventory.h"
#include "Logistics.h"
#include "Manipulators.h"
#include "Persistence.h"
#include "PriorityDispatchQueue.h"
#include "Vehicles.h"

using namespace ReliefOps;
using Inventory::DistributionDepot;
using Inventory::ReliefCargo;
using Inventory::SupplyType;
using Logistics::ReliefVehicle;
using Logistics::Route;
using Logistics::TransportVehicle;

namespace {

/// Owns the polymorphic fleet; deleting through ReliefVehicle* exercises the virtual destructors.
struct FleetOwner {
    std::vector<ReliefVehicle*> vehicles;
    ~FleetOwner() {
        std::cout << banner("Fleet deallocation: delete through ReliefVehicle* (virtual destructors)");
        for (std::size_t i = 0; i < vehicles.size(); ++i) delete vehicles[i];
        std::cout << "  all vehicles released, operational vehicles now = "
                  << TransportVehicle::operationalVehicles() << '\n';
    }
};

/// Runs `action`, reporting either success or the diagnostic of a ReliefException.
template <typename Action>
void attempt(const std::string& label, Action action) {
    try {
        action();
        std::cout << "  [ok]      " << label << '\n';
    } catch (const ReliefException& e) {
        std::cout << "  [caught]  " << label << "\n            -> " << e.what() << '\n';
    }
}

const char* yesNo(bool value) { return value ? "yes" : "no"; }

// =========================================================================== UNIT I
void demoUnitI(DistributionDepot& hub, TransportVehicle& truck, IncidentCommander& commander) {
    std::cout << banner("UNIT I: namespaces, overloading, references, inline, friend");

    hub += ReliefCargo(Inventory::nextBatchId(), SupplyType::RationKit, 500, 2.5, 3);
    hub += ReliefCargo(Inventory::nextBatchId(), SupplyType::TraumaMedicine, 200, 0.8, 5);
    hub += ReliefCargo(Inventory::nextBatchId(), SupplyType::WaterTank, 20, 250.0, 4);
    hub += ReliefCargo(Inventory::nextBatchId(), SupplyType::ThermalBlanket, 800, 0.4, 2);
    hub.print(std::cout);

    std::cout << "\nInline utility functions (ReliefOps::Logistics):\n"
              << "  payload of 120 ration kits       : " << detail::num(Logistics::payloadKg(120, 2.5)) << " kg\n"
              << "  Dehradun -> Joshimath (approx.)  : "
              << detail::num(Logistics::geoDistanceKm(30.3165, 78.0322, 30.5560, 79.5650)) << " km\n"
              << "  200 km at 45 km/h                : " << detail::num(Logistics::travelTimeHours(200, 45), 2)
              << " h\n";

    std::cout << "\nOverloaded allocateSupplies():\n";
    const double kg = Logistics::allocateSupplies(hub, truck, SupplyType::RationKit, 750.0);  // volume based
    std::cout << "  volume-based  (750 kg of ration kits)  -> loaded " << detail::num(kg) << " kg\n";
    const int units = Logistics::allocateSupplies(hub, truck, 3);  // priority triage
    std::cout << "  triage level 3 (trauma medicine)        -> loaded " << units << " units\n";
    std::cout << truck;

    std::cout << "\nCall by reference / return by reference:\n";
    Logistics::MissionParams alpha = {"Chamoli", 140.0, 3, 750.0};
    Logistics::MissionParams bravo = {"Uttarkashi", 95.0, 4, 400.0};
    Logistics::escalatePriority(alpha, 2);                   // modifies alpha in place
    Logistics::moreUrgent(alpha, bravo).payloadKg += 250.0;  // returns a reference to the real object
    hub.stockRef(SupplyType::WaterTank) += 5;                // returns a reference into the depot's stock
    std::cout << "  alpha priority after escalate : " << alpha.priority << '\n'
              << "  alpha payload after += 250    : " << detail::num(alpha.payloadKg) << " kg (same object)\n"
              << "  bravo payload (untouched)     : " << detail::num(bravo.payloadKg) << " kg\n"
              << "  water tanks after += 5 via ref: " << hub.quantityOf(SupplyType::WaterTank) << '\n';

    std::cout << "\nFriend class IncidentCommander (critical triage override):\n";
    TransportVehicle van("VAN-07", 100.0);
    ReliefCargo tanks(Inventory::nextBatchId(), SupplyType::WaterTank, 2, 250.0, 5);
    attempt("load 500 kg of water onto a 100 kg van", [&] { van.load(tanks); });
    commander.overrideLoad(van, tanks);
    std::cout << "  override by " << commander.name() << ": van now carries " << detail::num(van.currentLoadKg())
              << " kg\n";
    const int taken = commander.commandeerStock(hub, SupplyType::WaterTank, 3);
    std::cout << "  commandeered " << taken << " water tanks straight from the depot\n";
    commander.auditDepot(hub, std::cout);
}

// =========================================================================== UNIT II
void demoUnitII(DistributionDepot& hub, TransportVehicle& truck) {
    std::cout << banner("UNIT II: ADTs, static members, dynamic memory, operators");

    std::cout << "Static analytics:\n"
              << "  operational vehicles : " << TransportVehicle::operationalVehicles() << '\n'
              << "  depots               : " << DistributionDepot::depotCount() << '\n'
              << "  tonnes dispatched    : " << detail::num(TransportVehicle::totalDispatchedTonnes(), 3) << '\n';

    std::cout << "\nDeep copy constructor:\n";
    TransportVehicle copy(truck);
    copy.clearManifest();
    TransportVehicle twin(truck);
    std::cout << "  copy cleared -> original still holds " << truck.manifestSize() << " batches, copy holds "
              << copy.manifestSize() << '\n'
              << "  twin == truck : " << yesNo(twin == truck) << "    copy == truck : " << yesNo(copy == truck) << '\n'
              << "  operational vehicles now: " << TransportVehicle::operationalVehicles() << '\n';

    std::cout << "\nOperators += (restock) and -= (departure) on the depot:\n";
    std::cout << "  ration kits before : " << hub.quantityOf(SupplyType::RationKit) << '\n';
    hub += ReliefCargo(Inventory::nextBatchId(), SupplyType::RationKit, 100, 2.5, 3);
    std::cout << "  after += 100       : " << hub.quantityOf(SupplyType::RationKit) << '\n';
    hub -= ReliefCargo(Inventory::nextBatchId(), SupplyType::RationKit, 60, 2.5, 3);
    std::cout << "  after -= 60        : " << hub.quantityOf(SupplyType::RationKit) << '\n';

    std::cout << "\nOperator == (manifest identity / duplicate check):\n";
    ReliefCargo a(7001, SupplyType::WaterTank, 4, 250.0, 4);
    ReliefCargo b(7001, SupplyType::WaterTank, 9, 250.0, 4);
    ReliefCargo c(7002, SupplyType::WaterTank, 4, 250.0, 4);
    std::cout << "  a == b (same batch)      : " << yesNo(a == b) << '\n'
              << "  a == c (different batch) : " << yesNo(a == c) << '\n';

    std::cout << "\nOperators << and >>:\n";
    std::istringstream input("9001 WaterTank 3 250.0 4");
    ReliefCargo parsed;
    input >> parsed;  // operator>>
    std::cout << manifestHeader << parsed << '\n' << tableRule;  // operator<< (table form)
    std::ostringstream record;
    record << asRecord << parsed << asTable;  // operator<< (serialisation form)
    std::istringstream back(record.str());
    ReliefCargo again;
    back >> again;
    std::cout << "  record form: \"" << record.str() << "\"  -> round trip identical: "
              << yesNo(again.identicalTo(parsed)) << '\n';

    std::ostringstream manifestText;
    manifestText << asRecord << truck << asTable;
    TransportVehicle restored("TRK-01-RESTORED", truck.maxPayloadKg());
    std::istringstream manifestIn(manifestText.str());
    manifestIn >> restored;
    std::cout << "  whole manifest serialised and read back into another vehicle: restored == truck : "
              << yesNo(restored == truck) << '\n';
}

// =========================================================================== UNIT III
void buildFleet(FleetOwner& fleet) {
    fleet.vehicles.push_back(new Logistics::AerialTransport("HELI-01", 1200.0, 600.0, 30.3165, 78.0322));
    fleet.vehicles.push_back(new Logistics::GroundConvoy("CONVOY-02", 8000.0, 500.0, 30.3165, 78.0322, 6));
    fleet.vehicles.push_back(new Logistics::AmphibiousCraft("BOAT-03", 2500.0, 300.0, 30.3165, 78.0322));
    fleet.vehicles.push_back(
        new Logistics::AmphibiousMedicalUnit("AMU-04", 1800.0, 350.0, 30.3165, 78.0322, 6));
}

std::vector<Route> makeRoutes() {
    std::vector<Route> routes;
    routes.push_back(Route("NH-58 landslide stretch", 85.0, true, false, 30.40, 78.60));
    routes.push_back(Route("Alaknanda river crossing", 40.0, false, true, 30.55, 78.90));
    routes.push_back(Route("Valley highway", 120.0, false, false, 30.70, 79.10));
    routes.push_back(Route("Remote ridge camp", 420.0, false, false, 31.00, 79.50));
    return routes;
}

void demoUnitIII(FleetOwner& fleet) {
    std::cout << banner("UNIT III: inheritance, containership, virtual bases, polymorphism");

    const std::vector<Route> routes = makeRoutes();
    std::cout << "Route evaluation through std::vector<ReliefVehicle*> (runtime dispatch):\n";
    for (std::size_t v = 0; v < fleet.vehicles.size(); ++v) {
        const ReliefVehicle* vehicle = fleet.vehicles[v];
        for (std::size_t r = 0; r < routes.size(); ++r) {
            std::cout << "  " << std::left << std::setw(11) << vehicle->id() << std::setw(22)
                      << vehicle->vehicleClass() << std::setw(26) << routes[r].name;
            try {
                const double eta = vehicle->evaluateRoute(routes[r]);  // virtual call
                std::cout << "ETA " << detail::num(eta, 2) << " h\n";
            } catch (const ReliefException& e) {
                std::cout << "FAILED: " << e.what() << '\n';
            }
        }
    }

    std::cout << "\nDiamond problem (virtual base classes):\n";
    Logistics::AmphibiousMedicalUnit* amu = dynamic_cast<Logistics::AmphibiousMedicalUnit*>(fleet.vehicles[3]);
    if (amu != nullptr) {
        ReliefVehicle* viaGround = static_cast<Logistics::GroundConvoy*>(amu);
        ReliefVehicle* viaBoat = static_cast<Logistics::AmphibiousCraft*>(amu);
        std::cout << "  GroundConvoy path and AmphibiousCraft path reach the same ReliefVehicle: "
                  << yesNo(viaGround == viaBoat) << '\n'
                  << "  (one shared manifest, one id: " << amu->id() << ")\n";
        std::cout << "  hybrid treats 4 patients: " << yesNo(amu->treatPatients(4, 20)) << " (oxygen left "
                  << amu->lifeSupport().oxygenMinutes() << " min)\n";
    }

    std::cout << "\nContainership (HAS-A GPSBeacon + EmergencyLifeSupportUnit) and a full run:\n";
    ReliefVehicle* medic = fleet.vehicles[3];
    medic->load(ReliefCargo(Inventory::nextBatchId(), SupplyType::TraumaMedicine, 100, 0.8, 5));
    const Route river("Alaknanda river crossing", 40.0, false, true, 30.55, 78.90);
    std::cout << "  " << medic->id() << " GPS before: " << medic->gps() << ", range " << detail::num(medic->fuelRangeKm())
              << " km\n";
    const double eta = medic->executeRoute(river);  // virtual call through the base pointer
    const double tonnes = medic->dispatch();
    std::cout << "  " << medic->id() << " crossed in " << detail::num(eta, 2) << " h, GPS now " << medic->gps()
              << ", range left " << detail::num(medic->fuelRangeKm()) << " km, dispatched "
              << detail::num(tonnes, 3) << " t\n";
}

// =========================================================================== UNIT IV
void demoUnitIV(FleetOwner& fleet, DistributionDepot& hub, TransportVehicle& truck) {
    std::cout << banner("UNIT IV: exceptions, binary streams, STL, templates, manipulators");

    std::cout << "Custom exception hierarchy (all derive from std::exception):\n";
    const Route farRidge("Remote ridge camp", 420.0, false, false, 31.0, 79.5);
    const Route landslide("NH-58 landslide stretch", 85.0, true, false, 30.4, 78.6);
    attempt("helicopter on a 420 km leg", [&] { fleet.vehicles[0]->evaluateRoute(farRidge); });
    attempt("convoy on a landslide-blocked road", [&] { fleet.vehicles[1]->evaluateRoute(landslide); });
    attempt("overload the truck with 50 water tanks", [&] {
        truck.load(ReliefCargo(Inventory::nextBatchId(), SupplyType::WaterTank, 50, 250.0, 5));
    });
    attempt("load the same batch twice", [&] { truck.load(truck.cargoAt(0)); });
    attempt("take 9999 ration kits from the depot", [&] {
        hub -= ReliefCargo(Inventory::nextBatchId(), SupplyType::RationKit, 9999, 2.5, 3);
    });
    attempt("restore a manifest from a file that does not exist", [&] {
        std::string id;
        ManifestAudit("no_such_manifest.bin").restore(id);
    });

    std::cout << "\nBinary persistence with atomic updates (std::fstream):\n";
    ManifestAudit audit("manifest_audit.bin");
    const std::vector<ReliefCargo> before = truck.manifestSnapshot();
    audit.save(truck.id(), before);
    std::string restoredId;
    const std::vector<ReliefCargo> after = audit.restore(restoredId);
    bool same = before.size() == after.size();
    for (std::size_t i = 0; same && i < before.size(); ++i) same = before[i].identicalTo(after[i]);
    std::cout << "  manifest of " << restoredId << ": saved " << before.size() << " batches, restored "
              << after.size() << ", identical: " << yesNo(same) << '\n';

    TransitLog log("transit_log.bin");
    log.saveAll(std::vector<TransitRecord>());  // start with a clean log for this demo run
    const double tonnes = truck.dispatch();
    log.append(makeTransitRecord(truck.id(), "Chamoli", tonnes, 3.1, 5));
    log.append(makeTransitRecord("HELI-01", "Uttarkashi", 0.9, 0.8, 4));
    log.append(makeTransitRecord("AMU-04", "Tehri", 0.08, 1.6, 5));
    const std::vector<TransitRecord> records = log.loadAll();
    std::cout << "  transit log reloaded from disk (" << records.size() << " records):\n";
    std::for_each(records.begin(), records.end(), [](const TransitRecord& r) {
        std::cout << "    " << std::left << std::setw(9) << r.vehicleId << std::setw(12) << r.destination
                  << std::right << std::setw(7) << detail::num(r.tonnes, 3) << " t  ETA "
                  << detail::num(r.etaHours, 1) << " h  prio " << r.priority << '\n';
    });

    std::cout << "\nPriorityDispatchQueue<DispatchRequest, UrgencyLess> (template + functor):\n";
    std::vector<Logistics::DispatchRequest> requests;
    Logistics::DispatchRequest r1 = {1, "Chamoli", SupplyType::RationKit, 300, 3};
    Logistics::DispatchRequest r2 = {2, "Tehri", SupplyType::TraumaMedicine, 40, 3};
    Logistics::DispatchRequest r3 = {3, "Uttarkashi", SupplyType::WaterTank, 6, 5};
    Logistics::DispatchRequest r4 = {4, "Rudraprayag", SupplyType::TraumaMedicine, 80, 5};
    Logistics::DispatchRequest r5 = {5, "Pauri", SupplyType::ThermalBlanket, 500, 2};
    requests.push_back(r1);
    requests.push_back(r2);
    requests.push_back(r3);
    requests.push_back(r4);
    requests.push_back(r5);

    PriorityDispatchQueue<Logistics::DispatchRequest, Logistics::UrgencyLess> queue;
    for (std::size_t i = 0; i < requests.size(); ++i) queue.push(requests[i]);
    std::cout << "  serving order (medical first, then priority, then FIFO):\n";
    while (!queue.empty()) {
        const Logistics::DispatchRequest next = queue.pop();
        std::cout << "    #" << next.id << " " << std::left << std::setw(12) << next.destination << std::setw(15)
                  << Inventory::supplyTypeName(next.type) << "x" << next.quantity << "  prio " << next.priority
                  << '\n';
    }

    std::sort(requests.begin(), requests.end(), Logistics::MostUrgentFirst());  // functor with std::sort
    std::cout << "  same ordering using std::sort + MostUrgentFirst functor: ";
    std::for_each(requests.begin(), requests.end(),
                  [](const Logistics::DispatchRequest& r) { std::cout << '#' << r.id << ' '; });
    std::cout << '\n';

    std::cout << "\nstd::unordered_map fleet registry:\n";
    std::unordered_map<std::string, ReliefVehicle*> registry;
    for (std::size_t i = 0; i < fleet.vehicles.size(); ++i) registry[fleet.vehicles[i]->id()] = fleet.vehicles[i];
    std::unordered_map<std::string, ReliefVehicle*>::const_iterator found = registry.find("BOAT-03");
    if (found != registry.end())
        std::cout << "  registry[\"BOAT-03\"] -> " << found->second->vehicleClass() << ", cruise "
                  << detail::num(found->second->cruiseSpeedKmh(), 0) << " km/h\n";
    std::cout << "  registry holds " << registry.size() << " vehicles\n";

    std::cout << "\nUser-defined manifest table (custom manipulators) for the depot after all operations:\n";
    hub.print(std::cout);
}

}  // namespace

int main() {
    try {
        DistributionDepot hub("Dehradun Field Hub");
        TransportVehicle truck("TRK-01", 3000.0);
        IncidentCommander commander("Incident Cmdr. A. Singh");
        FleetOwner fleet;

        demoUnitI(hub, truck, commander);
        demoUnitII(hub, truck);
        buildFleet(fleet);
        demoUnitIII(fleet);
        demoUnitIV(fleet, hub, truck);

        std::cout << banner("Network-wide analytics (static members)");
        std::cout << "  total dispatched tonnage : " << detail::num(TransportVehicle::totalDispatchedTonnes(), 3)
                  << " t\n  operational vehicles     : " << TransportVehicle::operationalVehicles()
                  << "\n  depots                   : " << DistributionDepot::depotCount()
                  << "\n  commander overrides      : " << IncidentCommander::overridesIssued() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
