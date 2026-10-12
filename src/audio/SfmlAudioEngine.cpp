#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <utility>

namespace ampersand::audio {

SoundId SfmlAudioEngine::loadSound(const std::filesystem::path& path) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(path)) return 0;

    const SoundId soundId = this->nextSoundId_++;
    this->buffers_.emplace(soundId, std::move(buffer));
    return soundId;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
VoiceId SfmlAudioEngine::play(SoundId sound, float volume, float pitch,
                              bool loop) {
    this->pruneFinishedVoices();

    const auto buffer = this->buffers_.find(sound);
    if (buffer == this->buffers_.end()) return 0;

    auto voice = std::make_unique<sf::Sound>(buffer->second);
    voice->setVolume(volume);
    voice->setPitch(pitch);
    voice->setLooping(loop);
    voice->play();

    const VoiceId voiceId = this->nextVoiceId_++;
    this->voices_.emplace(voiceId, std::move(voice));
    return voiceId;
}

void SfmlAudioEngine::stop(VoiceId voice) {
    const auto voiceIt = this->voices_.find(voice);
    if (voiceIt == this->voices_.end()) return;
    voiceIt->second->stop();
    this->voices_.erase(voiceIt);
}

void SfmlAudioEngine::pause(VoiceId voice) {
    const auto voiceIt = this->voices_.find(voice);
    if (voiceIt != this->voices_.end()) voiceIt->second->pause();
}

void SfmlAudioEngine::resume(VoiceId voice) {
    const auto voiceIt = this->voices_.find(voice);
    if (voiceIt != this->voices_.end()) voiceIt->second->play();
}

bool SfmlAudioEngine::isPlaying(VoiceId voice) const {
    const auto voiceIt = this->voices_.find(voice);
    return voiceIt != this->voices_.end() &&
           voiceIt->second->getStatus() == sf::Sound::Status::Playing;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SfmlAudioEngine::setVolume(VoiceId voice, float volume) {
    const auto voiceIt = this->voices_.find(voice);
    if (voiceIt != this->voices_.end()) voiceIt->second->setVolume(volume);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SfmlAudioEngine::setPitch(VoiceId voice, float pitch) {
    const auto voiceIt = this->voices_.find(voice);
    if (voiceIt != this->voices_.end()) voiceIt->second->setPitch(pitch);
}

void SfmlAudioEngine::setPosition(VoiceId voice,
                                  const core::math::Vec3f& position) {
    const auto voiceIt = this->voices_.find(voice);
    if (voiceIt == this->voices_.end()) return;
    voiceIt->second->setSpatializationEnabled(true);
    voiceIt->second->setPosition({position.x, position.y, position.z});
}

void SfmlAudioEngine::setListenerPosition(const core::math::Vec3f& position) {
    sf::Listener::setPosition({position.x, position.y, position.z});
}

MusicId SfmlAudioEngine::openMusic(const std::filesystem::path& path) {
    auto music = std::make_unique<sf::Music>();
    if (!music->openFromFile(path)) return 0;

    const MusicId musicId = this->nextMusicId_++;
    this->music_.emplace(musicId, std::move(music));
    return musicId;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void SfmlAudioEngine::playMusic(MusicId music, float volume, bool loop) {
    const auto musicIt = this->music_.find(music);
    if (musicIt == this->music_.end()) return;
    musicIt->second->setVolume(volume);
    musicIt->second->setLooping(loop);
    musicIt->second->play();
}

void SfmlAudioEngine::pauseMusic(MusicId music) {
    const auto musicIt = this->music_.find(music);
    if (musicIt != this->music_.end()) musicIt->second->pause();
}

void SfmlAudioEngine::stopMusic(MusicId music) {
    const auto musicIt = this->music_.find(music);
    if (musicIt != this->music_.end()) musicIt->second->stop();
}

bool SfmlAudioEngine::isMusicPlaying(MusicId music) const {
    const auto musicIt = this->music_.find(music);
    return musicIt != this->music_.end() &&
           musicIt->second->getStatus() == sf::Music::Status::Playing;
}

void SfmlAudioEngine::pruneFinishedVoices() {
    for (auto voiceIt = this->voices_.begin();
         voiceIt != this->voices_.end();) {
        if (voiceIt->second->getStatus() == sf::Sound::Status::Stopped) {
            voiceIt = this->voices_.erase(voiceIt);
        } else {
            ++voiceIt;
        }
    }
}

}  // namespace ampersand::audio
