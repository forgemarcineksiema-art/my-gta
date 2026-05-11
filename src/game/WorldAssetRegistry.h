#pragma once

#include "WorldArtTypes.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Model;

namespace bs3d {

struct WorldAssetDefinition {
    std::string id;
    std::string modelPath;
    Vec3 fallbackSize{1.0f, 1.0f, 1.0f};
    WorldObjectTint fallbackColor{180, 180, 180, 255};
    std::string assetType{"SolidProp"};
    std::string usage{"active"};
    std::string originPolicy{"bottom_center"};
    std::string renderBucket{"Opaque"};
    std::string defaultCollision{"Unspecified"};
    std::vector<std::string> defaultTags;
    bool cameraBlocks = false;
    bool requiresClosedMesh = false;
    bool allowOpenEdges = false;
    bool allowFloating = false;
    bool renderInGameplay = true;
    bool hasMaxHeight = false;
    float maxHeight = 0.0f;
    bool hasVisualOffset = false;
    Vec3 visualOffset{};
};

struct AssetManifestLoadResult {
    bool loaded = false;
    int entries = 0;
    std::vector<std::string> warnings;
};

struct AssetValidationResult {
    bool ok = true;
    std::vector<std::string> missingAssets;
    std::vector<std::string> warnings;
};

struct WorldModelLoadResult {
    int requested = 0;
    int loaded = 0;
    std::vector<std::string> warnings;
};

struct ModelLoadQualityGate {
    bool ok = true;
    std::string message;
};

ModelLoadQualityGate evaluateModelLoadQuality(const WorldModelLoadResult& result, bool failOnWarnings);

class WorldAssetRegistry {
public:
    AssetManifestLoadResult loadManifest(const std::string& manifestPath);
    AssetValidationResult validateAssets(const std::string& assetRoot) const;
    void addDefinition(WorldAssetDefinition definition);

    bool contains(const std::string& id) const;
    const WorldAssetDefinition* find(const std::string& id) const;
    const std::vector<WorldAssetDefinition>& definitions() const;

private:
    std::vector<WorldAssetDefinition> definitions_;
    std::unordered_map<std::string, std::size_t> indexById_;
};

class WorldModelCache {
public:
    WorldModelCache();
    ~WorldModelCache();
    WorldModelCache(const WorldModelCache&) = delete;
    WorldModelCache& operator=(const WorldModelCache&) = delete;

    WorldModelLoadResult loadAll(const WorldAssetRegistry& registry, const std::string& assetRoot);
    void unload();
    const Model* findLoadedModel(const std::string& assetId) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace bs3d
