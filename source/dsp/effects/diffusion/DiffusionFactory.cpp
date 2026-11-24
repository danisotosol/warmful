// WARMFUL — factory for the Diffusion module's five effects, in the manual's order.
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Cascade.h"
#include "Collage.h"
#include "Reels.h"
#include "Reverse.h"
#include "Space.h"

namespace warmful
{

EffectSet createDiffusionEffects()
{
    EffectSet set;
    set[0] = std::make_unique<CascadeEffect>();
    set[1] = std::make_unique<ReelsEffect>();
    set[2] = std::make_unique<SpaceEffect>();
    set[3] = std::make_unique<CollageEffect>();
    set[4] = std::make_unique<ReverseEffect>();
    return set;
}

} // namespace warmful
