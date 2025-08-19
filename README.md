# fieldnotes

## IPC 进程间通信

### 1 无名管道 pipe

单向通信，只能在有亲缘关系的**进程之间**使用，例如：父子、兄弟进程

系统调用pipe在内核中创建一块缓冲区（环形队列），**同时**返回两个文件描述符读端：fd[0]和写端：fd[1]

对于这块缓冲区的内容，只能通过fd[1]写入，通过fd[0]读出，例如，子进程通过fd[1]写入内容，父进程通过fd[0]读出内容

半双工通信，意味着不能通过pipe同时双向读写其中的内容，具体的表现就是：
- 缓冲区满，阻塞写入
- 缓冲区空，阻塞读出

因此对于可以使用pipe通信的两个进程，同时拥有fd[0]和fd[1]：
- 如果读进程不关闭自己的fd[1]，当写进程写完内容关闭自己的fd[1]，内核会检测到仍存在fd[1]（读进程自己的fd[1]），不会给读进程返回EOF，阻塞读进程
- 如果写进程不关闭自己的fd[0]，当读进程读完内容关闭自己的fd[0]，内核会检测到仍存在fd[0]（写进程自己的fd[0]），不会给写进程返回SIGPIPE，写进程会持续写入，直到缓冲区满，卡死写进程

因此使用pipe实现进程间通信一定要关闭不需要的读端或者写端，如果想实现全双工的效果需要两个管道

## inline

**特性：**
inline有类型检查，相较于宏更加安全
编译器不会内联具备循环、递归和switch等复杂语句的函数
内联由编译器决定，inline标识不一定实际内联
cpp中，成员函数在类中定义，除了虚函数以外都被隐式内联

**优点：**
省略函数调用参数压栈，提升性能

**缺点：**
代码膨胀
inline函数改变时需要重新编译

不生效多态特性时，虚函数可以内联；即内联发生在编译期，运行期才能确定的动态类型对象优先生效多态特性，虚机制优先级高于内联机制

## this

```cpp
/* this 指向对象 */
class TempClass {
private:
    T a;
public:
    T getValue();
};

class *const this; // 隐式声明
```

## static

```c
/* static修饰变量 */
static T a; // 存储在BSS段，但是会被系统默认值初始化
static T a = 0; // 存储在数据段

// .c/.cpp
static void function(); // 仅在定义该函数的文件内才可以使用

// .h
static inline void function(); // 在每个包含.h的.c/.cpp中都会生成一个副本并展开
                               // 如果.h中声明的static函数没有inline：
                               // 1. 每个源文件一份副本，增加可执行文件体积
                               // 2. 修改静态函数，只编译部分源文件，函数行为不一致
                               // 3. 静态函数内部状态不共享
                               // 4. 难以维护和扩展
```

```cpp
class TempClass {
private:
    static T a;
    static T functionA();
protected:
    static T b;
    static T functionB();
public:
    static T c;
    static inline T d = 0; // C++17，必须在类内初始化
    static T functionC();
};

/* static成员变量必须类外初始化 */
T TempClass::a = 0;
T TempClass::b = 0;
T TempClass::c = 0;

/* static成员函数类外定义不要加static */
T TempClass::functionA() {return 0;}
T TempClass::functionB() {return 0;}
T TempClass::functionC() {return 0;}

cout<<TempClass::a<<endl; // 错误，static变量为private
TempClass::functionA(); // 错误，同上

class ChildTempClass: public TempClass {};
ChildTempClass *obj = new ChildTempClass;
cout<<obj->b<<endl; // 正确，基类的static变量可以被子类对象调用
obj->functionB(); // 正确，同上

cout<<TempClass::c<<endl; // 正确，static变量为public
TempClass::functionC(); // 正确，同上
                        // static成员函数不能访问非静态成员变量和成员函数
                        // 不能是纯虚函数或者虚函数
```

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

memset有可能被编译器优化，当某块内存被memset后不会被使用，编译器会认为进行memset的操作无意义，因此会优化掉改该行代码，这对于敏感数据非常危险

#### 1.2 memset_s

```c
/*
 * C11
 * @dest: 待填充的内存地址
 * @destsz: 缓冲区大小 
 * @ch: 待写入的字节值
 * @count: 预期写入的字节数
 * @returnValue: 错误码，0: 正常值，EINVAL: dest==NULL or count>destsz，ERANGE: destsz>ESIZE_MAX
 */
errno_t memset_s(void *dest, rsize_t destsz, int ch, rsize_t count);
```

核心特性：
1. 参数合法性校验
2. 返回错误码
3. 即使发生错误也尝试清理内存，保障敏感数据不保留在内存
4. 禁止编译器优化
5. C11标准支持，windows MSVC天然支持，linux gilbc不支持，使用如下替代

```c
void explicit_bzero(void *s, size_t n);
```

### 2 memcpy和memcpy_s

#### 2.1 memcpy

```c
#include <string.h>
void *memcpy(void *dest, const void *src, size_t count); // 将src开始的count个字节复制到dest中，两块内存不可重叠（未定义）
                                                         // dest缓冲区过小会造成缓冲区溢出
                                                         // 无返回值错误码
```

#### 2.2 memcpy_s

```c
/*
 * @returnValue: 0      成功
 *               EINVAL 参数非法NULL或越界
 *               ERANGE count>destsz
 * 失败时，缓冲区被清零，防止敏感数据泄露
 */
errno_t memcpy_s(void *dest, rsize_t destsz, const void *src, rsize_t count); // destsz指定缓冲区大小，最好使用sizeof(dest类型)，尤其当dest为结构体或数组
```

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