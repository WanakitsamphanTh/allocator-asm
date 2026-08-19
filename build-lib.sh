for af in src/*.asm; do
    [ -e "$af" ] || continue
    name=$(basename "$af" .asm)
    nasm -f elf64 -d DEBUG "$af" -o "lib/$name.o"
    #nasm -f elf64 "$af" -o "lib/$name.o"
done