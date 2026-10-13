#pragma once

namespace ampersand::audio {

/**
 * @brief Global volume multiplier applied to every sound and music.
 *
 * Required by AudioSystem: `world.addResource<AudioListener>()` once, then
 * set `volume` whenever the game changes it (an options menu, for example).
 */
struct AudioListener {
    /** @brief Global volume, from 0 (mute) to 100 (unchanged). */
    float volume{100.0F};
};

}  // namespace ampersand::audio
