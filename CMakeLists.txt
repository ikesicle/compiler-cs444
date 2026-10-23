cmake_minimum_required(VERSION 3.28)
project(cs444compiler LANGUAGES CXX)

add_executable(joosc-a1 src/main-a1.cpp)
target_compile_features(joosc-a1 PRIVATE cxx_std_23)

add_executable(joosc-a2 src/main-a2.cpp)
target_compile_features(joosc-a2 PRIVATE cxx_std_23)

add_executable(joosc-a3 src/main-a3.cpp)
target_compile_features(joosc-a3 PRIVATE cxx_std_23)

add_executable(joosc-a4 src/main-a4.cpp)
target_compile_features(joosc-a4 PRIVATE cxx_std_23)

add_executable(joosc-a5 src/main-a5.cpp)
target_compile_features(joosc-a5 PRIVATE cxx_std_23)

enable_testing()

add_subdirectory(modules/common)
add_subdirectory(modules/ast)
add_subdirectory(modules/lexer)
add_subdirectory(modules/parser)
add_subdirectory(modules/weeder)
add_subdirectory(modules/semantics)
add_subdirectory(modules/reachability)
add_subdirectory(modules/generation)

target_link_libraries(joosc-a1 PRIVATE lexer parser weeder common)
target_link_libraries(joosc-a2 PRIVATE lexer parser weeder common semantics)
target_link_libraries(joosc-a3 PRIVATE lexer parser weeder common semantics)
target_link_libraries(joosc-a4 PRIVATE lexer parser weeder common semantics reachability)
target_link_libraries(joosc-a5 PRIVATE lexer parser weeder common semantics reachability generation)

add_custom_command(
    TARGET joosc-a4
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/grammar.cfg"
            "$<TARGET_FILE_DIR:joosc-a5>/tools/grammar.cfg"
    COMMENT "Copying grammar.cfg to executable directory"
)

add_custom_command(
    TARGET joosc-a4
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/grammar.tbl"
            "$<TARGET_FILE_DIR:joosc-a5>/tools/grammar.tbl"
    COMMENT "Copying grammar.tbl to executable directory"
)

add_custom_command(
    TARGET joosc-a5
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:joosc-a5>"
            "${CMAKE_CURRENT_SOURCE_DIR}/joosc"
    COMMENT "Copying executable to build directory"
)

if (WIN32)
    set(TEST_EXEC test_joosc.bat)
    set(MARM_TEST_EXEC marmoset_sim_tests.bat)
else()
    set(TEST_EXEC test_joosc.sh)
    set(MARM_TEST_EXEC marmoset_sim_tests.sh)
endif()

add_test(NAME joosc-a1_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/a1/${TEST_EXEC} $<TARGET_FILE:joosc-a1>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/a1
)

add_test(NAME joosc-a2_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/a2/${TEST_EXEC} $<TARGET_FILE:joosc-a2>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/a2
)

add_test(NAME joosc-a3_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a3> a3
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

add_test(NAME joosc-a4_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a4> a4
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

add_test(NAME joosc-a2_test_marmoset
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a2> a2_marmoset
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

add_test(NAME joosc-a3_test_marmoset
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a3> a3_marmoset
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

add_test(NAME joosc-a4_test_marmoset
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a4> a4_marmoset
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

add_test(NAME joosc-a5
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a5> a5 compile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

add_test(NAME joosc-a5_test_marmoset
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a5> a5_marmoset compile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

add_test(NAME joosc-a6_test_marmoset
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/${MARM_TEST_EXEC} $<TARGET_FILE:joosc-a5> a6_marmoset compile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
)

