# Set output dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/benchmark)

# Add executables
add_executable(benchmark_clustertool2 ./benchmark_clustertool2.cpp)
add_executable(benchmark_clustertool3 ./benchmark_clustertool3.cpp)
add_executable(benchmark_clustertool4 ./benchmark_clustertool4.cpp)
add_executable(benchmark_clustertool5 ./benchmark_clustertool5.cpp)
add_executable(benchmark_clustertool6 ./benchmark_clustertool6.cpp)
add_executable(benchmark_clustertool7 ./benchmark_clustertool7.cpp)
add_executable(benchmark_fsm ./benchmark_fsm.cpp)
# add_executable(benchmark_lazyfsm ./benchmark_lazyfsm.cpp)
# add_executable(benchmark_lazyclustertool6 ./benchmark_lazyclustertool6.cpp)
# add_executable(benchmark_lazyclustertool7 ./benchmark_lazyclustertool7.cpp)
add_executable(benchmark_FULLLAZYclustertool5 ./benchmark_FULLLAZYclustertool5.cpp)
add_executable(benchmark_FULLLAZYclustertool6 ./benchmark_FULLLAZYclustertool6.cpp)
add_executable(benchmark_FULLLAZYclustertool7 ./benchmark_FULLLAZYclustertool7.cpp)
add_executable(benchmark_FULLLAZYfsm ./benchmark_FULLLAZYfsm.cpp)

# Link libraries
if (CLDES_OPENMP_ENABLED)
    target_link_libraries(benchmark_clustertool2 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_clustertool3 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_clustertool4 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_clustertool5 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_clustertool6 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_clustertool7 OpenMP::OpenMP_CXX)
    # target_link_libraries(benchmark_lazyfsm OpenMP::OpenMP_CXX)
    # target_link_libraries(benchmark_lazyclustertool6 OpenMP::OpenMP_CXX)
    # target_link_libraries(benchmark_lazyclustertool7 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_FULLLAZYclustertool5 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_FULLLAZYclustertool6 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_FULLLAZYclustertool7 OpenMP::OpenMP_CXX)
    target_link_libraries(benchmark_FULLLAZYfsm OpenMP::OpenMP_CXX)
endif()

if (CLDES_OPENCL_ENABLED)
    if (CLDES_OPENMP_ENABLED)
        # target_link_libraries(benchmark_clustertool6_gpu OpenMP::OpenMP_CXX OpenCL::OpenCL)
        # target_link_libraries(benchmark_clustertool7_gpu OpenMP::OpenMP_CXX OpenCL::OpenCL)
        # target_link_libraries(benchmark_fsm_gpu OpenMP::OpenMP_CXX OpenCL::OpenCL)
    else()
        # target_link_libraries(benchmark_clustertool6_gpu OpenCL::OpenCL)
        # target_link_libraries(benchmark_clustertool7_gpu OpenCL::OpenCL)
        # target_link_libraries(benchmark_fsm_gpu OpenCL::OpenCL)
    endif()
endif()
