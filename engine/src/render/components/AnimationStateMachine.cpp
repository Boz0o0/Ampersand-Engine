#include <algorithm>
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

void AnimationStateMachine::update(Animation& animation) {
    if (_action && _playing == *_action && isFinished(animation)) {
        _action.reset();
        _playing.clear();
    }
    if (!_action) {
        const auto transition = std::ranges::find_if(
            transitions, [this](const AnimationTransition& candidate) {
                return (candidate.from == _current || candidate.from == "*") &&
                       candidate.condition && candidate.condition(params);
            });
        if (transition != transitions.end()) {
            setCurrent(transition->to);
        }
    }
    const std::string& wanted = _action.value_or(_current);
    if (wanted == _playing) {
        return;
    }
    const auto clip = states.find(wanted);
    if (clip == states.end()) {
        throw std::invalid_argument("AnimationStateMachine has no state '" +
                                    wanted + "'");
    }
    animation = clip->second;
    animation.elapsed = 0.0F;
    animation.loop = animation.loop && !_action;
    _playing = wanted;
}

void AnimationStateMachine::requireState(const std::string& state) const {
    if (!states.contains(state)) {
        throw std::invalid_argument("AnimationStateMachine has no state '" +
                                    state + "'");
    }
}

}  // namespace ampersand::render
