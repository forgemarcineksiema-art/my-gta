#include "bs3d/core/CharacterStatusPolicy.h"

namespace bs3d {

bool characterShouldBeScared(bool chaseActive, PrzypalBand przypalBand) {
    return chaseActive ||
           przypalBand == PrzypalBand::ChaseRisk ||
           przypalBand == PrzypalBand::Meltdown;
}

} // namespace bs3d
