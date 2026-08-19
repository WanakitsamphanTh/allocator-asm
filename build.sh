for af in src/*.asm; do
    [ -e "$af" ] || continue
    name=$(basename "$af" .asm)
    nasm -f elf64 -d DEBUG "$af" -o "build/$name.o"
done

obj_files=(build/*.o)
gcc "${obj_files[@]}" -o run/main -lc