#pragma once

#include "bs3d/core/WorldEventLedger.h"

#include <optional>
#include <string>
#include <vector>

namespace bs3d {

enum class ShopServiceAccess {
    Normal,
    Wary,
    WindowOnly
};

enum class GarageServiceAccess {
    Normal,
    Wary
};

enum class CaretakerServiceAccess {
    Normal,
    Wary
};

enum class LocalRewirServiceAccess {
    Normal,
    Wary
};

struct ShopServiceState {
    ShopServiceAccess access = ShopServiceAccess::Normal;
    std::string prompt;
    std::string line;
};

struct GarageServiceState {
    GarageServiceAccess access = GarageServiceAccess::Normal;
    std::string prompt;
    std::string line;
};

struct CaretakerServiceState {
    CaretakerServiceAccess access = CaretakerServiceAccess::Normal;
    std::string prompt;
    std::string line;
};

struct LocalRewirServiceSpec {
    std::string interactionPointId;
    std::string interactionId;
    WorldLocationTag location = WorldLocationTag::Unknown;
    Vec3 interactionPosition{};
    float interactionRadius = 0.0f;
    std::string speaker;
    int priority = 0;
    std::string normalPrompt;
    std::string normalLine;
    std::string waryPrompt;
    std::string waryLine;
};

struct LocalRewirServiceState {
    LocalRewirServiceAccess access = LocalRewirServiceAccess::Normal;
    std::string interactionPointId;
    std::string interactionId;
    WorldLocationTag location = WorldLocationTag::Unknown;
    std::string speaker;
    int priority = 0;
    std::string prompt;
    std::string line;
};

struct LocalRewirFavorSpec {
    std::string id;
    std::string serviceInteractionId;
    std::string interactionPointId;
    WorldLocationTag location = WorldLocationTag::Unknown;
    Vec3 interactionPosition{};
    float interactionRadius = 0.0f;
    std::string prompt;
    std::string startLine;
    std::string completeLine;
};

struct LocalRewirFavorState {
    std::string id;
    std::string serviceInteractionId;
    std::string interactionPointId;
    WorldLocationTag location = WorldLocationTag::Unknown;
    std::string prompt;
    std::string startLine;
    std::string completeLine;
    int priority = 0;
};

struct LocalRewirFavorDigest {
    int total = 0;
    int active = 0;
    int completed = 0;
    std::string firstActiveInteractionPointId;
};

struct LocalRewirServiceDigest {
    int total = 0;
    int wary = 0;
    std::vector<std::string> waryInteractionPointIds;
};

struct LocalRewirServiceReliefResult {
    bool matchedService = false;
    bool pressureWasActive = false;
    int softenedEvents = 0;
    WorldLocationTag location = WorldLocationTag::Unknown;
    std::string feedbackLine;
};

const std::vector<LocalRewirServiceSpec>& localRewirServiceCatalog();
const std::vector<LocalRewirFavorSpec>& localRewirFavorCatalog();
std::optional<LocalRewirFavorSpec> localRewirFavorForService(const std::string& serviceInteractionId);
std::optional<LocalRewirFavorSpec> localRewirFavorForPoint(const std::string& interactionPointId);
std::optional<LocalRewirFavorSpec> localRewirFavorById(const std::string& id);
std::optional<LocalRewirFavorState> resolveLocalRewirFavorState(const WorldEventLedger& ledger,
                                                                Vec3 actorPosition,
                                                                const std::string& interactionPointId);
LocalRewirFavorDigest buildLocalRewirFavorDigest(const WorldEventLedger& ledger,
                                                 const std::vector<std::string>& completedFavorIds);
LocalRewirServiceDigest buildLocalRewirServiceDigest(const WorldEventLedger& ledger);
LocalRewirServiceReliefResult applyLocalRewirServiceRelief(WorldEventLedger& ledger,
                                                           Vec3 actorPosition,
                                                           const std::string& interactionId);
std::optional<LocalRewirServiceState> resolveLocalRewirServiceState(const WorldEventLedger& ledger,
                                                                    Vec3 actorPosition,
                                                                    const std::string& interactionPointId);
std::optional<LocalRewirServiceState> resolveLocalRewirServiceStateForInteraction(
    const WorldEventLedger& ledger,
    Vec3 actorPosition,
    const std::string& interactionId);

ShopServiceState resolveShopServiceState(const WorldEventLedger& ledger, bool storyShopBanActive);
GarageServiceState resolveGarageServiceState(const WorldEventLedger& ledger, Vec3 actorPosition);
CaretakerServiceState resolveCaretakerServiceState(const WorldEventLedger& ledger, Vec3 actorPosition);

} // namespace bs3d
