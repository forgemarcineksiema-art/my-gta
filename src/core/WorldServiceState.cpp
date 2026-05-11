#include "bs3d/core/WorldServiceState.h"

#include "bs3d/core/WorldRewirPressure.h"

#include <algorithm>
#include <utility>

namespace bs3d {

namespace {

const std::vector<LocalRewirServiceSpec>& localRewirServiceCatalogStorage() {
    static const std::vector<LocalRewirServiceSpec> specs{
        {"garage_rysiek",
         "garage_service_rysiek",
         WorldLocationTag::Garage,
         {-18.0f, 0.0f, 23.0f},
         2.8f,
         "Rysiek",
         42,
         "E: pogadaj z Ryskiem",
         "Gruz da sie zrobic, tylko nie pytaj jak.",
         "E: Rysiek liczy rysy",
         "Najpierw ostygnie garaz, potem gadamy o gruzie."},
        {"trash_dozorca",
         "trash_service_dozorca",
         WorldLocationTag::Trash,
         {9.0f, 0.0f, -4.0f},
         2.8f,
         "Dozorca",
         41,
         "E: pogadaj z Dozorca",
         "Smietnik spokojny, osiedle oddycha.",
         "E: Dozorca zamyka altane",
         "Po takim balecie altana ma godziny ciszy."},
        {"parking_parkingowy",
         "parking_service_parkingowy",
         WorldLocationTag::Parking,
         {-7.0f, 0.0f, 8.6f},
         2.8f,
         "Parkingowy",
         40,
         "E: pogadaj z Parkingowym",
         "Miejsca sa, tylko kultura jazdy nie.",
         "E: Parkingowy mierzy wgnioty",
         "Parking pamieta kazdy bokiem wjazd."},
        {"road_kierowca",
         "road_service_kierowca",
         WorldLocationTag::RoadLoop,
         {8.0f, 0.0f, -18.0f},
         2.8f,
         "Kierowca",
         39,
         "E: pogadaj z Kierowca",
         "Droga pusta, ale mandat wisi w powietrzu.",
         "E: Kierowca liczy slady",
         "Ta droga ma pamiec lepsza niz kierowcy."}
    };
    return specs;
}

const std::vector<LocalRewirFavorSpec>& localRewirFavorCatalogStorage() {
    static const std::vector<LocalRewirFavorSpec> specs{
        {"garage_favor_rysiek",
         "garage_service_rysiek",
         "garage_favor_rysiek_point",
         WorldLocationTag::Garage,
         {-21.4f, 0.0f, 25.2f},
         2.2f,
         "E: przetrzyj rysy",
         "Jak juz tu stoisz, przetrzyj rysy i bedzie ciszej.",
         "No, teraz garaz oddycha."},
        {"trash_favor_dozorca",
         "trash_service_dozorca",
         "trash_favor_dozorca_point",
         WorldLocationTag::Trash,
         {5.8f, 0.0f, -6.4f},
         2.2f,
         "E: ustaw kosz",
         "Ustaw kosz przy altanie, bo znowu bedzie gadane.",
         "Altana wyglada jak po czlowieku."},
        {"parking_favor_parkingowy",
         "parking_service_parkingowy",
         "parking_favor_parkingowy_point",
         WorldLocationTag::Parking,
         {-10.8f, 0.0f, 6.2f},
         2.2f,
         "E: ustaw slupek",
         "Postaw slupek, zanim ktos zrobi parking bokiem.",
         "Parking ma znowu jakis porzadek."},
        {"road_favor_kierowca",
         "road_service_kierowca",
         "road_favor_kierowca_point",
         WorldLocationTag::RoadLoop,
         {11.8f, 0.0f, -15.1f},
         2.2f,
         "E: odsun trojkat",
         "Odsun trojkat z drogi, bo kazdy tu jedzie na pamiec.",
         "Droga przejezdna, czyli cud."}
    };
    return specs;
}

const LocalRewirServiceSpec* findLocalRewirServiceByPointId(const std::string& interactionPointId) {
    for (const LocalRewirServiceSpec& spec : localRewirServiceCatalogStorage()) {
        if (spec.interactionPointId == interactionPointId) {
            return &spec;
        }
    }

    return nullptr;
}

const LocalRewirServiceSpec* findLocalRewirServiceByInteractionId(const std::string& interactionId) {
    for (const LocalRewirServiceSpec& spec : localRewirServiceCatalogStorage()) {
        if (spec.interactionId == interactionId) {
            return &spec;
        }
    }

    return nullptr;
}

const LocalRewirFavorSpec* findLocalRewirFavorByService(const std::string& serviceInteractionId) {
    for (const LocalRewirFavorSpec& spec : localRewirFavorCatalogStorage()) {
        if (spec.serviceInteractionId == serviceInteractionId) {
            return &spec;
        }
    }

    return nullptr;
}

const LocalRewirFavorSpec* findLocalRewirFavorByPoint(const std::string& interactionPointId) {
    for (const LocalRewirFavorSpec& spec : localRewirFavorCatalogStorage()) {
        if (spec.interactionPointId == interactionPointId) {
            return &spec;
        }
    }

    return nullptr;
}

const LocalRewirFavorSpec* findLocalRewirFavorById(const std::string& id) {
    for (const LocalRewirFavorSpec& spec : localRewirFavorCatalogStorage()) {
        if (spec.id == id) {
            return &spec;
        }
    }

    return nullptr;
}

bool shopRemembersSource(const WorldEventLedger& ledger, const std::string& source) {
    return !ledger.queryByLocationAndSource(WorldLocationTag::Shop, source).empty();
}

bool pressureMatchesLocation(const WorldEventLedger& ledger, Vec3 actorPosition, WorldLocationTag location) {
    const RewirPressureSnapshot pressure = resolveRewirPressure(ledger, actorPosition);
    return pressure.active && pressure.location == location;
}

LocalRewirServiceState resolveLocalRewirServiceStateForSpec(const WorldEventLedger& ledger,
                                                            Vec3 actorPosition,
                                                            const LocalRewirServiceSpec& spec) {
    const bool wary = pressureMatchesLocation(ledger, actorPosition, spec.location);
    return {wary ? LocalRewirServiceAccess::Wary : LocalRewirServiceAccess::Normal,
            spec.interactionPointId,
            spec.interactionId,
            spec.location,
            spec.speaker,
            spec.priority,
            wary ? spec.waryPrompt : spec.normalPrompt,
            wary ? spec.waryLine : spec.normalLine};
}

int softenMemoryForLocation(WorldEventLedger& ledger, WorldLocationTag location) {
    static constexpr float ReliefRemainingSeconds = 2.5f;
    static constexpr float ReliefIntensityMultiplier = 0.35f;

    std::vector<WorldEvent> events = ledger.recentEvents();
    int softened = 0;
    for (WorldEvent& event : events) {
        if (event.location != location) {
            continue;
        }

        event.remainingSeconds = std::min(event.remainingSeconds, ReliefRemainingSeconds);
        event.intensity = std::max(0.0f, event.intensity * ReliefIntensityMultiplier);
        event.stackCount = 1;
        ++softened;
    }

    if (softened > 0) {
        ledger.restoreRecentEvents(std::move(events));
    }

    return softened;
}

const char* reliefFeedbackLineForLocation(WorldLocationTag location) {
    switch (location) {
    case WorldLocationTag::Garage:
        return "Garaz odpuszcza.";
    case WorldLocationTag::Trash:
        return "Altana odpuszcza.";
    case WorldLocationTag::Parking:
        return "Parking odpuszcza.";
    case WorldLocationTag::RoadLoop:
        return "Droga odpuszcza.";
    case WorldLocationTag::Block:
        return "Blok odpuszcza.";
    case WorldLocationTag::Shop:
        return "Sklep odpuszcza.";
    case WorldLocationTag::Unknown:
        break;
    }
    return "Rewir odpuszcza.";
}

} // namespace

const std::vector<LocalRewirServiceSpec>& localRewirServiceCatalog() {
    return localRewirServiceCatalogStorage();
}

const std::vector<LocalRewirFavorSpec>& localRewirFavorCatalog() {
    return localRewirFavorCatalogStorage();
}

std::optional<LocalRewirFavorSpec> localRewirFavorForService(const std::string& serviceInteractionId) {
    const LocalRewirFavorSpec* spec = findLocalRewirFavorByService(serviceInteractionId);
    if (spec == nullptr) {
        return std::nullopt;
    }

    return *spec;
}

std::optional<LocalRewirFavorSpec> localRewirFavorForPoint(const std::string& interactionPointId) {
    const LocalRewirFavorSpec* spec = findLocalRewirFavorByPoint(interactionPointId);
    if (spec == nullptr) {
        return std::nullopt;
    }

    return *spec;
}

std::optional<LocalRewirFavorSpec> localRewirFavorById(const std::string& id) {
    const LocalRewirFavorSpec* spec = findLocalRewirFavorById(id);
    if (spec == nullptr) {
        return std::nullopt;
    }

    return *spec;
}

std::optional<LocalRewirFavorState> resolveLocalRewirFavorState(const WorldEventLedger& ledger,
                                                                Vec3 actorPosition,
                                                                const std::string& interactionPointId) {
    const LocalRewirFavorSpec* favor = findLocalRewirFavorByPoint(interactionPointId);
    if (favor == nullptr) {
        return std::nullopt;
    }

    const std::optional<LocalRewirServiceState> service =
        resolveLocalRewirServiceStateForInteraction(ledger, actorPosition, favor->serviceInteractionId);
    if (!service.has_value() || service->access != LocalRewirServiceAccess::Wary) {
        return std::nullopt;
    }

    return LocalRewirFavorState{
        favor->id,
        favor->serviceInteractionId,
        favor->interactionPointId,
        favor->location,
        favor->prompt,
        favor->startLine,
        favor->completeLine,
        service->priority + 8
    };
}

LocalRewirFavorDigest buildLocalRewirFavorDigest(const WorldEventLedger& ledger,
                                                 const std::vector<std::string>& completedFavorIds) {
    LocalRewirFavorDigest digest;

    for (const LocalRewirFavorSpec& favor : localRewirFavorCatalogStorage()) {
        ++digest.total;
        const bool completed = std::find(completedFavorIds.begin(), completedFavorIds.end(), favor.id) !=
                               completedFavorIds.end();
        if (completed) {
            ++digest.completed;
            continue;
        }

        const std::optional<LocalRewirFavorState> state =
            resolveLocalRewirFavorState(ledger, favor.interactionPosition, favor.interactionPointId);
        if (state.has_value()) {
            ++digest.active;
            if (digest.firstActiveInteractionPointId.empty()) {
                digest.firstActiveInteractionPointId = favor.interactionPointId;
            }
        }
    }

    return digest;
}

LocalRewirServiceDigest buildLocalRewirServiceDigest(const WorldEventLedger& ledger) {
    LocalRewirServiceDigest digest;

    for (const LocalRewirServiceSpec& spec : localRewirServiceCatalogStorage()) {
        ++digest.total;
        const LocalRewirServiceState state =
            resolveLocalRewirServiceStateForSpec(ledger, spec.interactionPosition, spec);
        if (state.access == LocalRewirServiceAccess::Wary) {
            ++digest.wary;
            digest.waryInteractionPointIds.push_back(spec.interactionPointId);
        }
    }

    return digest;
}

LocalRewirServiceReliefResult applyLocalRewirServiceRelief(WorldEventLedger& ledger,
                                                           Vec3 actorPosition,
                                                           const std::string& interactionId) {
    LocalRewirServiceReliefResult result;
    const LocalRewirServiceSpec* spec = findLocalRewirServiceByInteractionId(interactionId);
    if (spec == nullptr) {
        return result;
    }

    result.matchedService = true;
    result.location = spec->location;
    const LocalRewirServiceState state = resolveLocalRewirServiceStateForSpec(ledger, actorPosition, *spec);
    if (state.access != LocalRewirServiceAccess::Wary) {
        return result;
    }

    result.pressureWasActive = true;
    result.softenedEvents = softenMemoryForLocation(ledger, spec->location);
    if (result.softenedEvents > 0) {
        result.feedbackLine = reliefFeedbackLineForLocation(spec->location);
    }
    return result;
}

std::optional<LocalRewirServiceState> resolveLocalRewirServiceState(const WorldEventLedger& ledger,
                                                                    Vec3 actorPosition,
                                                                    const std::string& interactionPointId) {
    const LocalRewirServiceSpec* spec = findLocalRewirServiceByPointId(interactionPointId);
    if (spec == nullptr) {
        return std::nullopt;
    }

    return resolveLocalRewirServiceStateForSpec(ledger, actorPosition, *spec);
}

std::optional<LocalRewirServiceState> resolveLocalRewirServiceStateForInteraction(
    const WorldEventLedger& ledger,
    Vec3 actorPosition,
    const std::string& interactionId) {
    const LocalRewirServiceSpec* spec = findLocalRewirServiceByInteractionId(interactionId);
    if (spec == nullptr) {
        return std::nullopt;
    }

    return resolveLocalRewirServiceStateForSpec(ledger, actorPosition, *spec);
}

ShopServiceState resolveShopServiceState(const WorldEventLedger& ledger, bool storyShopBanActive) {
    if (storyShopBanActive || shopRemembersSource(ledger, "paragon_chaos")) {
        return {ShopServiceAccess::WindowOnly,
                "E: Zenon przez okienko",
                "Zakaz sklepu. Okienko albo won spod lady."};
    }

    if (shopRemembersSource(ledger, "shop_walk_intro") ||
        shopRemembersSource(ledger, "shop_mission") ||
        shopRemembersSource(ledger, "paragon_accusation") ||
        shopRemembersSource(ledger, "paragon_trick")) {
        return {ShopServiceAccess::Wary,
                "E: Zenon patrzy spod lady",
                "Patrze, pamietam i naliczam."};
    }

    return {ShopServiceAccess::Normal,
            "E: pogadaj z Zenonem",
            "Sklep stoi, nerwy tez."};
}

GarageServiceState resolveGarageServiceState(const WorldEventLedger& ledger, Vec3 actorPosition) {
    const std::optional<LocalRewirServiceState> state =
        resolveLocalRewirServiceState(ledger, actorPosition, "garage_rysiek");
    if (state.has_value() && state->access == LocalRewirServiceAccess::Wary) {
        return {GarageServiceAccess::Wary,
                state->prompt,
                state->line};
    }

    return {GarageServiceAccess::Normal,
            state.has_value() ? state->prompt : "E: pogadaj z Ryskiem",
            state.has_value() ? state->line : "Gruz da sie zrobic, tylko nie pytaj jak."};
}

CaretakerServiceState resolveCaretakerServiceState(const WorldEventLedger& ledger, Vec3 actorPosition) {
    const std::optional<LocalRewirServiceState> state =
        resolveLocalRewirServiceState(ledger, actorPosition, "trash_dozorca");
    if (state.has_value() && state->access == LocalRewirServiceAccess::Wary) {
        return {CaretakerServiceAccess::Wary,
                state->prompt,
                state->line};
    }

    return {CaretakerServiceAccess::Normal,
            state.has_value() ? state->prompt : "E: pogadaj z Dozorca",
            state.has_value() ? state->line : "Smietnik spokojny, osiedle oddycha."};
}

} // namespace bs3d
