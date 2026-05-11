#include "WorldAssetRegistry.h"

#include "raylib.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>

namespace bs3d {

namespace {

std::string trim(const std::string& value) {
    const std::string whitespace = " \t\r\n";
    const std::size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return {};
    }
    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::string part;
    std::istringstream stream(value);
    while (std::getline(stream, part, delimiter)) {
        parts.push_back(trim(part));
    }
    return parts;
}

bool parseVec3(const std::string& value, Vec3& out) {
    const std::vector<std::string> parts = split(value, ',');
    if (parts.size() != 3) {
        return false;
    }

    try {
        out = {std::stof(parts[0]), std::stof(parts[1]), std::stof(parts[2])};
    } catch (...) {
        return false;
    }
    return true;
}

bool parseColor(const std::string& value, WorldObjectTint& out) {
    const std::vector<std::string> parts = split(value, ',');
    if (parts.size() != 3 && parts.size() != 4) {
        return false;
    }

    try {
        out.r = static_cast<unsigned char>(std::clamp(std::stoi(parts[0]), 0, 255));
        out.g = static_cast<unsigned char>(std::clamp(std::stoi(parts[1]), 0, 255));
        out.b = static_cast<unsigned char>(std::clamp(std::stoi(parts[2]), 0, 255));
        out.a = parts.size() == 4 ? static_cast<unsigned char>(std::clamp(std::stoi(parts[3]), 0, 255)) : 255;
    } catch (...) {
        return false;
    }
    return true;
}

Color toRayColor(WorldObjectTint tint) {
    return {tint.r, tint.g, tint.b, tint.a};
}

bool parseBool(const std::string& value, bool& out) {
    if (value == "true") {
        out = true;
        return true;
    }
    if (value == "false") {
        out = false;
        return true;
    }
    return false;
}

bool parseFloat(const std::string& value, float& out) {
    try {
        std::size_t consumed = 0;
        out = std::stof(value, &consumed);
        return consumed == value.size() && std::isfinite(out);
    } catch (...) {
        return false;
    }
}

void applyMetadata(const std::string& value,
                   WorldAssetDefinition& definition,
                   std::vector<std::string>& warnings,
                   int lineNumber) {
    const std::vector<std::string> entries = split(value, ';');
    for (const std::string& entry : entries) {
        if (entry.empty()) {
            continue;
        }
        const std::size_t equals = entry.find('=');
        if (equals == std::string::npos) {
            warnings.push_back("bad asset metadata at line " + std::to_string(lineNumber) + ": " + entry);
            continue;
        }

        const std::string key = trim(entry.substr(0, equals));
        const std::string metadataValue = trim(entry.substr(equals + 1));
        if (key == "type") {
            definition.assetType = metadataValue;
        } else if (key == "usage") {
            definition.usage = metadataValue;
        } else if (key == "origin") {
            definition.originPolicy = metadataValue;
        } else if (key == "render") {
            definition.renderBucket = metadataValue;
        } else if (key == "collision") {
            definition.defaultCollision = metadataValue;
        } else if (key == "collisionProfile") {
            definition.defaultCollision = metadataValue;
        } else if (key == "tags") {
            definition.defaultTags = split(metadataValue, ',');
        } else if (key == "cameraBlocks") {
            if (!parseBool(metadataValue, definition.cameraBlocks)) {
                warnings.push_back("bad cameraBlocks metadata at line " + std::to_string(lineNumber) + ": " +
                                   metadataValue);
            }
        } else if (key == "requiresClosedMesh") {
            if (!parseBool(metadataValue, definition.requiresClosedMesh)) {
                warnings.push_back("bad requiresClosedMesh metadata at line " + std::to_string(lineNumber) + ": " +
                                   metadataValue);
            }
        } else if (key == "allowOpenEdges") {
            if (!parseBool(metadataValue, definition.allowOpenEdges)) {
                warnings.push_back("bad allowOpenEdges metadata at line " + std::to_string(lineNumber) + ": " +
                                   metadataValue);
            }
        } else if (key == "allowFloating") {
            if (!parseBool(metadataValue, definition.allowFloating)) {
                warnings.push_back("bad allowFloating metadata at line " + std::to_string(lineNumber) + ": " +
                                   metadataValue);
            }
        } else if (key == "renderInGameplay") {
            if (!parseBool(metadataValue, definition.renderInGameplay)) {
                warnings.push_back("bad renderInGameplay metadata at line " + std::to_string(lineNumber) + ": " +
                                   metadataValue);
            }
        } else if (key == "maxHeight") {
            if (parseFloat(metadataValue, definition.maxHeight)) {
                definition.hasMaxHeight = true;
            } else {
                warnings.push_back("bad maxHeight metadata at line " + std::to_string(lineNumber) + ": " +
                                   metadataValue);
            }
        } else if (key == "visualOffset") {
            Vec3 offset{};
            if (parseVec3(metadataValue, offset)) {
                definition.visualOffset = offset;
                definition.hasVisualOffset = true;
            } else {
                warnings.push_back("bad visualOffset metadata at line " + std::to_string(lineNumber) + ": " +
                                   metadataValue);
            }
        } else {
            warnings.push_back("unknown asset metadata key at line " + std::to_string(lineNumber) + ": " + key);
        }
    }
}

bool pathUsesAuthoredMaterials(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension == ".gltf" || extension == ".glb";
}

std::filesystem::path resolveExistingPath(const std::string& path) {
    std::filesystem::path candidate(path);
    if (std::filesystem::exists(candidate)) {
        return candidate;
    }
    candidate = std::filesystem::path("..") / path;
    if (std::filesystem::exists(candidate)) {
        return candidate;
    }
    candidate = std::filesystem::path("..") / ".." / path;
    return candidate;
}

struct ObjVertexRef {
    int position = -1;
    int texcoord = -1;
    int normal = -1;
};

struct ObjSourceMesh {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec3> texcoords;
    std::vector<std::vector<ObjVertexRef>> faces;
};

bool parseIntStrict(const std::string& text, int& value) {
    try {
        std::size_t consumed = 0;
        value = std::stoi(text, &consumed);
        return consumed == text.size();
    } catch (...) {
        return false;
    }
}

int resolveObjIndex(int index, int count) {
    const int resolved = index < 0 ? count + index : index - 1;
    return resolved >= 0 && resolved < count ? resolved : -1;
}

bool parseObjIndex(const std::string& text, int count, int& value) {
    int raw = 0;
    if (!parseIntStrict(text, raw)) {
        return false;
    }
    value = resolveObjIndex(raw, count);
    return value >= 0;
}

bool parseObjVertexRef(const std::string& token,
                       const ObjSourceMesh& source,
                       ObjVertexRef& ref,
                       const std::filesystem::path& path,
                       int lineNumber,
                       std::vector<std::string>& warnings) {
    const std::vector<std::string> parts = split(token, '/');
    if (parts.empty() || parts[0].empty() ||
        !parseObjIndex(parts[0], static_cast<int>(source.positions.size()), ref.position)) {
        warnings.push_back(path.string() + ":" + std::to_string(lineNumber) +
                           ": face references invalid vertex '" + token + "'");
        return false;
    }
    if (parts.size() >= 2 && !parts[1].empty() &&
        !parseObjIndex(parts[1], static_cast<int>(source.texcoords.size()), ref.texcoord)) {
        warnings.push_back(path.string() + ":" + std::to_string(lineNumber) +
                           ": face references invalid texture coordinate '" + token + "'");
        return false;
    }
    if (parts.size() >= 3 && !parts[2].empty() &&
        !parseObjIndex(parts[2], static_cast<int>(source.normals.size()), ref.normal)) {
        warnings.push_back(path.string() + ":" + std::to_string(lineNumber) +
                           ": face references invalid normal '" + token + "'");
        return false;
    }
    return true;
}

bool parseObjVec3(const std::string& line, Vec3& value) {
    std::istringstream stream(line);
    std::string tag;
    stream >> tag >> value.x >> value.y >> value.z;
    return !stream.fail();
}

bool parseObjTexcoord(const std::string& line, Vec3& value) {
    std::istringstream stream(line);
    std::string tag;
    stream >> tag >> value.x >> value.y;
    value.z = 0.0f;
    return !stream.fail();
}

bool loadObjSource(const std::filesystem::path& path, ObjSourceMesh& source, std::vector<std::string>& warnings) {
    std::ifstream file(path);
    if (!file.is_open()) {
        warnings.push_back("asset OBJ failed to open: " + path.string());
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        if (line.rfind("v ", 0) == 0) {
            Vec3 value{};
            if (parseObjVec3(line, value)) {
                source.positions.push_back(value);
            } else {
                warnings.push_back(path.string() + ":" + std::to_string(lineNumber) + ": malformed vertex line");
            }
        } else if (line.rfind("vn ", 0) == 0) {
            Vec3 value{};
            if (parseObjVec3(line, value)) {
                source.normals.push_back(value);
            } else {
                warnings.push_back(path.string() + ":" + std::to_string(lineNumber) + ": malformed normal line");
            }
        } else if (line.rfind("vt ", 0) == 0) {
            Vec3 value{};
            if (parseObjTexcoord(line, value)) {
                source.texcoords.push_back(value);
            } else {
                warnings.push_back(path.string() + ":" + std::to_string(lineNumber) + ": malformed texture coordinate line");
            }
        } else if (line.rfind("f ", 0) == 0) {
            std::istringstream stream(line);
            std::string tag;
            std::string token;
            std::vector<ObjVertexRef> face;
            bool faceValid = true;
            stream >> tag;
            while (stream >> token) {
                ObjVertexRef ref;
                if (!parseObjVertexRef(token, source, ref, path, lineNumber, warnings)) {
                    faceValid = false;
                }
                face.push_back(ref);
            }
            if (face.size() < 3) {
                warnings.push_back(path.string() + ":" + std::to_string(lineNumber) + ": face has fewer than three vertices");
            } else if (faceValid) {
                source.faces.push_back(std::move(face));
            }
        }
    }

    if (source.positions.empty()) {
        warnings.push_back("asset OBJ has no vertices: " + path.string());
        return false;
    }
    if (source.faces.empty()) {
        warnings.push_back("asset OBJ has no faces: " + path.string());
        return false;
    }
    return true;
}

Vec3 subtract(Vec3 lhs, Vec3 rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vec3 cross(Vec3 lhs, Vec3 rhs) {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

float length(Vec3 value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vec3 normalized(Vec3 value) {
    const float magnitude = length(value);
    if (magnitude <= 0.0001f) {
        return {0.0f, 1.0f, 0.0f};
    }
    return {value.x / magnitude, value.y / magnitude, value.z / magnitude};
}

void appendVertex(const ObjSourceMesh& source,
                  const ObjVertexRef& ref,
                  Vec3 fallbackNormal,
                  std::vector<float>& vertices,
                  std::vector<float>& normals,
                  std::vector<float>& texcoords) {
    const Vec3 position = source.positions[ref.position];
    const Vec3 normal = ref.normal >= 0 ? source.normals[ref.normal] : fallbackNormal;
    const Vec3 texcoord = ref.texcoord >= 0 ? source.texcoords[ref.texcoord] : Vec3{};
    vertices.push_back(position.x);
    vertices.push_back(position.y);
    vertices.push_back(position.z);
    normals.push_back(normal.x);
    normals.push_back(normal.y);
    normals.push_back(normal.z);
    texcoords.push_back(texcoord.x);
    texcoords.push_back(1.0f - texcoord.y);
}

void appendTriangle(const ObjSourceMesh& source,
                    const ObjVertexRef& a,
                    const ObjVertexRef& b,
                    const ObjVertexRef& c,
                    std::vector<float>& vertices,
                    std::vector<float>& normals,
                    std::vector<float>& texcoords) {
    const Vec3 pa = source.positions[a.position];
    const Vec3 pb = source.positions[b.position];
    const Vec3 pc = source.positions[c.position];
    const Vec3 fallbackNormal = normalized(cross(subtract(pb, pa), subtract(pc, pa)));
    appendVertex(source, a, fallbackNormal, vertices, normals, texcoords);
    appendVertex(source, b, fallbackNormal, vertices, normals, texcoords);
    appendVertex(source, c, fallbackNormal, vertices, normals, texcoords);
}

void copyFloats(float*& target, const std::vector<float>& source) {
    target = static_cast<float*>(MemAlloc(static_cast<unsigned int>(sizeof(float) * source.size())));
    std::copy(source.begin(), source.end(), target);
}

Model loadObjModel(const std::filesystem::path& path, std::vector<std::string>& warnings) {
    ObjSourceMesh source;
    if (!loadObjSource(path, source, warnings)) {
        return {};
    }

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texcoords;
    for (const std::vector<ObjVertexRef>& face : source.faces) {
        for (std::size_t index = 1; index + 1 < face.size(); ++index) {
            appendTriangle(source, face[0], face[index], face[index + 1], vertices, normals, texcoords);
        }
    }

    if (vertices.empty() || vertices.size() % 9 != 0) {
        warnings.push_back("asset OBJ produced no valid triangles: " + path.string());
        return {};
    }

    Mesh mesh{};
    mesh.vertexCount = static_cast<int>(vertices.size() / 3);
    mesh.triangleCount = mesh.vertexCount / 3;
    copyFloats(mesh.vertices, vertices);
    copyFloats(mesh.normals, normals);
    copyFloats(mesh.texcoords, texcoords);
    UploadMesh(&mesh, false);
    return LoadModelFromMesh(mesh);
}

bool isObjModelPath(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension == ".obj";
}

bool validateLoadedMeshTopology(const std::string& assetId,
                                const std::filesystem::path& path,
                                int meshIndex,
                                const Mesh& mesh,
                                std::vector<std::string>& warnings) {
    bool ok = true;
    if (mesh.vertexCount < 0 || mesh.triangleCount < 0) {
        warnings.push_back("asset mesh has negative counts: " + assetId + " -> " + path.string());
        assert(false && "negative mesh counts");
        return false;
    }
    if (mesh.vertices == nullptr && mesh.vertexCount > 0) {
        warnings.push_back("asset mesh has vertex count without vertices: " + assetId + " -> " + path.string());
        assert(false && "missing mesh vertices");
        return false;
    }

    Vec3 minPosition{std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max()};
    Vec3 maxPosition{std::numeric_limits<float>::lowest(),
                     std::numeric_limits<float>::lowest(),
                     std::numeric_limits<float>::lowest()};
    for (int vertex = 0; vertex < mesh.vertexCount; ++vertex) {
        const Vec3 position{mesh.vertices[vertex * 3 + 0], mesh.vertices[vertex * 3 + 1], mesh.vertices[vertex * 3 + 2]};
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
            warnings.push_back("asset mesh has non-finite vertex position: " + assetId + " -> " + path.string());
            ok = false;
        }
        minPosition.x = std::min(minPosition.x, position.x);
        minPosition.y = std::min(minPosition.y, position.y);
        minPosition.z = std::min(minPosition.z, position.z);
        maxPosition.x = std::max(maxPosition.x, position.x);
        maxPosition.y = std::max(maxPosition.y, position.y);
        maxPosition.z = std::max(maxPosition.z, position.z);
    }

    const int indexCount = mesh.triangleCount * 3;
    TraceLog(LOG_INFO,
             "MODELTOPOLOGY: asset=%s mesh=%d vertices=%d triangles=%d indices=%d min=(%.3f, %.3f, %.3f) max=(%.3f, %.3f, %.3f)",
             assetId.c_str(),
             meshIndex,
             mesh.vertexCount,
             mesh.triangleCount,
             mesh.indices != nullptr ? indexCount : 0,
             minPosition.x,
             minPosition.y,
             minPosition.z,
             maxPosition.x,
             maxPosition.y,
             maxPosition.z);

    if (mesh.indices != nullptr) {
        for (int index = 0; index < indexCount; ++index) {
            if (mesh.indices[index] >= mesh.vertexCount) {
                warnings.push_back("asset mesh index out of range: " + assetId + " -> " + path.string() +
                                   " mesh=" + std::to_string(meshIndex) +
                                   " index=" + std::to_string(mesh.indices[index]) +
                                   " vertexCount=" + std::to_string(mesh.vertexCount));
                assert(false && "mesh index out of bounds");
                ok = false;
                break;
            }
        }
    } else if (indexCount > mesh.vertexCount) {
        warnings.push_back("asset mesh triangle count exceeds unindexed vertex count: " + assetId + " -> " + path.string());
        assert(false && "unindexed triangle count exceeds vertex count");
        ok = false;
    }
    return ok;
}

bool validateLoadedModelTopology(const std::string& assetId,
                                 const std::filesystem::path& path,
                                 const Model& model,
                                 std::vector<std::string>& warnings) {
    bool ok = true;
    for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex) {
        ok = validateLoadedMeshTopology(assetId, path, meshIndex, model.meshes[meshIndex], warnings) && ok;
    }
    return ok;
}

} // namespace

AssetManifestLoadResult WorldAssetRegistry::loadManifest(const std::string& manifestPath) {
    definitions_.clear();
    indexById_.clear();
    AssetManifestLoadResult result;
    std::filesystem::path resolvedPath(manifestPath);
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        file.clear();
        resolvedPath = std::filesystem::path("..") / manifestPath;
        file.open(resolvedPath);
    }
    if (!file.is_open()) {
        file.clear();
        resolvedPath = std::filesystem::path("..") / ".." / manifestPath;
        file.open(resolvedPath);
    }
    if (!file.is_open()) {
        result.warnings.push_back("asset manifest not found: " + manifestPath);
        return result;
    }

