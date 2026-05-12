#pragma once

#include <string>

namespace bs3d {

struct RenderFrame;

enum class RenderFrameDumpVersion {
    V1,
    V2,
};

struct RenderFrameDumpWriteOptions {
    RenderFrameDumpVersion version = RenderFrameDumpVersion::V1;
};

/// Writes a RenderFrame to a simple text file.
/// V1: only camera, Box primitives, and debug lines are serialized.
/// V2: camera, all primitives with MeshHandle/MaterialHandle, and debug lines.
bool writeRenderFrameDump(const RenderFrame& frame, const std::string& path, std::string* error = nullptr);

bool writeRenderFrameDump(const RenderFrame& frame,
                          const std::string& path,
                          const RenderFrameDumpWriteOptions& options,
                          std::string* error = nullptr);

/// Reads a RenderFrame from a text file written by writeRenderFrameDump.
/// Accepts both v1 and v2 headers.
bool readRenderFrameDump(const std::string& path, RenderFrame& frame, std::string* error = nullptr);

} // namespace bs3d
