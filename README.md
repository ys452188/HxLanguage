# ⚠️ 警告：这是一个连开发者都不敢保证明天能跑起来的玩具
# HxLanguage
这是一个由 C++ 编写(早期用C,由于我发玩C++的vector、string、thread、new、折构比C更好玩，所以改用C++，但仍有C风格)的虚拟机及编译器 HXC。
---

## 🚀 核心组件

* **HXC (Compiler)**：负责把你的源代码变成 `.hxo` 二进制文件。
* **HXVM (Interpreter)**：一个简单的基于栈的解释器。

---

## 构建

如果你真的勇士，想在你的机器上编译它：
```bash
mkdir -p build
cd build
cmake ..
make -j2
# 如果你想看它疯狂输出调试信息
./hxvm
```
如果你打算把它用到生产环境，那你是个人物