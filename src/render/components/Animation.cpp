#include <ampersand/render/components/Animation.hpp>
#include <cstddef>

namespace ampersand::render {

bool isFinished(const Animation& animation) {
    return !animation.loop &&
           animation.elapsed >= animation.frameDuration *
                                    static_cast<float>(animation.frames.size());
}

std::vector<core::math::Rect<std::int32_t>> framesFromGrid(
    const FrameGrid& grid) {
    std::vector<core::math::Rect<std::int32_t>> frames;
    if (grid.columns <= 0 || grid.count <= 0) {
        return frames;
    }
    frames.reserve(static_cast<std::size_t>(grid.count));
    for (std::int32_t i = 0; i < grid.count; ++i) {
        const std::int32_t column = i % grid.columns;
        const std::int32_t row = i / grid.columns;
        frames.emplace_back(
            grid.offset.x + (column * (grid.frameSize.x + grid.gap.x)),
            grid.offset.y + (row * (grid.frameSize.y + grid.gap.y)),
            grid.frameSize.x, grid.frameSize.y);
    }
    return frames;
}

}  // namespace ampersand::render
