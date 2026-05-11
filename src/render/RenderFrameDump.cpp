#include "RenderFrameDump.h"
#include "bs3d/render/RenderFrame.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

namespace bs3d {

namespace {

bool parseRenderBucket(const std::string& name, RenderBucket& out) {
    if (name == "Sky") { out = RenderBucket::Sky; return true; }
    if (name == "Ground") { out = RenderBucket::Ground; return true; }
    if (name == "Opaque") { out = RenderBucket::Opaque; return true; }
    if (name == "Vehicle") { out = RenderBucket::Vehicle; return true; }
    if (name == "Decal") { out = RenderBucket::Decal; return true; }
    if (name == "Glass") { out = RenderBucket::Glass; return true; }
    if (name == "Translucent") { out = RenderBucket::Translucent; return true; }
    if (name == "Debug") { out = RenderBucket::Debug; return true; }
    if (name == "Hud") { out = RenderBucket::Hud; return true; }
    return false;
}

bool parseRenderPrimitiveKind(const std::string& name, RenderPrimitiveKind& out) {
    if (name == "Box") { out = RenderPrimitiveKind::Box; return true; }
    if (name == "Sphere") { out = RenderPrimitiveKind::Sphere; return true; }
    if (name == "CylinderX") { out = RenderPrimitiveKind::CylinderX; return true; }
    if (name == "QuadPanel") { out = RenderPrimitiveKind::QuadPanel; return true; }
    if (name == "Mesh") { out = RenderPrimitiveKind::Mesh; return true; }
    return false;
}

void writeCamera(std::ostream& out, const RenderCamera& camera) {
    out << "camera pos " << camera.position.x << ' ' << camera.position.y << ' ' << camera.position.z
        << " target " << camera.target.x << ' ' << camera.target.y << ' ' << camera.target.z
        << " up " << camera.up.x << ' ' << camera.up.y << ' ' << camera.up.z
        << " fovy " << camera.fovy << '\n';
}

void writePrimitive(std::ostream& out, const RenderPrimitiveCommand& command) {
    out << "primitive kind " << renderPrimitiveKindName(command.kind)
        << " bucket " << renderBucketName(command.bucket)
        << " pos " << command.transform.position.x << ' ' << command.transform.position.y << ' ' << command.transform.position.z
        << " scale " << command.transform.scale.x << ' ' << command.transform.scale.y << ' ' << command.transform.scale.z
        << " yaw " << command.transform.yawRadians
        << " size " << command.size.x << ' ' << command.size.y << ' ' << command.size.z
        << " tint " << static_cast<int>(command.tint.r) << ' ' << static_cast<int>(command.tint.g)
        << ' ' << static_cast<int>(command.tint.b) << ' ' << static_cast<int>(command.tint.a)
        << " sourceId " << command.sourceId << '\n';
}

void writeDebugLine(std::ostream& out, const RenderLineCommand& line) {
    out << "debugline start " << line.start.x << ' ' << line.start.y << ' ' << line.start.z
        << " end " << line.end.x << ' ' << line.end.y << ' ' << line.end.z
        << " tint " << static_cast<int>(line.tint.r) << ' ' << static_cast<int>(line.tint.g)
        << ' ' << static_cast<int>(line.tint.b) << ' ' << static_cast<int>(line.tint.a) << '\n';
}

bool readCamera(std::istringstream& stream, RenderCamera& camera) {
    std::string token;
    if (!(stream >> token) || token != "pos") return false;
    if (!(stream >> camera.position.x >> camera.position.y >> camera.position.z)) return false;
    if (!(stream >> token) || token != "target") return false;
    if (!(stream >> camera.target.x >> camera.target.y >> camera.target.z)) return false;
    if (!(stream >> token) || token != "up") return false;
    if (!(stream >> camera.up.x >> camera.up.y >> camera.up.z)) return false;
    if (!(stream >> token) || token != "fovy") return false;
    if (!(stream >> camera.fovy)) return false;
    return true;
}

bool readPrimitive(std::istringstream& stream, int lineNumber, RenderPrimitiveCommand& command, std::string* error) {
    std::string token;
    if (!(stream >> token) || token != "kind") return false;
    std::string kindName;
    if (!(stream >> kindName)) return false;
    if (!parseRenderPrimitiveKind(kindName, command.kind)) {
        if (error) {
            *error = "unknown primitive kind '" + kindName + "' on line " + std::to_string(lineNumber);
        }
        return false;
    }

    if (!(stream >> token) || token != "bucket") return false;
    std::string bucketName;
    if (!(stream >> bucketName)) return false;
    if (!parseRenderBucket(bucketName, command.bucket)) {
        if (error) {
            *error = "unknown bucket '" + bucketName + "' on line " + std::to_string(lineNumber);
        }
        return false;
    }

    if (!(stream >> token) || token != "pos") return false;
    if (!(stream >> command.transform.position.x >> command.transform.position.y >> command.transform.position.z)) return false;

    if (!(stream >> token) || token != "scale") return false;
    if (!(stream >> command.transform.scale.x >> command.transform.scale.y >> command.transform.scale.z)) return false;

    if (!(stream >> token) || token != "yaw") return false;
    if (!(stream >> command.transform.yawRadians)) return false;

    if (!(stream >> token) || token != "size") return false;
    if (!(stream >> command.size.x >> command.size.y >> command.size.z)) return false;

    if (!(stream >> token) || token != "tint") return false;
    int r = 0, g = 0, b = 0, a = 0;
    if (!(stream >> r >> g >> b >> a)) return false;
    command.tint.r = static_cast<std::uint8_t>(r);
    command.tint.g = static_cast<std::uint8_t>(g);
    command.tint.b = static_cast<std::uint8_t>(b);
    command.tint.a = static_cast<std::uint8_t>(a);

    if (!(stream >> token) || token != "sourceId") return false;
    std::getline(stream, command.sourceId);
    if (!command.sourceId.empty() && command.sourceId[0] == ' ') {
        command.sourceId.erase(0, 1);
    }
    return true;
}

bool readDebugLine(std::istringstream& stream, RenderLineCommand& line) {
    std::string token;
    if (!(stream >> token) || token != "start") return false;
    if (!(stream >> line.start.x >> line.start.y >> line.start.z)) return false;
    if (!(stream >> token) || token != "end") return false;
    if (!(stream >> line.end.x >> line.end.y >> line.end.z)) return false;
    if (!(stream >> token) || token != "tint") return false;
    int r = 0, g = 0, b = 0, a = 0;
    if (!(stream >> r >> g >> b >> a)) return false;
    line.tint.r = static_cast<std::uint8_t>(r);
    line.tint.g = static_cast<std::uint8_t>(g);
    line.tint.b = static_cast<std::uint8_t>(b);
    line.tint.a = static_cast<std::uint8_t>(a);
    return true;
}

} // namespace

bool writeRenderFrameDump(const RenderFrame& frame, const std::string& path, std::string* error) {
    std::ofstream file(path);
    if (!file.is_open()) {
        if (error) *error = "failed to open file for writing: " + path;
        return false;
    }

    file << "RenderFrameDump v1\n";

    writeCamera(file, frame.camera);

    for (const RenderPrimitiveCommand& command : frame.primitives) {
        if (command.kind != RenderPrimitiveKind::Box) {
            continue;
        }
        writePrimitive(file, command);
    }

    for (const RenderLineCommand& line : frame.debugLines) {
        writeDebugLine(file, line);
    }

    return true;
}

bool readRenderFrameDump(const std::string& path, RenderFrame& frame, std::string* error) {
    std::ifstream file(path);
    if (!file.is_open()) {
        if (error) *error = "failed to open file for reading: " + path;
        return false;
    }

    std::string headerLine;
    if (!std::getline(file, headerLine)) {
        if (error) *error = "missing RenderFrameDump header in: " + path;
        return false;
    }

    {
        std::istringstream headerStream(headerLine);
        std::string keyword;
        std::string version;
        if (!(headerStream >> keyword) || keyword != "RenderFrameDump") {
            if (error) *error = "missing or invalid RenderFrameDump header in: " + path;
            return false;
        }
        if (!(headerStream >> version) || version != "v1") {
            if (error) {
                if (version.empty()) {
                    *error = "missing RenderFrameDump version in header in: " + path;
                } else {
                    *error = "unsupported RenderFrameDump version '" + version + "' in: " + path;
                }
            }
            return false;
        }
    }

    frame = {};

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        std::string type;
        if (!(stream >> type)) {
            continue;
        }

        if (type == "camera") {
            if (!readCamera(stream, frame.camera)) {
                if (error) {
                    *error = "failed to parse camera on line " + std::to_string(lineNumber);
                }
                return false;
            }
        } else if (type == "primitive") {
            RenderPrimitiveCommand command;
            if (!readPrimitive(stream, lineNumber, command, error)) {
                if (error && error->empty()) {
                    *error = "failed to parse primitive on line " + std::to_string(lineNumber);
                }
                return false;
            }
            frame.primitives.push_back(command);
        } else if (type == "debugline") {
            RenderLineCommand lineCommand;
            if (!readDebugLine(stream, lineCommand)) {
                if (error) {
                    *error = "failed to parse debugline on line " + std::to_string(lineNumber);
                }
                return false;
            }
            frame.debugLines.push_back(lineCommand);
        }
    }

    return true;
}

} // namespace bs3d
