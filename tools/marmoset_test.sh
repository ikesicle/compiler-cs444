#!/bin/bash

tools/build.sh

cd tests

if [ -d "$1" ]; then
    echo "Testing directory: $1"
else
    if [[ "$1" == *_marmoset ]]; then
        echo "Skipping marmoset test: $1"
        exit 0
    else
        echo "Error: $1 is not a directory or a .java file, and is also not a marmoset test that can be skipped"
        exit 1
    fi
    exit 1
fi

cd "$1"

shift


ln -s ../java .
ln -s ../java_all.txt .

java_all_files=()
while IFS= read -r line; do
    java_all_files+=("$line")
done < java_all.txt
for item in $@; do
    # Skip "java" and "java_all.txt"
    [[ "$item" == "java" || "$item" == "java_all.txt" ]] && continue
    
    files=()
    if [[ -d "$item" ]]; then
        echo "Is a directory: $item"
        # It's a directory, find all .java files
        while IFS= read -r file; do
            files+=("$file")
        done < <(find "$item" -name "*.java")
    elif [[ "$item" == *.java ]]; then
        # It's a .java file
        files=("$item")
    else
        # Skip other files
        continue
    fi
    echo "Files to test: ${files[*]}"
    echo "Testing $item"
    "../../joosc" "${files[@]}" "${java_all_files[@]}" 2>&1
    exit_code=$?
    # echo "Exit code: $exit_code"
    if (( $exit_code == 0 )) && [[ $item == Je* ]]; then
        echo "Error: Test $item expected to fail but passed (exit code 0)"
        echo "Command: ../../joosc ${files[@]} ${java_all_files[@]}"
        exit=1
    fi
    if (( $exit_code == 42 )) && [[ $item != Je* ]]; then
        echo "Error: Test $item expected to pass but failed (exit code $exit_code)"
        echo "Command: ../../joosc ${files[@]} ${java_all_files[@]}"
        exit=1
    fi
    if (( $exit_code != 0 )) && (( $exit_code != 42 )); then
        echo "Test $item failed due to an internal error, with exit code $exit_code)"
        echo "Command: ../../joosc ${files[@]} ${java_all_files[@]}"
        exit=1
    fi
    echo "Exit code: $exit_code"
done

echo "All tests completed successfully!"
exit $exit
