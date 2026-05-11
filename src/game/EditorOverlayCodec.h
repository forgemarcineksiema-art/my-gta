#pragma once

#include "EditorOverlayData.h"

#include <string>
#include <vector>

namespace bs3d {

struct EditorOverlayLoadResult {
    bool success = false;
    EditorOverlayDocument document{};
    std::vector<std::string> warnings;
};

EditorOverlayLoadResult parseEditorOverlay(const std::string& text);
EditorOverlayLoadResult loadEditorOverlayFile(const std::string& path);
std::string serializeEditorOverlay(const EditorOverlayDocument& document);
bool saveEditorOverlayFile(const std::string& path,
                           const EditorOverlayDocument& document,
                           std::vector<std::string>& warnings);

const char* worldLocationTagName(WorldLocationTag tag);
WorldLocationTag worldLocationTagFromName(const std::string& name);

} // namespace bs3d
