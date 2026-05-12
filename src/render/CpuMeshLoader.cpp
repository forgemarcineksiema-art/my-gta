#include "CpuMeshLoader.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace bs3d {

namespace {

int parseFacePartIndex(const std::string& token) {
    if (token.empty()) {
        return 0;
    }
    const auto slashPos = token.find('/');
    const std::string indexStr = (slashPos == std::string::npos) ? token : token.substr(0, slashPos);
    if (indexStr.empty()) {
        return 0;
    }
    char* end = nullptr;
    const long value = std::strtol(indexStr.c_str(), &end, 10);
    if (end != indexStr.c_str() + indexStr.size()) {
        return 0;
    }
    if (value > 2147483647L) {
        return -1;
    }
    return static_cast<int>(value);
}

} // namespace

CpuMeshLoadResult loadCpuMeshFromObjText(const std::string& text, const std::string& debugName) {
    CpuMeshLoadResult result;
    std::vector<CpuMeshVertex> vertices;
    std::vector<std::uint16_t> faceIndices;

    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        if (prefix == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            lineStream >> x >> y >> z;
            vertices.push_back({{x, y, z}});
        } else if (prefix == "f") {
            std::vector<int> faceParts;
            std::string token;
            while (lineStream >> token) {
                const int index = parseFacePartIndex(token);
                faceParts.push_back(index);
            }

            if (faceParts.size() < 3 || faceParts.size() > 4) {
                result.error = "face with " + std::to_string(faceParts.size()) + " vertices (expected 3 or 4)";
                return result;
            }

            // Resolve to temp array and validate
            std::vector<int> resolved;
            for (const int idx : faceParts) {
                if (idx == 0) {
                    result.error = "face references index 0";
                    return result;
                }
                if (idx < 0) {
                    result.error = "face references negative index " + std::to_string(idx);
                    return result;
                }
                const int zeroBased = idx - 1;
                if (zeroBased < 0 || zeroBased >= static_cast<int>(vertices.size())) {
                    result.error = "face references out-of-range index " + std::to_string(idx);
                    return result;
                }
                resolved.push_back(zeroBased);
            }

            if (faceParts.size() == 3) {
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[0]));
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[1]));
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[2]));
            } else {
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[0]));
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[1]));
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[2]));
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[0]));
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[2]));
                faceIndices.push_back(static_cast<std::uint16_t>(resolved[3]));
            }
        }
    }

    if (vertices.empty()) {
        result.error = "no vertices found";
        return result;
    }
    if (faceIndices.empty()) {
        result.error = "no faces found";
        return result;
    }

    result.mesh.vertices = std::move(vertices);
    result.mesh.indices = std::move(faceIndices);
    result.mesh.debugName = debugName;
    result.ok = true;
    return result;
}

CpuMeshLoadResult loadCpuMeshFromObjFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        CpuMeshLoadResult result;
        result.error = "cannot open file: " + path;
        return result;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return loadCpuMeshFromObjText(buffer.str(), path);
}

} // namespace bs3d
