#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/audio/resources/SceneMusic.hpp>
#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <variant>

namespace ampersand::audio {

AudioSystem::AudioSystem(IAudioEngine& engine) : engine_(engine) {}

void AudioSystem::update(core::ecs::World& world) {
    engine_.setGlobalVolume(world.getResource<AudioListener>().volume * 100.0F);

    this->stopOrphanedSounds(world);

    for (const core::ecs::Entity entity : world.owners<AudioSource>()) {
        auto& source = world.get<AudioSource>(entity);
        if (std::holds_alternative<SoundRef>(source.source)) {
            this->updateSoundSource(entity, source);
        } else {
            this->updateMusicSource(entity, source);
        }
    }

    if (world.hasResource<SceneMusic>()) {
        auto& scene = world.getResource<SceneMusic>();
        if (scene.playing) {
            if (!this->sceneMusicActive_) {
                engine_.playMusic(scene.music, scene.volume, scene.loop);
                this->sceneMusicActive_ = true;
            } else if (engine_.isMusicPlaying(scene.music)) {
                engine_.setMusicVolume(scene.music, scene.volume);
            } else {
                this->sceneMusicActive_ = false;
                scene.playing = false;
            }
        } else if (this->sceneMusicActive_) {
            engine_.stopMusic(scene.music);
            this->sceneMusicActive_ = false;
        }
    }
}

void AudioSystem::stopOrphanedSounds(core::ecs::World& world) {
    for (auto voiceIt = this->activeVoices_.begin();
         voiceIt != this->activeVoices_.end();) {
        if (world.alive(voiceIt->first) &&
            world.has<AudioSource>(voiceIt->first)) {
            ++voiceIt;
        } else {
            engine_.stop(voiceIt->second.second);
            voiceIt = this->activeVoices_.erase(voiceIt);
        }
    }

    for (auto musicIt = this->activeMusic_.begin();
         musicIt != this->activeMusic_.end();) {
        if (world.alive(musicIt->first) &&
            world.has<AudioSource>(musicIt->first)) {
            ++musicIt;
        } else {
            engine_.stopMusic(musicIt->second);
            musicIt = this->activeMusic_.erase(musicIt);
        }
    }
}

void AudioSystem::updateSoundSource(core::ecs::Entity entity,
                                    AudioSource& source) {
    const auto sound = std::get<SoundRef>(source.source).id;
    const auto voiceIt = this->activeVoices_.find(entity);
    const bool tracksThisSound =
        voiceIt != this->activeVoices_.end() && voiceIt->second.first == sound;

    if (source.playing) {
        if (!tracksThisSound) {
            if (voiceIt != this->activeVoices_.end()) {
                engine_.stop(voiceIt->second.second);  // retargeted mid-play
            }
            const VoiceId voice =
                engine_.play(sound, source.volume, 1.0F, source.loop);
            this->activeVoices_[entity] = {sound, voice};
        } else if (engine_.isPlaying(voiceIt->second.second)) {
            engine_.setVolume(voiceIt->second.second, source.volume);
        } else {
            this->activeVoices_.erase(voiceIt);
            source.playing = false;
        }
    } else if (voiceIt != this->activeVoices_.end()) {
        engine_.stop(voiceIt->second.second);
        this->activeVoices_.erase(voiceIt);
    }
}

void AudioSystem::updateMusicSource(core::ecs::Entity entity,
                                    AudioSource& source) {
    const auto music = std::get<MusicRef>(source.source).id;
    const auto musicIt = this->activeMusic_.find(entity);
    const bool tracksThisMusic =
        musicIt != this->activeMusic_.end() && musicIt->second == music;

    if (source.playing) {
        if (!tracksThisMusic) {
            if (musicIt != this->activeMusic_.end()) {
                engine_.stopMusic(musicIt->second);  // retargeted mid-play
            }
            engine_.playMusic(music, source.volume, source.loop);
            this->activeMusic_[entity] = music;
        } else if (engine_.isMusicPlaying(music)) {
            engine_.setMusicVolume(music, source.volume);
        } else {
            this->activeMusic_.erase(musicIt);
            source.playing = false;
        }
    } else if (musicIt != this->activeMusic_.end()) {
        engine_.stopMusic(musicIt->second);
        this->activeMusic_.erase(musicIt);
    }
}

}  // namespace ampersand::audio
