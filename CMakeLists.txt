cmake_minimum_required(VERSION 3.10)
project(PlagiarismChecker VERSION 2.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")

# Find Clang/LLVM
find_package(Clang REQUIRED)

# Add executable
add_executable(plagiarism_checker
    main.cpp
    plagiarism_checker.cpp
    tokenizer.cpp
)

# Include directories
target_include_directories(plagiarism_checker PRIVATE
    ${CLANG_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(plagiarism_checker PRIVATE
    ${CLANG_LIBRARIES}
    pthread
)

# Installation
install(TARGETS plagiarism_checker DESTINATION bin)

# Print configuration
message(STATUS "===========================================")
message(STATUS "Plagiarism Checker - Phase 2 Configuration")
message(STATUS "===========================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Clang Include: ${CLANG_INCLUDE_DIRS}")
message(STATUS "Clang Libraries: ${CLANG_LIBRARIES}")
message(STATUS "===========================================")