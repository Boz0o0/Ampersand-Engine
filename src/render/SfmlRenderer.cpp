#include <algorithm>
#include <ampersand/render/SfmlConversions.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace ampersand::render {

bool SfmlRenderer::open(const WindowConfig& config) {
    this->_window.create(sf::VideoMode({config.size.x, config.size.y}),
                         config.title);
    this->_window.setVerticalSyncEnabled(config.vsync);
    this->_customView = false;
    this->updateDefaultView();
    this->_window.setView(this->_defaultView);
    return this->_window.isOpen();
}

void SfmlRenderer::close() {
    this->_drawCommands.clear();
    this->_window.close();
}

core::math::Vec2u SfmlRenderer::size() const {
    const auto windowSize = this->_window.getSize();
    return {windowSize.x, windowSize.y};
}

std::optional<Event> SfmlRenderer::pollEvent() {
    while (const auto event = this->_window.pollEvent()) {
        if (event->is<sf::Event::Resized>()) {
            this->updateDefaultView();
            if (!this->_customView) this->_window.setView(this->_defaultView);
        }
        const auto converted = event->visit(
            [](const auto& sfmlEvent) { return toEvent(sfmlEvent); });
        if (converted) {
            return converted;
        }
    }
    return std::nullopt;
}

void SfmlRenderer::setView(const Camera& camera) {
    if (!std::isfinite(camera.center.x) || !std::isfinite(camera.center.y) ||
        !std::isfinite(camera.size.x) || !std::isfinite(camera.size.y) ||
        camera.size.x <= 0.0F || camera.size.y <= 0.0F) {
        throw std::invalid_argument(
            "Camera center and size must be finite, and "
            "camera size must be positive");
    }
    this->_window.setView(sf::View({camera.center.x, camera.center.y},
                                   {camera.size.x, camera.size.y}));
    this->_customView = true;
}

void SfmlRenderer::resetView() {
    this->_customView = false;
    this->updateDefaultView();
    this->_window.setView(this->_defaultView);
}

TextureId SfmlRenderer::loadTexture(const std::filesystem::path& path) {
    sf::Texture texture;
    if (!texture.loadFromFile(path)) return 0;

    const TextureId textureId = this->_nextId++;
    this->_textures.emplace(textureId, std::move(texture));
    return textureId;
}

FontId SfmlRenderer::loadFont(const std::filesystem::path& path) {
    sf::Font font;
    if (!font.openFromFile(path)) return 0;

    const FontId fontId = this->_nextFontId++;
    this->_fonts.emplace(fontId, std::move(font));
    return fontId;
}

void SfmlRenderer::beginFrame(Color clear) {
    this->_drawCommands.clear();
    this->_window.clear(toSfColor(clear));
}

void SfmlRenderer::draw(const Sprite& sprite,
                        const core::Transform& transform) {
    this->_drawCommands.push_back({
        .layer = sprite.layer,
        .draw =
            [this, sprite, transform] {
                const auto texture = this->_textures.find(sprite.texture);
                if (texture == this->_textures.end()) return;

                sf::Sprite renderedSprite(texture->second);
                if (sprite.source.width > 0 && sprite.source.height > 0) {
                    renderedSprite.setTextureRect(sf::IntRect(
                        {sprite.source.x, sprite.source.y},
                        {sprite.source.width, sprite.source.height}));
                }
                renderedSprite.setColor(toSfColor(sprite.color));
                applyTransform(renderedSprite, transform);
                this->batchSprite(renderedSprite, sprite.texture);
            },
    });
}

void SfmlRenderer::draw(const Rectangle& rectangle,
                        const core::Transform& transform) {
    this->_drawCommands.push_back({
        .layer = rectangle.layer,
        .draw =
            [this, rectangle, transform] {
                this->flush();
                sf::RectangleShape shape({rectangle.size.x, rectangle.size.y});
                shape.setFillColor(toSfColor(rectangle.fill));
                shape.setOutlineColor(toSfColor(rectangle.outline));
                shape.setOutlineThickness(rectangle.outlineThickness);
                applyTransform(shape, transform);
                this->_window.draw(shape);
            },
    });
}

