cmake_minimum_required(VERSION 3.5)

project(poisson_disk_sampler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BENCHMARK_ENABLE_TESTING FALSE)
include(FetchContent)
FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark
)
FetchContent_MakeAvailable(googlebenchmark)

include(FetchContent)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
)
FetchContent_MakeAvailable(glm)

add_executable(poisson_disk_sampler_benchmark main.cpp poisson.cpp)
target_link_libraries(poisson_disk_sampler_benchmark benchmark::benchmark glm::glm)
