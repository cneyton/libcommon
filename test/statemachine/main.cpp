#include <thread>
#include <chrono>
#include <iostream>
#include "statemachine.h"
#include "log.h"
#include "spdlog/sinks/stdout_color_sinks.h"


class Test
{
private:
    enum class states {
        disconnected,
        connecting,
        connected
    };

    int handler_state_disconnected_()
    {
        log_info(spdlog::get("console"), "disconnected");
        connection_opened_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return common::statemachine::goto_next_state;
    }

    int handler_state_connecting_()
    {
        log_info(spdlog::get("console"), "connecting");
        disconnected_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return 0;
    }

    int handler_state_connected_()
    {
        log_info(spdlog::get("console"), "connected");
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return 0;
    }

    int check_connection_opened_()
    {
        if (connection_opened_)
            return 1;
        return 0;
    }

    int check_connected_()
    {
        if (connection_established_)
            return 1;
        return 0;
    }

    int check_disconnected_()
    {
        if (disconnected_)
            return 1;
        return 0;
    }

    const common::StatesList<states> states_ {
        {"disconnected", states::disconnected,
            {{std::bind(&Test::handler_state_disconnected_, this), states::disconnected},
             {std::bind(&Test::check_connection_opened_, this),    states::connecting}}
        },
        {"connecting", states::connecting,
            {{std::bind(&Test::handler_state_connecting_, this),   states::connecting,},
             {std::bind(&Test::check_disconnected_, this),         states::disconnected},
             {std::bind(&Test::check_connected_, this),            states::connected,}}
        },
        {"connected", states::connected,
            {{std::bind(&Test::handler_state_connected_, this),    states::connected,},
             {std::bind(&Test::check_disconnected_, this),         states::disconnected,}}
        }
    };

    common::Statemachine<states> statemachine_;

    bool connection_opened_      = false;
    bool connection_established_ = false;
    bool disconnected_           = true;

    common::Logger logger_;

public:
    Test(common::Logger logger) : statemachine_("sm_test", states_, states::disconnected, logger)
    {
        statemachine_.display_trace();
    }
    void run()
    {
        int ret;
        while (1) {
            ret = statemachine_.wakeup();
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
    bool exit = false;
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
            log_error(spdlog::get("console"), "error : {}", usr_in);
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

            case 255:
                exit = true;
                break;

            case 254:
            default:
                break;
        }
    } while (!exit);

    return 0;
}

int main()
{
    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);
    console->set_pattern("[%T:%e][%^%l%$] %s:%#:%! | %v");

    Test test(console);

    std::thread sm_thread(&Test::run, &test);

    int ret = host_cli(test);

    sm_thread.join();

    return 0;
}

