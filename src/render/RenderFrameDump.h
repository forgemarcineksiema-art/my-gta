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
///
/// V1: only camera, Box primitives, and debug lines are serialized.
///     Non-Box primitive kinds (Mesh, Sphere, CylinderX, QuadPanel) are
///     intentionally skipped.  V1 is the default.
///
/// V2: camera, all primitive kinds with MeshHandle id and MaterialHandle id,
///     and debug lines.  V2 is a metadata-only format:
///     - Does NOT serialize mesh vertex/index geometry.
///     - Does NOT serialize texture paths, material definitions, or assets.
///     - A Mesh command in v2 means "draw mesh handle N at this
///       transform/tint" — it is the renderer's responsibility to have
///       that handle uploaded.  D3D11Renderer only draws its supported
///       subset regardless of what the dump file contains.
bool writeRenderFrameDump(const RenderFrame& frame, const std::string& path, std::string* error = nullptr);

bool writeRenderFrameDump(const RenderFrame& frame,
                          const std::string& path,
                          const RenderFrameDumpWriteOptions& options,
                          std::string* error = nullptr);

/// Reads a RenderFrame from a text file written by writeRenderFrameDump.
/// Accepts both "RenderFrameDump v1" and "RenderFrameDump v2" headers.
/// Unknown versions are rejected with an error.
bool readRenderFrameDump(const std::string& path, RenderFrame& frame, std::string* error = nullptr);

} // namespace bs3d
