# compiler-cs444

Repository for the Joosc compiler, made for CS444 in Winter 2026, targeting x86 Linux.

To build, run
```
make all
```
in the repository directory. This will create a file called "joosc" in the repository directory.
Unlike an actual Java compiler, Joosc requires all files to be compiled to be passed in as command-line arguments.
When executed, it will create a new directory called `output` in the execution location with `.s` assembly files, that then have to be manually compiled together and linked using `nasm` and `ld`. A script which does this is located at `tools/fullcompile.sh`.

An example assembly, alongside all of the supporting assembly files can be viewed at `tests/output/JAVA_J1_1_Instanceof_InLazyExp.s`.
