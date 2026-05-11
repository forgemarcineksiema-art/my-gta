#include "MissionRuntimeBridge.h"

namespace bs3d {

MissionRuntimeBridgeResult MissionRuntimeBridge::handleTrigger(const MissionTriggerResult& trigger,
                                                               MissionRuntimeBridgeContext& context) const {
    MissionRuntimeBridgeResult result;
    if (!trigger.triggered) {
        return result;
    }

    switch (trigger.action) {
    case MissionTriggerAction::ShopVisitedOnFoot:
        context.mission.onShopVisitedOnFoot();
        context.story.onShopVisitedOnFoot();
        context.feedback.trigger(FeedbackEvent::MarkerReached);
        if (context.eventCooldowns.consume(WorldEventType::ShopTrouble, "shop_walk_intro", 4.0f)) {
            context.emitWorldEvent(WorldEventType::ShopTrouble, trigger.position, 0.30f, "shop_walk_intro");
        }
        result.handled = true;
        break;
    case MissionTriggerAction::ShopReachedByVehicle:
        context.mission.onShopReachedByVehicle();
        context.feedback.trigger(FeedbackEvent::ChaseWarning);
        if (context.eventCooldowns.consume(WorldEventType::ShopTrouble, "shop_mission", 4.0f)) {
            context.emitWorldEvent(WorldEventType::ShopTrouble, context.vehiclePosition, 0.85f, "shop_mission");
        }
        result.handled = true;
        break;
    case MissionTriggerAction::DropoffReached:
        context.mission.onDropoffReached();
        context.story.onIntroCompleted();
        context.feedback.trigger(FeedbackEvent::MissionComplete);
        result.handled = true;
        result.completed = true;
        break;
    case MissionTriggerAction::None:
        break;
    }

    return result;
}

} // namespace bs3d
