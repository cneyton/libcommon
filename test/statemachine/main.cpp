#include <thread>
#include <chrono>
#include <iostream>
#include "common/statemachine.h"
#include "common/log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace common;
auto console = spdlog::stdout_color_mt("console");
bool exit_test = false;

class Test
{
private:
    enum class states {
        disconnected,
        connecting,
        connected
    };

    transition_status handler_state_disconnected_()
    {
        log_info(console, "disconnected");
        disconnected_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        return transition_status::stay_curr_state;
    }

    transition_status handler_state_connecting_()
    {
        log_info(console, "connecting");
        connection_opened_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        return transition_status::stay_curr_state;
    }

    transition_status handler_state_connected_()
    {
        log_info(console, "connected");
        connection_established_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        return transition_status::stay_curr_state;
    }

    transition_status check_connection_opened_()
    {
        if (connection_opened_) {
            log_info(console, "check ok");
            return transition_status::goto_next_state;
        }
        return transition_status::stay_curr_state;
    }

    transition_status check_connected_()
    {
        if (connection_established_)
            return transition_status::goto_next_state;
        return transition_status::stay_curr_state;
    }

    transition_status check_disconnected_()
    {
        if (disconnected_)
            return transition_status::goto_next_state;
        return transition_status::stay_curr_state;
    }

    const common::Statemachine<states>::StateList states_ {
        {"disconnected", states::disconnected,
            {{states::disconnected, std::bind(&Test::handler_state_disconnected_, this)},
             {states::connecting,   std::bind(&Test::check_connection_opened_, this)}}
        },
        {"connecting", states::connecting,
            {{states::connecting,   std::bind(&Test::handler_state_connecting_, this)},
             {states::disconnected, std::bind(&Test::check_disconnected_, this)},
             {states::connected,    std::bind(&Test::check_connected_, this)}}
        },
        {"connected", states::connected,
            {{states::connected,    std::bind(&Test::handler_state_connected_, this)},
             {states::disconnected, std::bind(&Test::check_disconnected_, this)}}
        }
    };

    common::Statemachine<states> statemachine_;

    bool connection_opened_      = false;
    bool connection_established_ = false;
    bool disconnected_           = false;

    common::Logger logger_;

public:
    Test(common::Logger logger) : statemachine_("sm_test", states_, states::disconnected)
    {
        statemachine_.set_transition_handler(
            [logger] (const Statemachine<states>::State * p, const Statemachine<states>::State * c)
            {
                log_info(logger, "statemachine : {} -> {}", p->name, c->name);
            });
    }

    void run()
    {
        while (!exit_test) {
            statemachine_.wakeup();
        }
    }

    int open_connection()
    {
        connection_opened_ = true;
        return 0;
    }

    int disconnect()
    {
        disconnected_ = true;
        return 0;
    }

    int establish_connection()
    {
        connection_established_ = true;
        return 0;
    }

    int reinit_statemachine()
    {
        statemachine_.reinit();
        return 0;
    }
};

int host_cli(Test& test)
{
    int  choice = 254;

    std::string usr_in;

    do {
        std::cout << "Select a command:\n"
            "  0) Open connection\n"
            "  1) Connect\n"
            "  2) Disconnect\n"
            "255) EXIT\n"
            ">> ";
        /* get user input */
        std::cin >> usr_in;
        try {
            choice = std::stoi(usr_in);
        } catch (...) {
            log_error(console, "error : {}", usr_in);
            choice = 254;
        }

        switch (choice) {
            case 0:
                test.open_connection();
                choice = 254;
                break;

            case 1:
                test.establish_connection();
                choice = 254;
                break;

            case 2:
                test.disconnect();
                choice = 254;
                break;

            case 3:
                test.reinit_statemachine();
                break;

            case 255:
                exit_test = true;
                break;

            case 254:
            default:
                break;
        }
    } while (!exit_test);

    return 0;
}

int main()
{
    console->set_level(spdlog::level::debug);
    console->set_pattern("[%T:%e][%^%l%$] %s:%#:%! | %v");

    Test test(console);

    std::thread sm_thread(&Test::run, &test);

    int ret;
    ret = host_cli(test);
    common_die_zero(console, ret, -1, "cli error");

    sm_thread.join();

    return 0;
}

