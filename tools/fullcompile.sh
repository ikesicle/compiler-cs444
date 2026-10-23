tools/build.sh || exit 1
rm tests/output/JAVA_*.s
tools/run.sh $@ || exit 1
cd tests

rm output/*.o
for f in output/*.s; do
    echo "Compiling $f"
    /usr/bin/nasm -O1 -f elf -g -F dwarf "$f" || exit 1
done
echo "Linking..."
ld -melf_i386 -o main output/*.o || exit 1
rm output/*.o
./main
