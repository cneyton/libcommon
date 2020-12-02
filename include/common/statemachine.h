#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <limits>
#include <map>

namespace common {

enum class transition_status {
    stay_curr_state,
    goto_next_state
};

template<typename T>
class Statemachine
{
public:
    struct error: std::runtime_error
    {
        error(const std::string& what_arg): std::runtime_error(what_arg) {}
    };

    struct Transition
    {
        using Handler = std::function<transition_status()>;
        T       next_state_id;
        Handler handler;
    };

    struct State
    {
        std::string              name;
        T                        id;
        std::vector<Transition>  transitions;
    };

    using TransitionHandler = std::function<void(const State*, const State*)>;
    using StateList         = std::vector<State>;

    Statemachine(std::string name, const StateList& states, T initial_state_id):
        name_(name)
    {
        for (auto const& st: states) {
            map_[st.id] = st;
        }

        const auto search = map_.find(initial_state_id);
        if (search == map_.end())
            throw error("initial state no found in states");

        initial_state_ = &(search->second);
        curr_state_    = initial_state_;
        prev_state_    = initial_state_;
    }

    void reinit()
    {
        std::unique_lock<std::mutex> lk(mutex_, std::defer_lock);
        if (!lk.try_lock()) {
            reinit_requested_  = true;
            return;
        }
        reinit_requested_ = false;
        prev_state_ = curr_state_;
        curr_state_ = initial_state_;
        nb_loop_in_current_state_ = 0;
        if (transition_handler_) {
            try {
                transition_handler_(prev_state_, curr_state_);
            } catch (...) {
                error("error during transition callback");
            }
        }
    }

    void enable()        { enabled_ = true; }
    void disable()       { enabled_ = false; }
    T curr_state() const { return curr_state_->id; }
    T prev_state() const { return prev_state_->id; }
    uint64_t nb_loop_in_current_state() { return nb_loop_in_current_state_; }
    void set_transition_handler(TransitionHandler&& h) { transition_handler_ = h; }

    std::cv_status wait_for(T st, const std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        return  cv_.wait_for(lk, timeout, [&] {return curr_state() == st;});
    }

    void wait(T st)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [&] {return curr_state() == st;});
    }

    void wakeup()
    {
        if (!enabled_)
            return;

        nb_loop_in_current_state_++;
        /* hack to avoid overload on long states */
        if (nb_loop_in_current_state_ == std::numeric_limits<uint64_t>::max())
            nb_loop_in_current_state_ = 100;

        {
            std::unique_lock<std::mutex> lk(mutex_);
            // execute each transition handler to check is a state change is required
            for (auto const& t: curr_state_->transitions) {
                if (t.handler() == transition_status::goto_next_state &&
                    t.next_state_id != curr_state_->id) {
                    nb_loop_in_current_state_ = 0;
                    auto search = map_.find(t.next_state_id);
                    if (search == map_.end())
                        throw error("next state not found");
                    prev_state_ = curr_state_;
                    curr_state_ = &(search->second);
                    if (transition_handler_) {
                        try {
                            transition_handler_(prev_state_, curr_state_);
                        } catch (...) {
                            error("error during transition callback");
                        }
                    }
                    break;
                }
            }
        }
        cv_.notify_all();

        if (reinit_requested_)
            reinit();
    }

private:
    std::string        name_;
    std::map<T, State> map_;
    const State      * initial_state_;
    const State      * curr_state_;
    const State      * prev_state_;

    TransitionHandler  transition_handler_;

    uint64_t    nb_loop_in_current_state_ = 0;
    bool        reinit_requested_ = false;
    bool        enabled_          = true;

    std::mutex              mutex_;
    std::condition_variable cv_;
};

} /* namespace common */
