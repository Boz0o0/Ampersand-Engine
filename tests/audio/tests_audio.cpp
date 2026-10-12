#include <gtest/gtest.h>

#include <ampersand/audio/AudioListener.hpp>
#include <ampersand/audio/AudioSource.hpp>
#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <type_traits>

TEST(AudioModule, MissingAssetsReturnZeroIds) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_EQ(engine.loadSound("/definitely/not/a/real/sound.wav"), 0);
    EXPECT_EQ(engine.openMusic("/definitely/not/a/real/music.ogg"), 0);
}

TEST(AudioModule, PlayingAnUnknownSoundReturnsZero) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_EQ(engine.play(1, 100.0F, 1.0F, false), 0);
}

TEST(AudioModule, UnknownVoicesAreNoOpAndNeverPlaying) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_FALSE(engine.isPlaying(1));
    EXPECT_NO_THROW(engine.stop(1));
    EXPECT_NO_THROW(engine.pause(1));
    EXPECT_NO_THROW(engine.resume(1));
    EXPECT_NO_THROW(engine.setVolume(1, 50.0F));
    EXPECT_NO_THROW(engine.setPitch(1, 2.0F));
    EXPECT_NO_THROW(engine.setPosition(1, {1.0F, 2.0F, 3.0F}));
}

TEST(AudioModule, UnknownMusicIsNoOpAndNeverPlaying) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_FALSE(engine.isMusicPlaying(1));
    EXPECT_NO_THROW(engine.playMusic(1, 100.0F, true));
    EXPECT_NO_THROW(engine.pauseMusic(1));
    EXPECT_NO_THROW(engine.stopMusic(1));
}

TEST(AudioModule, ListenerPositionCanBeSetWithoutAnyVoice) {
    ampersand::audio::SfmlAudioEngine engine;

    EXPECT_NO_THROW(engine.setListenerPosition({0.0F, 0.0F, 0.0F}));
}

TEST(AudioSourceComponent, DefaultsToAudibleNonLoopingSilence) {
    const ampersand::audio::AudioSource source;

    EXPECT_EQ(source.sound, 0U);
    EXPECT_FLOAT_EQ(source.volume, 100.0F);
    EXPECT_FLOAT_EQ(source.pitch, 1.0F);
    EXPECT_FALSE(source.loop);
    EXPECT_FALSE(source.spatial);
    EXPECT_EQ(source.voice, 0U);
}

TEST(AudioListenerComponent, IsAnEmptyTag) {
    const ampersand::audio::AudioListener listener;
    (void)listener;
    EXPECT_TRUE(std::is_empty_v<ampersand::audio::AudioListener>);
}
