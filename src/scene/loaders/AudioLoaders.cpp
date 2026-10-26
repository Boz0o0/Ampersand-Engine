#ifdef AMPERSAND_MODULE_AUDIO

#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/audio/resources/SceneMusic.hpp>
#include <ampersand/scene/resources/Backends.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <memory>
#include <stdexcept>

namespace ampersand::scene {

void SceneManager::loadAudioListener(core::ecs::World& world,
                                     const json& data) {
    std::shared_ptr<audio::IAudioEngine> engine;
    const auto name = data.at("backend").get<std::string>();
    if (name == "sfml") {
        engine = std::make_shared<audio::SfmlAudioEngine>();
    } else {
        throw std::runtime_error("Unknown audio backend '" + name + "'");
    }
    world.addResource<audio::AudioListener>(*engine,
                                            data.value("volume", 100.0F));
    if (!world.hasResource<Backends>()) {
        world.addResource<Backends>();
    }
    world.getResource<Backends>().owned["AudioListener"] = engine;
}

void SceneManager::removeAudioListener(core::ecs::World& world) {
    if (world.hasResource<audio::AudioListener>()) {
        world.removeResource<audio::AudioListener>();
    }
    if (world.hasResource<Backends>()) {
        world.getResource<Backends>().owned.erase("AudioListener");
    }
    forgetAssets("sound");
    forgetAssets("music");
}

std::uint32_t SceneManager::audioAsset(audio::IAudioEngine& engine,
                                       const std::string& kind,
                                       const std::string& path) {
    return asset(kind, path, [&engine, &kind](const std::string& full) {
        return kind == "sound" ? engine.loadSound(full)
                               : engine.openMusic(full);
    });
}

void SceneManager::loadSceneMusic(core::ecs::World& world, const json& data) {
    auto& engine = world.getResource<audio::AudioListener>().engine;
    audio::SceneMusic music;
    music.music =
        audioAsset(engine, "music", data.at("music").get<std::string>());
    music.volume = data.value("volume", music.volume);
    music.loop = data.value("loop", music.loop);
    music.playing = data.value("playing", music.playing);
    world.addResource<audio::SceneMusic>(music);
}

void SceneManager::loadAudioSource(core::ecs::World& world,
                                   core::ecs::Entity entity, const json& data) {
    auto& engine = world.getResource<audio::AudioListener>().engine;
    audio::AudioSource source;
    if (data.contains("sound") == data.contains("music")) {
        throw std::runtime_error("AudioSource needs either 'sound' or 'music'");
    }
    if (data.contains("sound")) {
        source.source = audio::SoundRef{
            audioAsset(engine, "sound", data.at("sound").get<std::string>()),
        };
    } else {
        source.source = audio::MusicRef{
            audioAsset(engine, "music", data.at("music").get<std::string>()),
        };
    }
    data.at("volume").get_to(source.volume);
    data.at("loop").get_to(source.loop);
    data.at("playing").get_to(source.playing);
    world.add(entity, source);
}

void SceneManager::registerAudio() {
    registerResource(
        "AudioListener", loadAudioListener,
        [this](core::ecs::World& world) { removeAudioListener(world); },
        ResourceScope::Config);
    registerResource<audio::SceneMusic>(
        "SceneMusic", [this](core::ecs::World& world, const json& data) {
            loadSceneMusic(world, data);
        });
    registerComponent(
        "AudioSource",
        [this](core::ecs::World& world, core::ecs::Entity entity,
               const json& data) { loadAudioSource(world, entity, data); });
}

}  // namespace ampersand::scene

#endif
