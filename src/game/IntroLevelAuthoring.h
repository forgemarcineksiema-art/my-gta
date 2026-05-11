#pragma once

#include "IntroLevelBuilder.h"

namespace bs3d {

class IntroLevelAuthoring {
public:
    static void addCoreLayout(IntroLevelData& level);
    static void addDressing(IntroLevelData& level);
    static void addGameplayData(IntroLevelData& level);
};

} // namespace bs3d