    result.loaded = true;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const std::vector<std::string> parts = split(line, '|');
        if (parts.size() != 4 && parts.size() != 5) {
            result.warnings.push_back("bad asset manifest line " + std::to_string(lineNumber));
            continue;
        }

        WorldAssetDefinition definition;
        definition.id = parts[0];
        definition.modelPath = parts[1];
        if (definition.id.empty() || definition.modelPath.empty()) {
            result.warnings.push_back("empty asset id/path at line " + std::to_string(lineNumber));
            continue;
        }
        if (!parseVec3(parts[2], definition.fallbackSize)) {
            result.warnings.push_back("bad fallback size at line " + std::to_string(lineNumber));
            continue;
        }
        if (!parseColor(parts[3], definition.fallbackColor)) {
            result.warnings.push_back("bad fallback color at line " + std::to_string(lineNumber));
            continue;
        }
        if (definition.fallbackColor.a < 255) {
            definition.renderBucket = "Translucent";
        }
        if (parts.size() == 5) {
            applyMetadata(parts[4], definition, result.warnings, lineNumber);
        }

        if (contains(definition.id)) {
            result.warnings.push_back("duplicate asset id at line " + std::to_string(lineNumber) + ": " + definition.id);
        }
        addDefinition(definition);
    }

    result.entries = static_cast<int>(definitions_.size());
    return result;
}

