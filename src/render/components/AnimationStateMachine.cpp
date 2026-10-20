#include <ampersand/render/components/AnimationStateMachine.hpp>
#include <stdexcept>
#include <utility>

namespace ampersand::render {

AnimationStateMachine::AnimationStateMachine(
    std::unordered_map<std::string, Animation> states, std::string initial)
    : states(std::move(states)), _current(std::move(initial)) {
    requireState(_current);
}

const std::string& AnimationStateMachine::current() const { return _current; }

void AnimationStateMachine::setCurrent(std::string state) {
    requireState(state);
    _current = std::move(state);
}

void AnimationStateMachine::playOnce(std::string action) {
    requireState(action);
    _action = std::move(action);
    _playing.clear();
}

bool AnimationStateMachine::isPlayingAction() const {
    return _action.has_value();
}

void AnimationStateMachine::reload() { _playing.clear(); }

void AnimationStateMachine::requireState(const std::string& state) const {
    if (!states.contains(state)) {
        throw std::invalid_argument("AnimationStateMachine has no state '" +
                                    state + "'");
    }
}

}  // namespace ampersand::render
