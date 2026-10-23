add_library(generation)
target_include_directories(generation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(generation PUBLIC
    src/gen_visitor.cpp
)

target_link_libraries(generation PUBLIC common ast)

target_compile_features(generation PUBLIC cxx_std_23)