void SfmlRenderer::draw(const Circle& circle,
                        const core::Transform& transform) {
    this->_drawCommands.push_back({
        .layer = circle.layer,
        .draw =
            [this, circle, transform] {
                this->flush();
                sf::CircleShape shape(circle.radius);
                shape.setFillColor(toSfColor(circle.fill));
                shape.setOutlineColor(toSfColor(circle.outline));
                shape.setOutlineThickness(circle.outlineThickness);
                applyTransform(shape, transform);
                this->_window.draw(shape);
            },
    });
}

void SfmlRenderer::draw(const Line& line, const core::Transform& transform) {
    this->_drawCommands.push_back({
        .layer = line.layer,
        .draw =
            [this, line, transform] {
                this->flush();
                sf::Transformable transformable;
                applyTransform(transformable, transform);
                const sf::Color color = toSfColor(line.color);
                const std::array<sf::Vertex, 2> vertices{
                    {
                        {
                            .position = {line.from.x, line.from.y},
                            .color = color,
                        },
                        {
                            .position = {line.to.x, line.to.y},
                            .color = color,
                        },
                    },
                };
                this->_window.draw(
                    vertices.data(), vertices.size(), sf::PrimitiveType::Lines,
                    sf::RenderStates(transformable.getTransform()));
            },
    });
}

void SfmlRenderer::draw(const Text& text, const core::Transform& transform) {
    const auto ownedText = std::make_shared<const Text>(text);
    this->_drawCommands.push_back({
        .layer = text.layer,
        .draw =
            [this, ownedText, transform] {
                const auto font = this->_fonts.find(ownedText->font);
                if (font == this->_fonts.end()) return;

                sf::Text drawable(font->second, ownedText->value,
                                  ownedText->size);
                drawable.setFillColor(toSfColor(ownedText->color));
                applyTransform(drawable, transform);
                this->flush();
                this->_window.draw(drawable);
            },
    });
}

void SfmlRenderer::endFrame() {
    std::ranges::stable_sort(this->_drawCommands, {}, &DrawCommand::layer);
    for (const DrawCommand& command : this->_drawCommands) command.draw();
    this->flush();
    this->_drawCommands.clear();
    this->_window.display();
}

void SfmlRenderer::applyTransform(sf::Transformable& drawable,
                                  const core::Transform& transform) {
    drawable.setPosition({transform.position.x, transform.position.y});
    drawable.setScale({transform.scale.x, transform.scale.y});
    drawable.setRotation(sf::degrees(transform.rotationDegrees.z));
    drawable.setOrigin({transform.origin.x, transform.origin.y});
}

void SfmlRenderer::batchSprite(const sf::Sprite& sprite, TextureId texture) {
    static constexpr std::array<std::size_t, 6> kQuad{0, 1, 2, 2, 1, 3};

    if (texture != this->_batchTexture) {
        this->flush();
        this->_batchTexture = texture;
    }
    const sf::FloatRect source(sprite.getTextureRect());
    const std::array corners{
        sf::Vector2f(0.0F, 0.0F),
        sf::Vector2f(source.size.x, 0.0F),
        sf::Vector2f(0.0F, source.size.y),
        source.size,
    };
    const sf::Transform& transform = sprite.getTransform();
    for (const std::size_t corner : kQuad) {
        this->_batch.push_back({
            .position = transform.transformPoint(corners[corner]),
            .color = sprite.getColor(),
            .texCoords = source.position + corners[corner],
        });
    }
}

void SfmlRenderer::flush() {
    if (this->_batch.empty()) return;

    this->_window.draw(
        this->_batch.data(), this->_batch.size(), sf::PrimitiveType::Triangles,
        sf::RenderStates(&this->_textures.at(this->_batchTexture)));
    this->_batch.clear();
}

void SfmlRenderer::updateDefaultView() {
    const auto windowSize = this->_window.getSize();
    if (windowSize.x == 0 || windowSize.y == 0) return;
    this->_defaultView = sf::View(
        {static_cast<float>(windowSize.x) / 2.0F,
         static_cast<float>(windowSize.y) / 2.0F},
        {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)});
}

}  // namespace ampersand::render
