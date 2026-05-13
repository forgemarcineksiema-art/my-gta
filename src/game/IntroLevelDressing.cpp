#include "IntroLevelAuthoring.h"

#include "IntroLevelDressingSections.h"

// Art-direction pivot 2026-05: dressing builds the readable osiedle identity
// on top of core layout geometry. Target is stylized mid-poly with modelled
// detail, beveled/chamfered edges, recessed fenestration, shop frontage depth,
// and material identity (concrete, dirty plaster, asphalt, metal, glass).
// Collision stays on core layout objects only; dressing objects are visual.

namespace bs3d {

void IntroLevelAuthoring::addDressing(IntroLevelData& level) {
    IntroLevelDressingSections::addIdentityDressing(level);
    IntroLevelDressingSections::addGroundTruthAndClutter(level);
    IntroLevelDressingSections::addPlaygroundDressing(level);
    IntroLevelDressingSections::addArteryAndPavilionsDressing(level);
    IntroLevelDressingSections::addGarageBeltDressing(level);
}

} // namespace bs3d
