add_library(weeder)

target_sources(weeder PRIVATE
    src/weeder.cpp
)

target_include_directories(weeder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(weeder PUBLIC common)

target_compile_features(weeder PUBLIC cxx_std_17)
