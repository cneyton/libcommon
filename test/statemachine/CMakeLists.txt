add_executable(common_test_statemachine main.cpp)
target_link_libraries(common_test_statemachine PUBLIC common)
target_compile_options(common_test_statemachine PRIVATE -Werror -Wall -Wextra)
