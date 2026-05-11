#include "WorldObjectInteraction.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace bs3d {

namespace {

bool hasTag(const WorldObject& object, const std::string& tag) {
    return std::find(object.gameplayTags.begin(), object.gameplayTags.end(), tag) != object.gameplayTags.end();
}

float interactionRadiusForObject(const WorldObject& object) {
    const float footprint = std::max(object.scale.x, object.scale.z);
    return std::clamp(footprint + 0.95f, 1.45f, 2.8f);
}

WorldObjectInteractionAffordance affordance(const WorldObject& object,
                                            std::string outcomeId,
                                            std::string prompt,
                                            std::string speaker,
                                            std::string line,
                                            InteractionAction action,
                                            int priority = 20) {
    WorldObjectInteractionAffordance result;
    result.candidateId = "object:" + object.id;
    result.objectId = object.id;
    result.outcomeId = std::move(outcomeId);
    result.prompt = std::move(prompt);
    result.speaker = std::move(speaker);
    result.line = std::move(line);
    result.action = action;
    result.position = object.position;
    result.location = object.zoneTag;
    result.radius = interactionRadiusForObject(object);
    result.priority = priority;
    return result;
}

InteractionCandidate toCandidate(const WorldObjectInteractionAffordance& affordance) {
    return {affordance.candidateId,
            affordance.action,
            affordance.input,
            affordance.prompt,
            affordance.position,
            affordance.radius,
            affordance.priority,
            true};
}

} // namespace

std::optional<WorldObjectInteractionAffordance> worldObjectInteractionAffordance(const WorldObject& object) {
    if (object.id == "shop_rolling_grille") {
        return affordance(object,
                          "shop_rolling_grille_checked",
                          "E: sprawdz rolete",
                          "Roleta",
                          "Porysowana roleta pamieta wiecej zamkniec niz promocji.",
                          InteractionAction::UseDoor,
                          21);
    }
    if (hasTag(object, "shop_price_card")) {
        return affordance(object,
                          "shop_prices_read_" + object.id,
                          "E: przeczytaj ceny",
                          "Kartka",
                          "Promocja wyglada legalnie, dopoki nie policzysz groszy.",
                          InteractionAction::UseMarker,
                          15);
    }
    if (hasTag(object, "readable_notice")) {
        if (hasTag(object, "shop_notice")) {
            return affordance(object,
                              "shop_notice_read_" + object.id,
                              "E: przeczytaj kartke",
                              "Kartka Zenona",
                              "Kartka ma ton urzedowy, ale charakter prywatnej grozby.",
                              InteractionAction::UseMarker,
                              14);
        }
        if (hasTag(object, "block_notice")) {
            return affordance(object,
                              "block_notice_read_" + object.id,
                              "E: przeczytaj ogloszenie",
                              "Ogloszenie",
                              "Spoldzielnia informuje, ze wszystko jest tymczasowe od lat.",
                              InteractionAction::UseMarker,
                              14);
        }
    }
    if (hasTag(object, "intercom")) {
        return affordance(object,
                          "block_intercom_buzzed",
                          "E: nacisnij domofon",
                          "Domofon",
                          "Trzeszczy tak, jakby caly blok mial zaraz odpowiedziec naraz.",
                          InteractionAction::UseMarker,
                          17);
    }
    if (object.id == "sign_no_parking" || hasTag(object, "parking_sign")) {
        return affordance(object,
                          "parking_sign_read_" + object.id,
                          "E: przeczytaj znak",
                          "Znak",
                          "Znak stojacy tu od lat bardziej zabrania niż pomaga.",
                          InteractionAction::UseMarker,
                          15);
    }
    if (object.assetId == "shop_door" || hasTag(object, "entrance")) {
        return affordance(object,
                          "shop_door_checked",
                          "E: sprawdz drzwi",
                          "Drzwi",
                          "Klamka chodzi, ale Zenon i tak woli obslugiwac przez spojrzenie.",
                          InteractionAction::UseDoor,
                          24);
    }
    if (object.assetId == "garage_door_segment" &&
        (object.zoneTag == WorldLocationTag::Garage || hasTag(object, "garage_identity"))) {
        return affordance(object,
                          "garage_door_checked_" + object.id,
                          "E: sprawdz garaz",
                          "Garaz",
                          "Blacha trzeszczy. To brzmi jak przyszla robota dla gruza.",
                          InteractionAction::UseDoor,
                          22);
    }
    if (object.assetId == "bench" || hasTag(object, "bogus")) {
        return affordance(object,
                          "bogus_bench_checked",
                          "E: obejrzyj lawke",
                          "Lawka",
                          "Tu zaczyna sie wiecej spraw, niz powinno sie zaczynac na lawce.",
                          InteractionAction::UseMarker,
                          18);
    }
    if (object.assetId == "trash_bin_lowpoly" || hasTag(object, "trash_dressing")) {
        return affordance(object,
                          "trash_disturbed_" + object.id,
                          "E: rusz smietnik",
                          "Smietnik",
                          "Robi halas wystarczajacy, zeby ktos mial zdanie.",
                          InteractionAction::UseMarker,
                          18);
    }
    if (hasTag(object, "glass_surface")) {
        return affordance(object,
                          "glass_peeked_" + object.id,
                          "E: zajrzyj przez szybe",
                          "Szyba",
                          "Widac sklep, ceny i powod, zeby jeszcze nie robic sceny.",
                          InteractionAction::UseMarker,
                          16);
    }
    return std::nullopt;
}

