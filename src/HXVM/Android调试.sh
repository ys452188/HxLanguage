# Google风格格式化
clang-format -style=Google -i Main.c VM.h
clang -g -fno-omit-frame-pointer -fsanitize=address Main.c
cp -r a.out ~/
rm a.out
cd ~/
chmod +x a.out
./a.out