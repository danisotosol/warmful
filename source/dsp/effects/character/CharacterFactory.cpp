// WARMFUL — CHARACTER module factory (manual order: Drive, Sweeten, Fuzz, Howl, Swell).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Drive.h"
#include "Fuzz.h"
#include "Howl.h"
#include "Sweeten.h"
#include "Swell.h"

namespace warmful
{

EffectSet createCharacterEffects()
{
    EffectSet set;
    set[0] = std::make_unique<DriveEffect>();
    set[1] = std::make_unique<SweetenEffect>();
    set[2] = std::make_unique<FuzzEffect>();
    set[3] = std::make_unique<HowlEffect>();
    set[4] = std::make_unique<SwellEffect>();
    return set;
}

} // namespace warmful
