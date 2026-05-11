#pragma once

#include "bs3d/core/PlayerPresentation.h"

#include <string>

namespace bs3d {

class CharacterPosePreview {
public:
    bool active() const;
    int activeIndex() const;
    int count() const;
    CharacterPoseKind activePose() const;
    std::string activeLabel() const;

    void toggle();
    void next();
    void previous();

    PlayerPresentationState activePreviewState() const;
    PlayerPresentationState previewStateFor(CharacterPoseKind poseKind) const;

private:
    bool active_ = false;
    int activeIndex_ = 0;
};

std::string characterPoseLabel(CharacterPoseKind poseKind);

} // namespace bs3d
