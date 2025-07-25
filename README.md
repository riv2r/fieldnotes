# fieldnotes

## .c/.cpp内存模型

```
        ↑ 高地址
+---------------------------+
|       栈区 (Stack)        |← 函数局部变量、参数
+---------------------------+
|       空间 (保留)         |← 一般未使用
+---------------------------+
|       堆区 (Heap)         |← malloc/new 动态分配
+---------------------------+
|       BSS段 (未初始化)    |← 全局/静态未初始化变量
+---------------------------+
|       数据段 (已初始化)   |← 全局/静态已初始化变量
+---------------------------+
|       代码段 (Text)       |← 函数体、指令、常量
+---------------------------+
        ↓ 低地址
```

## const

常量（逻辑层面）

```C
/* const and pointer */
const T *ptr; // 常量指针，指向的内容是常量，无法改变指向的内容，但是可以改变自身
T const *ptr;
/* e.g. */
T a = 0;
T b = 1;
const T *ptr = &a;
*ptr = b; // 错误

T *const ptr; // 指针常量，指针本身是常量，指针指向的内容可以改变，但是自身不能改变
/* e.g. */
T a = 0;
T b = 1;
T *const ptr = &a;
*ptr = b;
ptr = &b; // 错误

const T *const ptr; // 指向内容和自身都不能改变的指针

/* const and reference */
const T &ref; // 多用于形参，避免函数对参数的修改，避免了拷贝的开销

/* const member function */
class A{
private:
    T value;
    mutable T m;
public:
    T changeValue(T v);
    T getValue() const; // 常成员函数，不可改变成员变量，除非成员变量被mutable修饰，例如m
                        // mutable应用场景：
                        // 1. 缓存：缓存情况进行记录，优化性能
                        // 2. 日志counter：const保障函数不改变日志本身内容，mutable counter记录日志记录次数
                        // 3. 访问次数
                        // 4. 延迟初始化
};

A a;
T v = 1;
T oldValue = a.getValue(); // 普通成员可以调用常成员函数
T newValue = a.changeValue(v); // 普通成员可以调用普通成员函数
const A b;
oldValue = b.getValue();
newValue = b.changeValue(v); // 错误，常对象只能调用常成员函数

/* 声明和定义 */
// 1. 仅在当前函数/文件使用->定义在.c/.cpp
#pragma once // 保证.h不被重复包含
const T a = 0; // const默认具有内部链接，相当于加了static
// 2. 跨文件共享
// .h声明
extern const T a;
// .c/.cpp定义
const T a = 0; // 避免多个定义违反ODR

// 不推荐将const定义在.h中，这样在多个include该头文件的.c/.cpp都会产生一个副本，链接时可能出错
// .h中的const定义可以使用constexpr，隐式inline，可以在副本中定义多次，但只链接一次
constexpr T a = 0;
```