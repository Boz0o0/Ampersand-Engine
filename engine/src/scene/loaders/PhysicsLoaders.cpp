#ifdef AMPERSAND_MODULE_PHYSICS

#include <ampersand/scene/resources/SceneManager.hpp>

namespace ampersand::scene {

void SceneManager::registerPhysics() {
    registerResource<physics::GravityForce>("GravityForce");
    registerComponent<physics::Velocity>("Velocity");
    registerComponent<physics::Gravity>("Gravity");
    registerComponent<physics::BoxCollider>("BoxCollider");
}

}  // namespace ampersand::scene

#endif
