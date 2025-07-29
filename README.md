# fieldnotes

## 安全函数

### 1 memset和memset_s

#### 1.1 memset

```c
/*
 * @s: 待填充内存地址
 * @c: 预期设置值，会被强转为unsigned char类型，因此只能取低8bit，int形同虚设
 * @n: 待填充字节数
 * @returnValue: s
 */
void *memset(void *s, int c, size_t n);

char *str = NULL;
memset(str, 'A', 0); // 正确，n==0
memset(str, 'A', 10); // 错误，段错误

char str[10];
memset(str, 'A', 9); // 正确
memset(str, 'A', 10); // 正确
memset(str, 'A', 11); // 错误，buffer overflow

int arr[10];
memset(arr, 0, sizeof(arr)); // 正确
memset(arr, -1, sizeof(arr)); // 正确
memset(arr, 0xFF, sizeof(arr)); // 正确，结果同上
memset(arr, 1, sizeof(arr)); // 错误，禁止设置非0和非-1的值，memset按照字节写入，实际arr中的元素是0x01010101

TempStru obj;
memset(&obj, 0, sizeof(obj)); // 正确，但是存在特殊情况
typedef struct {
    int n;
    char *str;
}TempStru;
TempStru obj = {0};
obj.str = (char*)malloc(sizeof(char)*20);
memset(&obj, 0, sizeof(obj)); // 错误，str会被置为NULL，无法访问堆上内存，造成内存泄露
if (obj.str!=NULL) {
    free(obj.str);
    obj.str = NULL;
}
memset(&obj, 0, sizeof(obj)); // 正确
```

```cpp
TempStru obj{}; // C++初始化方法

TempClass obj;
memset(&obj, 0, sizeof(obj)); // 错误，不能用于C++类，使用构造/析构函数

int *arr = new int[20];
memset(arr, 0, 20*sizeof(int)); // 正确

TempClass *obj = new TempClass;
memset(obj, 0, sizeof(TempClass)); // 错误
                                   // new包含堆上内存分配和调用构造函数初始化对象
                                   // memset会破坏对象中的复杂结构：
                                   // 1、虚函数指针
                                   // 2、初始化逻辑
                                   // 3、内存管理和资源句柄等
                                   // 4、存在STL
```

使用**AddressSanitizer/Valgrind**在调试期间发现内存问题
memset的第三个入参最好使用**sizeof(obj或者*ptr)**

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

```c
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

```cpp
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
```