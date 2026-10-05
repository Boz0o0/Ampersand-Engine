#include <ampersand/render/SfmlKey.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <utility>

namespace ampersand::render {

bool SfmlRenderer::open(const WindowConfig& config) {
  this->window_.create(sf::VideoMode({config.size.x, config.size.y}),
                       config.title);
  this->window_.setVerticalSyncEnabled(config.vsync);
  return this->window_.isOpen();
}

void SfmlRenderer::close() { this->window_.close(); }

core::math::Vec2u SfmlRenderer::size() const {
  const auto size = this->window_.getSize();
  return {size.x, size.y};
}

std::optional<Event> SfmlRenderer::pollEvent() {
  while (const auto event = this->window_.pollEvent()) {
    if (event->is<sf::Event::Closed>()) return Closed{};
    if (const auto* const resized = event->getIf<sf::Event::Resized>()) {
      return Resized{{resized->size.x, resized->size.y}};
    }
    if (const auto* const keyPressed = event->getIf<sf::Event::KeyPressed>()) {
      return ampersand::render::KeyPressed{toKey(keyPressed->code)};
    }
    if (const auto* const keyReleased =
            event->getIf<sf::Event::KeyReleased>()) {
      return ampersand::render::KeyReleased{toKey(keyReleased->code)};
    }
  }
  return std::nullopt;
}

TextureId SfmlRenderer::loadTexture(const std::filesystem::path& path) {
  sf::Texture texture;
  if (!texture.loadFromFile(path)) return 0;

  const TextureId textureId = this->nextId_++;
  this->textures_.emplace(textureId, std::move(texture));
  return textureId;
}

void SfmlRenderer::beginFrame(Color clear) {
  this->window_.clear(sf::Color(clear.r, clear.g, clear.b, clear.a));
}

void SfmlRenderer::draw(const Sprite& sprite, core::math::Vec2f position) {
  const auto texture = this->textures_.find(sprite.texture);
  if (texture == this->textures_.end()) return;

  sf::Sprite renderedSprite(texture->second);
  if (sprite.source.width > 0 && sprite.source.height > 0) {
    renderedSprite.setTextureRect(
        sf::IntRect({sprite.source.x, sprite.source.y},
                    {sprite.source.width, sprite.source.height}));
  }
  renderedSprite.setPosition({position.x, position.y});
  this->window_.draw(renderedSprite);
}

void SfmlRenderer::endFrame() { this->window_.display(); }

};  // namespace ampersand::render
