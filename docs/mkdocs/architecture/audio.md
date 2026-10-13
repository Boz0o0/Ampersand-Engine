# Audio

The audio module exposes an SFML-backed audio engine through `IAudioEngine`.
For the ECS, it provides one component and two resources so a single
`AudioSystem` can drive every sound and music track without the game calling
the engine itself.

## Concepts

| Name             | Kind      | What it does                                                                 |
| ---------------- | --------- | ------------------------------------------------------------------------------ |
| `AudioSource`    | component | a sound or music an entity plays: `source`, `volume`, `loop`, `playing`       |
| `AudioListener`  | resource  | global volume multiplier (0 to 1) applied to everything                      |
| `SceneMusic`     | resource  | background music for the whole scene, belonging to no entity                 |
| `AudioSystem`    | system    | starts, stops and live-syncs every `AudioSource` and `SceneMusic`, applies the global volume |

`AudioSystem` throws `ResourceNotFound` if the World has no `AudioListener`.
`SceneMusic` is only used if the World has one (like `GravityForce` in
[Physics](physics.md)).

## Example

```cpp
namespace audio = ampersand::audio;

ampersand::audio::SfmlAudioEngine engine;
const auto laser = engine.loadSound("assets/laser.wav");
const auto theme = engine.openMusic("assets/theme.ogg");

world.addResource<audio::AudioListener>().volume = 0.8F;
world.addResource<audio::SceneMusic>() = {.music = theme, .playing = true};

const ecs::Entity turret = world.create();
world.add(turret, audio::AudioSource{.source = audio::SoundRef{laser}});

audio::AudioSystem audioSystem(engine);  // a system, given the engine once

while (running) {
    // fire: the game only flips a flag, the system does the rest
    world.get<audio::AudioSource>(turret).playing = true;
    audioSystem.update(world);
}
```

`AudioSystem` keeps its own bookkeeping (which entity started which voice or
music) in its members, the same way any `ISystem` would (see
[Systems](ecs.md#systems)). It never stores a runtime handle back into the
component: `AudioSource` only ever holds what the game decided (what to
play, how loud, whether it loops, whether it should be playing).

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

`setPosition` and `setListenerPosition` are still there for a game that wants
spatial audio by driving the engine directly; `AudioSource` and
`AudioListener` do not use them (see [ECS integration](#ecs-integration)).

## Music

Longer tracks are opened as a `MusicId` and streamed from disk with
`sf::Music` instead of being fully decoded into memory, through `openMusic`,
`playMusic`, `pauseMusic` and `stopMusic`. `setMusicVolume` changes the
volume of a track that is already playing without restarting it.

```cpp
const auto theme = engine.openMusic("assets/theme.ogg");
engine.playMusic(theme, 100.0F, /*loop*/ true);
engine.setMusicVolume(theme, 60.0F);
```

## ECS integration

### AudioListener: the global volume

`AudioListener` is a resource, not a component: it belongs to no entity, and
there is only one per World. `AudioSystem` applies it every frame through
`IAudioEngine::setGlobalVolume`, which SFML combines with the volume of
every voice and music track:

```cpp
world.addResource<ampersand::audio::AudioListener>();  // volume 1.0 by default
world.getResource<ampersand::audio::AudioListener>().volume = 0.5F;  // an options menu, for example
```

### AudioSource: sounds and music relative to an entity

`AudioSource` plays either a sound effect or a music track, and either kind
can be attached to any entity: `source` holds a `SoundRef` or a `MusicRef`
(`SoundId` and `MusicId` are both plain `std::uint32_t`, so these two
one-field wrappers are what let a `std::variant` tell them apart).

```cpp
#include <ampersand/audio/components/AudioSource.hpp>

namespace audio = ampersand::audio;

const ecs::Entity turret = world.create();
world.add(turret, audio::AudioSource{.source = audio::SoundRef{laser}});

world.get<audio::AudioSource>(turret).playing = true;  // AudioSystem starts it
```

Set `playing` to `true` to start it, `false` to stop it. `AudioSystem`
resets `playing` to `false` on its own once a non-looping sound or track
reaches its end, and always stops the sound of an entity that no longer has
an `AudioSource` (destroyed, or the component removed) — a sound never
outlives the entity it belongs to.

### SceneMusic: music for the whole scene

A music track that is not relative to any entity (a level's background
music) is `SceneMusic`, a resource instead of a component for the same
reason `AudioListener` is: it belongs to no entity, and the scene plays at
most one. It is optional — `AudioSystem` simply does not touch it if the
World has none:

```cpp
world.addResource<ampersand::audio::SceneMusic>() =
    {.music = theme, .volume = 100.0F, .loop = true, .playing = true};
```

It follows the same `playing` convention as `AudioSource`.
