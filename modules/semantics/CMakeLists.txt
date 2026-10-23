add_library(semantics)
target_sources(semantics PRIVATE
    src/ast_parser.cpp
    src/disambiguation.cpp
    src/type_linker.cpp
    src/type_checker.cpp
    src/definite_assignment.cpp
)

target_link_libraries(semantics PUBLIC common ast)

target_include_directories(semantics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(semantics PRIVATE cxx_std_23)