AssetValidationResult WorldAssetRegistry::validateAssets(const std::string& assetRoot) const {
    AssetValidationResult result;
    const std::filesystem::path root = resolveExistingPath(assetRoot);
    for (const WorldAssetDefinition& definition : definitions_) {
        const std::filesystem::path modelPath = root / definition.modelPath;
        if (!std::filesystem::exists(modelPath)) {
            result.ok = false;
            result.missingAssets.push_back(definition.id);
            result.warnings.push_back("missing asset '" + definition.id + "': " + modelPath.string());
        }
    }
    return result;
}

void WorldAssetRegistry::addDefinition(WorldAssetDefinition definition) {
    const auto existing = indexById_.find(definition.id);
    if (existing != indexById_.end()) {
        definitions_[existing->second] = std::move(definition);
        return;
    }
    indexById_[definition.id] = definitions_.size();
    definitions_.push_back(std::move(definition));
}

bool WorldAssetRegistry::contains(const std::string& id) const {
    return find(id) != nullptr;
}

const WorldAssetDefinition* WorldAssetRegistry::find(const std::string& id) const {
    const auto found = indexById_.find(id);
    return found == indexById_.end() ? nullptr : &definitions_[found->second];
}

const std::vector<WorldAssetDefinition>& WorldAssetRegistry::definitions() const {
    return definitions_;
}

