#include <gtest/gtest.h>

#include <ampersand/render/SfmlRenderer.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <filesystem>
#include <string>

TEST(RenderModule, MissingAssetsReturnZeroIds) {
    ampersand::render::SfmlRenderer renderer;

    EXPECT_EQ(renderer.loadTexture("/definitely/not/a/real/texture.png"), 0);
    EXPECT_EQ(renderer.loadFont("/definitely/not/a/real/font.ttf"), 0);
}

TEST(RenderComponents, SpriteAndTextDefaultToWhite) {
    const ampersand::render::Sprite sprite;
    const ampersand::render::Text text;

    EXPECT_EQ(sprite.color.r, 255);
    EXPECT_EQ(sprite.color.g, 255);
    EXPECT_EQ(sprite.color.b, 255);
    EXPECT_EQ(sprite.color.a, 255);
    EXPECT_EQ(text.color.r, 255);
    EXPECT_EQ(text.color.a, 255);
    EXPECT_EQ(text.size, 24U);
    EXPECT_EQ(sprite.layer, 0);
    EXPECT_EQ(text.layer, 0);
}

TEST(RenderResources, ClearColorDefaultsToOpaqueBlack) {
    const ampersand::render::ClearColor clear;

    EXPECT_EQ(clear.value.r, 0);
    EXPECT_EQ(clear.value.g, 0);
    EXPECT_EQ(clear.value.b, 0);
    EXPECT_EQ(clear.value.a, 255);
}

namespace {

using ampersand::core::Transform;
using ampersand::render::Circle;
using ampersand::render::Line;
using ampersand::render::Rectangle;
using ampersand::render::SfmlRenderer;
using ampersand::render::Sprite;
using ampersand::render::Text;
using ampersand::render::TextureId;

/**
 * @brief Opens a small SfmlRenderer window with two loaded textures. The
 * renderer cannot read its pixels back, so the tests check that every batching
 * path runs, not what is drawn. Skipped where no window can be opened.
 */
class SfmlRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        _root = std::filesystem::temp_directory_path() /
                ("ampersand_render_" +
                 std::string(::testing::UnitTest::GetInstance()
                                 ->current_test_info()
                                 ->name()));
        std::filesystem::remove_all(_root);
        std::filesystem::create_directories(_root);
        if (!_renderer.open(
                {.size = {64, 64}, .title = "test", .vsync = false})) {
            GTEST_SKIP() << "No display to open a window";
        }
        _red = texture("red.png", sf::Color::Red);
        _blue = texture("blue.png", sf::Color::Blue);
    }

    void TearDown() override {
        _renderer.close();
        std::filesystem::remove_all(_root);
    }

    /** @brief Saves a 4x4 image of one color and loads it as a texture. */
    TextureId texture(const std::string& name, sf::Color color) {
        const std::filesystem::path path = _root / name;
        EXPECT_TRUE(sf::Image({4, 4}, color).saveToFile(path));
        return _renderer.loadTexture(path);
    }

    std::filesystem::path _root;
    SfmlRenderer _renderer;
    TextureId _red{};
    TextureId _blue{};
};

}  // namespace

TEST_F(SfmlRendererTest, LoadsTexturesUnderDistinctIds) {
    EXPECT_NE(_red, 0U);
    EXPECT_NE(_blue, 0U);
    EXPECT_NE(_red, _blue);
}

TEST_F(SfmlRendererTest, DrawsAnEmptyFrame) {
    _renderer.beginFrame({});
    EXPECT_NO_THROW(_renderer.endFrame());
    EXPECT_TRUE(_renderer.isOpen());
}

TEST_F(SfmlRendererTest, DrawsManySpritesOfOneTexture) {
    for (int frame = 0; frame < 3; ++frame) {
        _renderer.beginFrame({});
        for (int i = 0; i < 100; ++i) {
            _renderer.draw(Sprite{.texture = _red},
                           Transform{
                               .position =
                                   {
                                       static_cast<float>(i % 64),
                                       0.0F,
                                       0.0F,
                                   },
                           });
        }
        EXPECT_NO_THROW(_renderer.endFrame());
    }
}

TEST_F(SfmlRendererTest, DrawsAlternatingTexturesWithShapesBetween) {
    _renderer.beginFrame({});
    _renderer.draw(Sprite{.texture = _red, .layer = 1}, Transform{});
    _renderer.draw(Rectangle{.size = {8.0F, 8.0F}, .outlineThickness = 1.0F},
                   Transform{});
    _renderer.draw(Sprite{.texture = _blue, .source = {0, 0, 2, 2}},
                   Transform{});
    _renderer.draw(Sprite{.texture = _red}, Transform{});
    _renderer.draw(Circle{.radius = 4.0F, .layer = 1}, Transform{});
    _renderer.draw(Line{.to = {10.0F, 10.0F}}, Transform{});
    _renderer.draw(Sprite{.texture = _blue, .layer = -1}, Transform{});
    _renderer.draw(Sprite{.texture = _blue, .layer = 2}, Transform{});
    EXPECT_NO_THROW(_renderer.endFrame());
}

TEST_F(SfmlRendererTest, SkipsUnknownTextureAndFontInsideABatch) {
    _renderer.beginFrame({});
    _renderer.draw(Sprite{.texture = _red}, Transform{});
    _renderer.draw(Sprite{.texture = 999}, Transform{});
    _renderer.draw(Text{.font = 999, .value = "x"}, Transform{});
    _renderer.draw(Sprite{.texture = _red}, Transform{});
    EXPECT_NO_THROW(_renderer.endFrame());
}

TEST_F(SfmlRendererTest, DrawsAgainAfterCloseInTheMiddleOfAFrame) {
    _renderer.beginFrame({});
    _renderer.draw(Sprite{.texture = _red}, Transform{});
    _renderer.close();
    ASSERT_TRUE(
        _renderer.open({.size = {64, 64}, .title = "test", .vsync = false}));

    _renderer.beginFrame({});
    _renderer.draw(Sprite{.texture = _blue}, Transform{});
    EXPECT_NO_THROW(_renderer.endFrame());
}