WorldObjectInteractionAffordance worldObjectAffordanceWithOutcomeData(
    const WorldObjectInteractionAffordance& affordance,
    const ObjectOutcomeCatalogData* outcomeCatalog) {
    WorldObjectInteractionAffordance result = affordance;
    if (outcomeCatalog == nullptr || !outcomeCatalog->loaded) {
        return result;
    }
    const ObjectOutcomeData* outcome = findObjectOutcomeData(*outcomeCatalog, affordance.outcomeId);
    if (outcome == nullptr) {
        return result;
    }
    if (!outcome->speaker.empty()) {
        result.speaker = outcome->speaker;
    }
    if (!outcome->line.empty()) {
        result.line = outcome->line;
    }
    return result;
}

std::optional<WorldObjectInteractionEvent> worldEventForObjectAffordance(
    const WorldObjectInteractionAffordance& /*affordance*/) {
    return std::nullopt;
}

std::optional<WorldObjectInteractionEvent> worldEventForObjectAffordance(
    const WorldObjectInteractionAffordance& affordance,
    const ObjectOutcomeCatalogData* outcomeCatalog) {
    if (outcomeCatalog != nullptr && outcomeCatalog->loaded) {
        if (const ObjectOutcomeData* outcome = findObjectOutcomeData(*outcomeCatalog, affordance.outcomeId)) {
            if (outcome->worldEvent.has_value()) {
                WorldObjectInteractionEvent event;
                event.type = outcome->worldEvent->type;
                event.position = affordance.position;
                event.intensity = outcome->worldEvent->intensity;
                event.source = affordance.outcomeId;
                event.cooldownSeconds = outcome->worldEvent->cooldownSeconds;
                return event;
            }
            return std::nullopt;
        }
    }
    return worldEventForObjectAffordance(affordance);
}

std::vector<InteractionCandidate> buildWorldObjectInteractionCandidates(const std::vector<WorldObject>& objects) {
    std::vector<InteractionCandidate> candidates;
    for (const WorldObject& object : objects) {
        if (const std::optional<WorldObjectInteractionAffordance> affordance =
                worldObjectInteractionAffordance(object)) {
            candidates.push_back(toCandidate(*affordance));
        }
    }
    return candidates;
}

std::optional<WorldObjectInteractionAffordance> findWorldObjectInteractionAffordance(
    const std::vector<WorldObject>& objects,
    const std::string& candidateId) {
    for (const WorldObject& object : objects) {
        const std::optional<WorldObjectInteractionAffordance> affordance =
            worldObjectInteractionAffordance(object);
        if (affordance.has_value() && affordance->candidateId == candidateId) {
            return affordance;
        }
    }
    return std::nullopt;
}

} // namespace bs3d
