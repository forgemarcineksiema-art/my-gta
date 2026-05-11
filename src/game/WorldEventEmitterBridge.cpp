#include "WorldEventEmitterBridge.h"

namespace bs3d {

bool WorldEventEmitterBridge::emit(WorldEventLedger& ledger,
                                   PrzypalSystem& przypal,
                                   GameFeedback& feedback,
                                   std::string& lastReason,
                                   WorldEventType type,
                                   WorldLocationTag location,
                                   Vec3 position,
                                   float intensity,
                                   const std::string& source) const {
    WorldEvent event;
    event.type = type;
    event.location = location;
    event.position = position;
    event.intensity = intensity;
    event.source = source;

    const WorldEventAddResult result = ledger.add(event);
    if (!result.heatPulseAccepted || result.heatMultiplier <= 0.0f) {
        return false;
    }

    przypal.onWorldEvent(result);
    lastReason = std::string(eventName(result.type)) + " @ " + locationName(result.location);
    if (przypal.consumeBandPulse()) {
        feedback.trigger(FeedbackEvent::PrzypalNotice);
    }
    return true;
}

const char* WorldEventEmitterBridge::eventName(WorldEventType type) {
    switch (type) {
    case WorldEventType::PublicNuisance:
        return "PublicNuisance";
    case WorldEventType::PropertyDamage:
        return "PropertyDamage";
    case WorldEventType::ChaseSeen:
        return "ChaseSeen";
    case WorldEventType::ShopTrouble:
        return "ShopTrouble";
    }

    return "Unknown";
}

const char* WorldEventEmitterBridge::locationName(WorldLocationTag location) {
    switch (location) {
    case WorldLocationTag::Unknown:
        return "Unknown";
    case WorldLocationTag::Block:
        return "Block";
    case WorldLocationTag::Shop:
        return "Shop";
    case WorldLocationTag::Parking:
        return "Parking";
    case WorldLocationTag::Garage:
        return "Garage";
    case WorldLocationTag::Trash:
        return "Trash";
    case WorldLocationTag::RoadLoop:
        return "RoadLoop";
    }

    return "Unknown";
}

} // namespace bs3d
