#include "RenderFrameDump.h"
#include "bs3d/render/RenderFrame.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

namespace bs3d {

namespace {

RenderBucket parseRenderBucket(const std::string& name) {
    if (name == "Sky") return RenderBucket::Sky;
    if (name == "Ground") return RenderBucket::Ground;
    if (name == "Opaque") return RenderBucket::Opaque;
    if (name == "Vehicle") return RenderBucket::Vehicle;
    if (name == "Decal") return RenderBucket::Decal;
    if (name == "Glass") return RenderBucket::Glass;
    if (name == "Translucent") return RenderBucket::Translucent;
    if (name == "Debug") return RenderBucket::Debug;
    if (name == "Hud") return RenderBucket::Hud;
    return RenderBucket::Opaque;
}

RenderPrimitiveKind parseRenderPrimitiveKind(const std::string& name) {
    if (name == "Box") return RenderPrimitiveKind::Box;
    if (name == "Sphere") return RenderPrimitiveKind::Sphere;
    if (name == "CylinderX") return RenderPrimitiveKind::CylinderX;
    if (name == "QuadPanel") return RenderPrimitiveKind::QuadPanel;
    if (name == "Mesh") return RenderPrimitiveKind::Mesh;
    return RenderPrimitiveKind::Box;
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

bool readPrimitive(std::istringstream& stream, RenderPrimitiveCommand& command) {
    std::string token;
    if (!(stream >> token) || token != "kind") return false;
    std::string kindName;
    if (!(stream >> kindName)) return false;
    command.kind = parseRenderPrimitiveKind(kindName);

    if (!(stream >> token) || token != "bucket") return false;
    std::string bucketName;
    if (!(stream >> bucketName)) return false;
    command.bucket = parseRenderBucket(bucketName);

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
            if (!readPrimitive(stream, command)) {
                if (error) {
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
