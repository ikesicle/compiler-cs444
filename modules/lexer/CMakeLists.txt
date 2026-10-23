add_library(lexer)
target_include_directories(lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(lexer PRIVATE
    src/checkpoint_stream.cpp
)

target_link_libraries(lexer PUBLIC common)

target_compile_features(lexer PRIVATE cxx_std_23)

add_executable(lex_file src/lex_file.cpp)
target_link_libraries(lex_file PRIVATE lexer)
target_compile_features(lex_file PRIVATE cxx_std_23)

add_test(NAME lexer_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_lexer.sh $<TARGET_FILE:lex_file>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

