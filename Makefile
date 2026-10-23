.PHONY: all clean

all:
	cmake -G "Ninja" -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S . -B build && cd build && ninja

clean:
	rm -rf build joosc
