clang -g -fno-omit-frame-pointer -fsanitize=address Main.c
cp -r a.out ~/
cd ~/
chmod +x a.out
./a.out