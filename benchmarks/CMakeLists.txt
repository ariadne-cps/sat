add_executable(benchmark_sat_pigeonhole benchmark_sat_pigeonhole.cpp)
target_link_libraries(benchmark_sat_pigeonhole PRIVATE
    ariadne-sat sat logging LOGGING_SRC
)
add_dependencies(benchmarks benchmark_sat_pigeonhole)
