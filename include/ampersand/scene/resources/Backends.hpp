#pragma once

#include <map>
#include <memory>
#include <string>

namespace ampersand::scene {

/**
 * @brief Owns the backends created from the configuration files (an
 * SfmlRenderer for "backend": "sfml", for example).
 *
 * Added to the World by the SceneManager the first time a resource needs a
 * backend, so the backends live as long as the World.
 */
struct Backends {
    /**
     * @brief Backends by the name of the resource using them ("Renderer"),
     * type-erased but deleted correctly.
     */
    std::map<std::string, std::shared_ptr<void>> owned;
};

}  // namespace ampersand::scene
