add_library(parser)
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(parser PRIVATE
    src/generator.cpp
    src/grammar.cpp
    src/lr1_parser.cpp
)

target_link_libraries(parser PRIVATE common)

target_compile_features(parser PRIVATE cxx_std_23)