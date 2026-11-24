// WARMFUL — TEXTURE module factory (manual order: Filter, Squash, Cassette, Broken, Interference).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Broken.h"
#include "Cassette.h"
#include "Filter.h"
#include "Interference.h"
#include "Squash.h"

namespace warmful
{

EffectSet createTextureEffects()
{
    EffectSet set;
    set[0] = std::make_unique<FilterEffect>();
    set[1] = std::make_unique<SquashEffect>();
    set[2] = std::make_unique<CassetteEffect>();
    set[3] = std::make_unique<BrokenEffect>();
    set[4] = std::make_unique<InterferenceEffect>();
    return set;
}

} // namespace warmful
