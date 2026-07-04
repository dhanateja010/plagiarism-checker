cmake_minimum_required(VERSION 3.16)
project(PlagiarismChecker VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")

add_executable(plagiarism_checker
    main.cpp
    plagiarism_checker.cpp
    tokenizer.cpp
)

target_compile_features(plagiarism_checker PRIVATE cxx_std_17)

target_link_libraries(plagiarism_checker PRIVATE pthread)

install(TARGETS plagiarism_checker DESTINATION bin)