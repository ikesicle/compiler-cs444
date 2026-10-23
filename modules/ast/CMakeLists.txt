add_library(ast)
target_include_directories(ast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(ast PRIVATE
    src/ast_printer.cpp
    src/nodes.cpp
    src/java_type.cpp
)

target_link_libraries(ast PRIVATE common)

target_compile_features(ast PRIVATE cxx_std_23)