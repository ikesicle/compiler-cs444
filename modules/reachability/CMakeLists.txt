add_library(reachability)
target_include_directories(reachability PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(reachability PRIVATE
    src/reachability.cpp
)

target_link_libraries(reachability PRIVATE common ast)

target_compile_features(reachability PRIVATE cxx_std_23)