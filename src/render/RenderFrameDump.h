#pragma once

#include <string>

namespace bs3d {

struct RenderFrame;

/// Writes a RenderFrame to a simple text file.
/// Only camera, Box primitives, and debug lines are serialized.
bool writeRenderFrameDump(const RenderFrame& frame, const std::string& path, std::string* error = nullptr);

/// Reads a RenderFrame from a text file written by writeRenderFrameDump.
bool readRenderFrameDump(const std::string& path, RenderFrame& frame, std::string* error = nullptr);

} // namespace bs3d
