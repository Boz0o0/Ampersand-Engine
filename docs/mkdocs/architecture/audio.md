# Audio

The audio module exposes an SFML-backed audio engine through `IAudioEngine`,
plus two ECS components (`AudioSource`, `AudioListener`) the game attaches to
entities to drive it.

## Sounds and voices

A `SoundId` names audio data loaded once with `loadSound`; a `VoiceId` names
one playing instance of it, since the same sound can be played several times
at once (for example, three enemies firing the same laser sound). `play`
starts a new voice and returns its id, which is then used to `stop`, `pause`,
`resume`, or change the volume, pitch or position of that one instance.

```cpp
ampersand::audio::SfmlAudioEngine engine;

const auto laser = engine.loadSound("assets/laser.wav");
const auto voice = engine.play(laser, /*volume*/ 80.0F, /*pitch*/ 1.0F,
                               /*loop*/ false);

engine.setVolume(voice, 50.0F);
if (!engine.isPlaying(voice)) {
    // the voice finished and was dropped; engine.play again to replay it
}
```

A finished, non-looping voice is dropped automatically the next time `play`
is called, so one-shot sounds do not accumulate.

## Music

Longer tracks are opened as a `MusicId` and streamed from disk with
`sf::Music` instead of being fully decoded into memory, through `openMusic`,
`playMusic`, `pauseMusic` and `stopMusic`.

```cpp
const auto theme = engine.openMusic("assets/theme.ogg");
engine.playMusic(theme, 100.0F, /*loop*/ true);
```

## ECS components

`AudioSource` is a plain data component attached to an entity with
`world.add`. The game drives it directly with `IAudioEngine`, storing the
returned `VoiceId` back into the component to control that entity's sound
later:

```cpp
#include <ampersand/audio/AudioSource.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace ecs = ampersand::core::ecs;

ecs::World world;
const ecs::Entity turret = world.create();
world.add(turret, ampersand::audio::AudioSource{.sound = laser});

auto& source = world.get<ampersand::audio::AudioSource>(turret);
source.voice = engine.play(source.sound, source.volume, source.pitch,
                           source.loop);
```

`AudioListener` is an empty tag for the entity whose `Transform` is the
listener (usually the camera or the player). Each frame, the game syncs the
listener and any spatial sources to their entity's position:

```cpp
for (const ecs::Entity entity : world.owners<ampersand::audio::AudioListener>()) {
    const auto& transform = world.get<ampersand::core::Transform>(entity);
    engine.setListenerPosition(transform.position);
}

for (const ecs::Entity entity : world.owners<ampersand::audio::AudioSource>()) {
    auto& source = world.get<ampersand::audio::AudioSource>(entity);
    if (!source.spatial) continue;
    const auto& transform = world.get<ampersand::core::Transform>(entity);
    engine.setPosition(source.voice, transform.position);
}
```

Only one entity should carry `AudioListener` at a time, since the underlying
listener position is global to the audio engine.
