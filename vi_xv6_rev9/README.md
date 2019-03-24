# xv6_with_vi
基于xv6_rev9版本，添加关于控制台（console）的系统调用，实现简单的vi 文本编辑器。


# vi的设计思路
1. 增加对屏幕的系统调用，这样才能操作屏幕。（已完成）
1. 接收字符输入，但是不显示在屏幕上。OS自己实现时会将键入字符自动显示在屏幕上。（已完成）
1. 实现文本浏览器，即可以打开一个文件，浏览里面的内容，可以任意移动光标，翻页，换行等。（出错中）
1. 在上一版的基础上添加编辑功能，主要是在光标处插入字符。


## 阶段1
* 增加系统调用，使得能操作屏幕。

## 阶段2
* 修改console.c文件中的函数consoleintr()，并添加相应的系统调用，使得目标实现。

## 阶段3
* 难点：dot与cursor的同步问题。核心函数：int countCursorPosFromScreenBegin2Dot(); 根据指针screenbegin和dot来确定光标的位置。

## 阶段4



# 增加的系统调用（sys_call)

## 控制台相关：需要修改的文件：console.c, defs.h(在里面增加函数声明）， 然后再在相关文件中设置系统调用。
* setCursorPos(int x,int y) ： 设置控制台的光标位置到(x, y)，从0开始。
* writeAt(int x, int y,int val) : 在屏幕位置(x, y)处显示字符val。 先调用setCursorPos，再调用cgaputc().
* void clearScreen() : 将屏幕清空，然后将光标置为(0, 0)。
* copyFromTextToScreen(char* text,int pos) : 将字符串text复制到显示器的相对位置pos后面, 要求： 0 <= pos < 25*80.
* setBufferFlag(int flag) : 改变控制台缓存设置。 在xv6的控制台（console）的实现中，设置了一个输入缓冲区，用户键入的字符，只有在按下回车后才会通过stdin被用户程序感知。若是将缓存关闭，键入一个字符就会立即通知用户，这样就能提高实时交互性。
* setShowAtOnce(int) ：键入的字符是否显示在屏幕上，如果不想，就将其设置为0.（名字取得好像有点不适合。。。是显不显示在屏幕上，而不是 是不是立即显示）
* getCharOnCursor()：获得光标所在位置处的字符。



-----------------------------------------------------
# 其他
* 控制台真实的屏幕大小为25 x 80, 但是在xv6中，只用到了24 x 80. 只需要改动几个数字，就能完整的用到25行了：在cgaputc()中的最后面// Scroll up那一段改变一下数字和条件。

# 增加系统调用，需要修改下面的文件：
* syscall.h：增加宏定义 #define SYS_write 16
* syscall.c：1. 增加外部函数引用 extern int sys_write(void); 2. 在数组static int (*syscalls[])(void)中增加一行 “[SYS_write]   sys_write,”
* sysfile.c 或者 sysproc.c：增加函数定义 int sys_write(void){}，实现具体的系统调用功能，可以调用其他文件中没有被static修饰的函数来实现真正的功能。 注：这2个文件中的头文件不同，故能调用的函数会有些差异。	
* usys.S ：增加SYSCALL(write)
* user.h ： 进行函数声明 int write(int, void*, int);
* defs.h ： 可能会增加一些函数声明，如果一些内核文件增加了函数的话（没有用static修饰）。

# 调试xv6：参考链接：http://zoo.cs.yale.edu/classes/cs422/2013/lec/l2-hw
在xv6源码目录中进行如下操作：
1. 键入命令：make qemu-gdb。启动gdb调试。此时程序会打开qemu模拟器，然后等待加载内核。
2. 在相同目录下再次打开一个命令行窗口，键入命令：gdb kernel 。启动gdb来加载内核的相关内容
3. 在打开了gdb的窗口（有（gdb）提示）键入： (gdb) target remote localhost:port  。与前面使用make qemu-gdb打开的xv6进行关联，才能进行下一步。注：要将port换成对应的数字，在另一个窗口可以看到。
4. 接下来就可以设置断点，进行调试了。




