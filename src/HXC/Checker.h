#ifndef HXLANG_SRC_HXC_CHECKER_H
#define HXLANG_SRC_HXC_CHECKER_H
/**
* 在分析到新函数时检查函数是否重复定义，包含对错误的处理
* @return 错误为-1，0为未声明，否则为声明在表中的索引
*/
int isFunctionRepeatDefine(IR_Function* fun, IR_Function** table, int table_size) {

}
#endif