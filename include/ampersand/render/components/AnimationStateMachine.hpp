#pragma once

#include <ampersand/render/components/Animation.hpp>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ampersand::render {

/**
 * @brief Named values written by the gameplay and read by the conditions of
 * the AnimationTransitions (speed, grounded...).
 */
struct AnimationParams {
    std::unordered_map<std::string, float> floats;
    std::unordered_map<std::string, bool> bools;
};

/**
 * @brief Switches the base state of an AnimationStateMachine from `from` to
 * `to` when `condition` returns true.
 */
struct AnimationTransition {
    /** @brief Base state the transition leaves, or "*" for any state. */
    std::string from;
    /** @brief Base state the transition enters. */
    std::string to;
    /** @brief Fires the transition when it returns true (never if empty). */
    std::function<bool(const AnimationParams&)> condition;
};

/**
 * @brief Picks which clip the entity's Animation plays, driven by the
 * AnimationSystem.
 *
 * The base state (current) follows the transitions. An action started with
 * playOnce overrides it, plays once, then gives the base state back. While an
 * action plays, the transitions are paused. When the playing clip changes,
 * update copies it into the entity's Animation (added if missing
 * by the AnimationSystem) and restarts it.
 */
class AnimationStateMachine {
public:
    /**
     * @param states Clips by state name, used both for base states and
     * actions.
     * @param initial Base state to start in.
     * @throws std::invalid_argument if `states` has no state named
     * `initial`.
     */
    AnimationStateMachine(std::unordered_map<std::string, Animation> states,
                          std::string initial);

    /** @brief Base state, played when no action plays. */
    [[nodiscard]] const std::string& current() const;

    /**
     * @brief Moves to another base state, without waiting for a transition
     *
     * @param state Name of a state of the machine.
     * @throws std::invalid_argument if there is no state named `state`.
     */
    void setCurrent(std::string state);

    /**
     * @brief Plays a state once on top of the base state, then goes back to
     * it
     *
     * Replaces the action already playing, if any. Playing the same action
     * again restarts it.
     *
     * @param action Name of a state of the machine, played without looping.
     * @throws std::invalid_argument if there is no state named `action`.
     */
    void playOnce(std::string action);

    /**
     * @brief Checks if an action started with playOnce is still playing
     *
     * @return true until the action's last frame was shown for its whole
     * duration.
     */
    [[nodiscard]] bool isPlayingAction() const;

    /** @brief Makes the next update load the playing clip again from
     * `states`, from its first frame. */
    void reload();

    /**
     * @brief Ends the finished action, applies the first matching transition
     * if no action plays, then loads the clip to play into `animation` if it
     * changed
     *
     * Called by the AnimationSystem once per frame.
     *
     * @param animation Animation of the same entity, overwritten (and
     * restarted) when the playing clip changes.
     * @throws std::invalid_argument if the clip to play is not in `states`.
     */
    void update(Animation& animation);

    /** @brief Clips by state name. Call reload after editing the playing
     * one. */
    std::unordered_map<std::string, Animation> states;
    /** @brief Checked in order, the first match wins (one per update), even
     * if it leads to the current state. */
    std::vector<AnimationTransition> transitions;
    AnimationParams params;

private:
    void requireState(const std::string& state) const;

    std::string _current;
    std::optional<std::string> _action;
    /** @brief Name of the clip loaded in the Animation, empty to reload. */
    std::string _playing;
};

}  // namespace ampersand::render
