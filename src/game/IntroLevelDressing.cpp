#include "IntroLevelAuthoring.h"

#include "IntroLevelDressingSections.h"

namespace bs3d {

void IntroLevelAuthoring::addDressing(IntroLevelData& level) {
    IntroLevelDressingSections::addIdentityDressing(level);
    IntroLevelDressingSections::addGroundTruthAndClutter(level);
}

} // namespace bs3d
