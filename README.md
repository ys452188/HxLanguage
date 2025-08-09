# HxLanguage
## By 硫酸铜非常好吃
- git : `https://github.com/ys452188/HxLanguage.git`
- [GitHub](https://github.com/ys452188/HxLanguage)
# 语法 
关键字：
```text
var,con,fun,for,if,else,while,for,foreach,break,continue,wchar,char,float,double,switch,case,class,using,false,true,定义变量,定义常量,定义函数,如果,否则,整型,浮点型,字符型,宽字符型,精确浮点型
```
---
### &ensp;定义变量
#### &ensp;&ensp;巴克斯范式表示：
```BNF
定义变量 ::= <<"var">|<"定义变量"> <":">|<"："> <标识符> <","> <"type">|<"它的类型是"> <":">|<"："> <关键字>|<标识符>
```
---
### &ensp;定义常量
#### &ensp;&ensp;巴克斯范式表示：
```BNF
定义常量 ::= <"con">|<"定义常量"> <":">|<"："> <标识符> <","> <"type">|<"它的类型是"> <":">|<"："> <关键字>|<标识符>
```
---
### &ensp;定义函数
#### &ensp;&ensp;巴克斯范式表示：
```BNF
定义函数 ::= <"fun">|<"定义函数"> <标识符> <"("> <标识符> <":"> <关键字>|<标识符> <")"> <":">|<"："> <标识符>|<关键字> <"{"> <代码块> <"}">
```
---
# 目标文件
* 魔数：0x48584F424A
* 后缀：.hxe