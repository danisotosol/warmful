// WARMFUL — Movement module factory (manual order).
// SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Doubler.h"
#include "Phaser.h"
#include "Pitch.h"
#include "Tremolo.h"
#include "Vibrato.h"

namespace warmful
{

EffectSet createMovementEffects()
{
    EffectSet set;
    set[0] = std::make_unique<DoublerEffect>();
    set[1] = std::make_unique<VibratoEffect>();
    set[2] = std::make_unique<PhaserEffect>();
    set[3] = std::make_unique<TremoloEffect>();
    set[4] = std::make_unique<PitchEffect>();
    return set;
}

} // namespace warmful