ModelLoadQualityGate evaluateModelLoadQuality(const WorldModelLoadResult& result, bool failOnWarnings) {
    ModelLoadQualityGate gate;
    if (!failOnWarnings || result.warnings.empty()) {
        return gate;
    }
    gate.ok = false;
    gate.message = "model load warnings are fatal in dev/CI:";
    for (const std::string& warning : result.warnings) {
        gate.message += " " + warning;
    }
    return gate;
}

struct WorldModelCache::Impl {
    struct Entry {
        std::string assetId;
        Model model{};
        bool loaded = false;
    };

    std::vector<Entry> entries;
    std::unordered_map<std::string, std::size_t> indexById;
};

WorldModelCache::WorldModelCache()
    : impl_(std::make_unique<Impl>()) {
}

WorldModelLoadResult WorldModelCache::loadAll(const WorldAssetRegistry& registry, const std::string& assetRoot) {
    unload();
    WorldModelLoadResult result;
    const std::filesystem::path root = resolveExistingPath(assetRoot);
    for (const WorldAssetDefinition& definition : registry.definitions()) {
        ++result.requested;
        Impl::Entry entry;
        entry.assetId = definition.id;
        const std::filesystem::path modelPath = root / definition.modelPath;
        if (FileExists(modelPath.string().c_str())) {
            entry.model = isObjModelPath(modelPath)
                              ? loadObjModel(modelPath, result.warnings)
                              : LoadModel(modelPath.string().c_str());
            entry.loaded = entry.model.meshCount > 0;
            if (entry.loaded) {
                if (validateLoadedModelTopology(definition.id, modelPath, entry.model, result.warnings)) {
                    ++result.loaded;
                } else {
                    UnloadModel(entry.model);
                    entry.model = {};
                    entry.loaded = false;
                }
            } else {
                result.warnings.push_back("asset failed to load meshes: " + definition.id);
            }
            if (entry.loaded && !pathUsesAuthoredMaterials(modelPath)) {
                for (int i = 0; i < entry.model.materialCount; ++i) {
                    entry.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = toRayColor(definition.fallbackColor);
                }
            }
        } else {
            result.warnings.push_back("asset file missing: " + definition.id + " -> " + modelPath.string());
        }
        impl_->indexById[entry.assetId] = impl_->entries.size();
        impl_->entries.push_back(entry);
    }
    return result;
}

WorldModelCache::~WorldModelCache() {
    unload();
}

void WorldModelCache::unload() {
    for (Impl::Entry& entry : impl_->entries) {
        if (entry.loaded) {
            UnloadModel(entry.model);
            entry.loaded = false;
        }
    }
    impl_->entries.clear();
    impl_->indexById.clear();
}

const Model* WorldModelCache::findLoadedModel(const std::string& assetId) const {
    const auto found = impl_->indexById.find(assetId);
    if (found == impl_->indexById.end()) {
        return nullptr;
    }
    const Impl::Entry& entry = impl_->entries[found->second];
    return entry.loaded ? &entry.model : nullptr;
}

} // namespace bs3d
