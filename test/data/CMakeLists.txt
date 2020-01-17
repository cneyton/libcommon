add_executable(common_test_data main.cpp)
target_link_libraries(common_test_data PUBLIC common)
target_compile_options(common_test_data PRIVATE -Werror -Wall -Wextra)
