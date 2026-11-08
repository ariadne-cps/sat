# Benchmarking registers this directory only for standalone builds.
# Production SAT does not link against the Benchmarking framework.
add_executable(benchmark_sat_pigeonhole benchmark_sat_pigeonhole.cpp)
target_link_libraries(benchmark_sat_pigeonhole PRIVATE
    ariadne-sat sat ariadne-benchmarking)
add_dependencies(benchmarks benchmark_sat_pigeonhole)

add_executable(benchmark_sat_nqueens benchmark_sat_nqueens.cpp)
target_link_libraries(benchmark_sat_nqueens PRIVATE
    ariadne-sat sat ariadne-benchmarking)
add_dependencies(benchmarks benchmark_sat_nqueens)
