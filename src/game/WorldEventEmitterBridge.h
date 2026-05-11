#pragma once

#include "bs3d/core/GameFeedback.h"
#include "bs3d/core/PrzypalSystem.h"
#include "bs3d/core/WorldEventLedger.h"

#include <string>

namespace bs3d {

class WorldEventEmitterBridge {
public:
    bool emit(WorldEventLedger& ledger,
              PrzypalSystem& przypal,
              GameFeedback& feedback,
              std::string& lastReason,
              WorldEventType type,
              WorldLocationTag location,
              Vec3 position,
              float intensity,
              const std::string& source) const;

private:
    static const char* eventName(WorldEventType type);
    static const char* locationName(WorldLocationTag location);
};

} // namespace bs3d
