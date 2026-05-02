set -e
#Google风格格式化
find . \( -name "*.h" -o -name "*.cpp" \) -exec clang-format --style=Google -i {} +
gcc Main.c -o hxc -g
cp - r hxc ~/ 
cp - r test.hxl ~/ 
rm hxc 
cd ~/ 
chmod +x hxc
./hxc test.hxl 
rm hxc 
rm test.hxl