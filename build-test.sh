name=""
if [[ "$1" == *.asm ]]; then
    name=$(basename "$1" .asm)
    nasm -f elf64 -d DEBUG "test/$1" -o "build/$name.o"
elif [[ "$1" == *.c ]]; then
    name=$(basename "$1" .c)
    gcc -c test/$1 -o "build/$name.o"
fi


obj_files=(build/$name.o lib/*.o)
gcc "${obj_files[@]}" -o run/$name -lc -no-pie