cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S . -B build && \
cd build && \
ninja && ctest --output-on-failure
