cd tests
java_all_files=()
while IFS= read -r line; do
    java_all_files+=("$line")
done < java_all.txt

../joosc "$@" "${java_all_files[@]}"
if [[ $? -ne 0 ]]; then
    echo "Error: joosc failed to compile."
    echo "Command: ../joosc $* ${java_all_files[*]}"
    exit 1
fi