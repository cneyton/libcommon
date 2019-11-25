#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <mutex>
#include <cstdint>
#include <functional>
#include <vector>
#include <chrono>

#include "log.h"

namespace common
{

namespace statemachine
{

constexpr int goto_next_state = 1;
constexpr int stay_curr_state = 0;

using TransitionHandler = std::function<int()>;

template<typename T>
class Transition
{
public:
    Transition(std::function<int()> handler, T next_state_id)
        : next_state_id_(next_state_id), handler_(handler) {}

    T                    get_next_state() const {return next_state_id_;}
    TransitionHandler    get_handler()    const {return handler_;}

private:
    T                    next_state_id_;
    TransitionHandler    handler_;
};

template<typename T>
class State
{
public:
    State(std::string name, T id, std::vector<Transition<T>> transitions)
        : name_(name), id_(id), transitions_(transitions) {}

    std::string                 get_name()        const {return name_;}
    T                           get_id()          const {return id_;}
    std::vector<Transition<T>>  get_transitions() const {return transitions_;}

private:
    std::string                 name_;
    T                           id_;
    std::vector<Transition<T>>  transitions_;
};

} /* namespace statemachine */


template<typename T>
using StatesList = std::vector<statemachine::State<T>>;

template<typename T>
class Statemachine
{
public:
    Statemachine(const std::string name, const StatesList<T>& states,
                 const T initial_state_id, Logger logger)
        : name_(name), states_(states), nb_loop_in_current_state_(0), logger_(logger)
    {
        for (auto const& st: states_) {
            if (st.get_id() == initial_state_id) {
                current_state_ = &st;
                initial_state_ = &st;
            }
        }
    }

    int reinit()
    {
        std::unique_lock<std::timed_mutex> lk(mutex_, std::defer_lock);
        if (!lk.try_lock()) {
            reinit_requested_  = true;
            log_info(logger_, "unable to reinit, try again later");
            return 0;
        }
        current_state_ = initial_state_;
        nb_loop_in_current_state_ = 0;
        return 0;
    }

    int enable()
    {
        enabled_ = true;
        return 0;
    }

    int disable()
    {
        enabled_ = false;
        return 0;
    }

    int display_trace()
    {
        verbose_ = true;
        return 0;
    }

    uint32_t get_nb_loop_in_current_state()
    {
        return nb_loop_in_current_state_;
    }


    int wakeup()
    {
        if (!enabled_)
            return 0;

        nb_loop_in_current_state_++;
        /* hack to avoid overload on long states */
        if (nb_loop_in_current_state_ == 0xFFFFFFFF)
            nb_loop_in_current_state_ = 100;

        int ret;
        {
            std::unique_lock<std::timed_mutex> lk(mutex_, std::defer_lock);

            if (!lk.try_lock_for(std::chrono::milliseconds(1000)))
                common_die(logger_, -1, "fail to lock statemachine {}", name_);

            T next_state_id = current_state_->get_id();

            for (auto const& t: current_state_->get_transitions()) {
                ret = t.get_handler()();
                common_die_zero(logger_, ret, -1, "transition handler error");
                if (ret == statemachine::goto_next_state) {
                    if (t.get_next_state() != next_state_id)
                        nb_loop_in_current_state_ = 0;
                    next_state_id = t.get_next_state();
                    break;
                }
            }

            if (next_state_id != current_state_->get_id() && !reinit_requested_) {
                for (auto const& st: states_) {
                    if (st.get_id() == next_state_id) {
                        current_state_ = &st;
                        break;
                    }
                }
                if (verbose_)
                    log_info(logger_, "statemachine {} -> {}", name_, current_state_->get_name());
            }
        }

        if (reinit_requested_) {
            log_info(logger_, "reinit the statemachine {} now", name_);
            ret = reinit();
            common_die_zero(logger_, ret, -1, "statemachine {} reinit error", name_);
            reinit_requested_ = false;
        }



        return 0;
    }

private:
    Logger      logger_;
    std::timed_mutex  mutex_;
    uint32_t    nb_loop_in_current_state_ = 0;
    bool        verbose_          = false;
    bool        reinit_requested_ = false;
    bool        enabled_          = true;

    std::string            const   name_;
    StatesList<T>          const   states_;
    statemachine::State<T> const * initial_state_;
    statemachine::State<T> const * current_state_;
};

} /* namespace common */

#endif /* STATEMACHINE_H */
