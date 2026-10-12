#pragma once

namespace ampersand::audio {

/**
 * @brief Tags the entity whose Transform is the audio listener.
 *
 * Attach this empty tag to the camera or player entity, then each frame
 * call `IAudioEngine::setListenerPosition` with its Transform position so
 * spatial AudioSources are heard relative to it. Only one entity should
 * carry this tag at a time, since the underlying listener is global.
 */
struct AudioListener {};

}  // namespace ampersand::audio
