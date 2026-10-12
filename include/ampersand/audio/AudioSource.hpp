#pragma once

#include <ampersand/audio/Sound.hpp>
#include <ampersand/audio/Voice.hpp>

namespace ampersand::audio {

/**
 * @brief Sound an entity can play through the audio engine.
 *
 * A plain data component: attach it with `world.add`, then drive it with
 * `IAudioEngine` (`play` using its `sound`, `volume`, `pitch` and `loop`
 * fields, storing the returned VoiceId back into `voice` to `stop` or
 * update it later). If `spatial` is true, the game should keep the voice's
 * position synced to the entity's Transform with `setPosition`.
 */
struct AudioSource {
    SoundId sound{};
    float volume{100.0F};
    float pitch{1.0F};
    bool loop{false};
    bool spatial{false};
    VoiceId voice{};
};

}  // namespace ampersand::audio
