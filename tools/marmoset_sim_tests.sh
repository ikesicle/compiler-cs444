#!/bin/bash

echo "Executable: $1"
exit=0;

if [ -d "$2" ]; then
    echo "Testing directory: $2"
else
    if [[ "$2" == *_marmoset ]]; then
        echo "Skipping marmoset test: $2"
        exit 0
    else
        echo "Error: $2 is not a directory or a .java file, and is also not a marmoset test that can be skipped"
        exit 1
    fi
    exit 1
fi

java_all_files=()
while IFS= read -r line; do
    java_all_files+=("$line")
done < java_all.txt

for item in $2/*; do
    # Skip "java" and "java_all.txt" any .expectout files
    [[ "$item" == "java" || "$item" == "java_all.txt" || "$item" == *.expectout ]] && continue
    
    files=()
    
    test_name=$(basename "$item")
    base_name=""
    if [[ -d "$item" ]]; then
        # It's a directory, find all .java files
        if [[ -f "$item/Main.java" ]]; then
            files+=("$item/Main.java")
        fi
        while IFS= read -r file; do
            files+=("$file")
        done < <(find "$item" -name "*.java" -not -name "Main.java")
        base_name=$item
    elif [[ "$item" == *.java ]]; then
        # It's a .java file
        files=("$item")
        base_name="${item%.java}"
    else
        # Skip other files
        continue
    fi
    
    # echo "Testing $item"
    rm output/JAVA_*.s 2> /dev/null
    $1 "${files[@]}" "${java_all_files[@]}"
    exit_code=$?
    # echo "Exit code: $exit_code"
    if (( $exit_code == 0 )) && [[ $test_name == Je* ]]; then
        echo "Error: Test $item expected to fail but passed (exit code 0)"
        echo "Command: $1 ${files[@]} ${java_all_files[@]}"
        exit=1
        exit 1
    fi
    if (( $exit_code == 42 )) && [[ $test_name != Je* ]]; then
        echo "Error: Test $item expected to pass but failed (exit code $exit_code)"
        echo "Command: $1 ${files[@]} ${java_all_files[@]}"
        exit=1
        exit 1
    fi
    if (( $exit_code != 0 )) && (( $exit_code != 42 )); then
        echo "Test $item failed due to an internal error, with exit code $exit_code)"
        echo "Command: $1 ${files[@]} ${java_all_files[@]}"
        exit=1
        exit 1
    fi
    
    if [[ $3 == "compile" ]]; then # Compile and test
        rm output/*.o 2> /dev/null
        for f in output/*.s; do
            echo "Compiling $f"
            nasm -O1 -f elf -g -F dwarf "$f"; exit_code=$?
            if (( $exit_code != 0 )); then
                echo "Error: Failed to compile $f"
                exit=1
                exit 1
            fi
        done
        ld -melf_i386 -o output/main output/*.o; exit_code=$?
        if (( $exit_code != 0 )); then
            echo "Error: Failed to link main"
            exit=1
            exit 1
        fi
        tout=$(mktemp)
        output/main > "$tout"
        exit_code=$?
        if (( $exit_code != 123 )) && [[ $test_name != J?e* ]]; then
            echo "Error: Executable for passing test $test_name expected to return 123 but returned $exit_code"
            exit=1
            echo "Command: tools/fullcompile.sh ${files[@]}"
            exit 1
        fi
        if (( $exit_code != 13 )) && [[ $test_name == J?e* ]]; then
            echo "Error: Executable for failing test $test_name expected to return 13 but returned $exit_code"
            exit=1
            echo "Command: tools/fullcompile.sh ${files[@]}"
            exit 1
        fi
        if [[ -s "$base_name.expectout" ]]; then
            diff -b "$base_name.expectout" "$tout" > /dev/null; exit_code=$?
            if (( $exit_code != 0 )); then
                echo "Error: Output for test $test_name does not match expected output"
                echo "Command: tools/fullcompile.sh ${files[@]}"
                exit=1
                exit 1
            fi
        fi
    fi
done
exit $exit
