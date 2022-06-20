
## 1 前言

### 1.1 基本介绍

- 编程语言：C语言和ARM汇编语言
- 环境及硬件：Win / Keil 5 / Stm32cubmx / stm32f103xxx（stm32f103zet6,"zet6"仅仅体现在内存容量，工作温度等）
- 参考链接：[ez-rtos](https://github.com/cw1997/ez-rtos)

### 1.2 基本概念

- CPU

    中央处理器，可以按顺序执行硬盘或者 Flash 闪存中存储的程序指令。

- 指令集

    存储于CPU内部，用来引导CPU进行加减运算和控制计算机操作系统的一系列指令集合，本质上是一段二进制机器码。不同的CPU使用的指令集不同，例如Intel的CPU，基础指令集都是X86；手机上的CPU，例如高通骁龙的基础指令集是ARM。

- 指令集架构

    架构主要是指某一个处理器所使用的具体指令集，如imx6ull是基于ArmV7架构的，就是指它是使用armV7指令集。
    指令集是CPU和编译器的设计规范和参考标准。一方面，芯片工程师设计CPU时按照它规定的指令格式为标准实现不同的译码电路来支持指令集各种指令的运行。指令集最终的实现就是微架构，就是CPU内部的各种译码和执行电路。另一方面，编译器工具或IDE在研发时也要以指令集为标准将我们编写的C语言高级程序转换为指令集中规定的各种机器指令。
    指令集也不是—成不变的也会随着应用需求的推动不断迭代更新，不断扩充新的指令。

- 微架构

    前面说了，指令集只是一个设计规范，具体怎么实现这个规范，不同的CPU设计厂商的实现方式也不同。将实现指令集行为的数字电路设计方案称为微架构。这里所使用的stm32f103zet6的CPU指令集架构为 Cortex-M3，简称CM3。开发过程中需要参考《Cortex-M3权威指南》。

- 单片机

    单片机则是将传统电脑中互相分离的CPU，内存，主板，硬盘等硬件整合在了一块芯片上。如stm32f103zet6集成了 Cortex-M3 的 CPU、512KB的Flash（“硬盘”）、64KB的RAM内存以及带有 112 个 GPIO 接口。

- 外设

    单片机内部的外设集成了定时器，USART，USB，SD卡等外设，方便CPU和外界交换数据。

## 2 startup与bootloader

每个os都应该会有一个bootloader，它起到的作用就是初始化 CPU 之后开始执行操作系统，而初始化 CPU 本身的代码通常称为 startup。系统运行的流程为：`CPU通电 -> startup -> bootloader -> 运行用户代码-> 关机断电`。

Cortex-M3的startup与bootloader怎么写？请看《CM3权威指南中文版》的第 49 页，如下图。

![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版1.PNG)

cpu通电后，首先设置stack的位置，即栈顶指针，将它存放在0x00000000这个位置。然后，CPU 将会从 0x00000004 这个位置取出程序执行的起点内存地址，放在 PC 寄存器中。最后， CPU 就开始按照 PC 所指向的内存单元获取机器码指令并且开始按顺序执行下去。

### 2.1 stm32启动文件分析

使用stm32cubmx生成代码时，已经自动生成了startup代码，可参考工程中 `startup_stm32f10x_hd.s` 这个代码文件。这个文件中不仅做了上面两件事（startup和bootLoader），还设置了 heap 堆地址，生成了 NVIC 中断向量表。
文件里面大量使用了“ARM伪指令”，其中有一部分指令是起到分配内存空间的作用，例如 DCD 指令表示分配一段连续的字节区域，并且用指令后面的表达式作为这个字节区域的初始值。文件中的最前面两个 DCD 命令分别将 stack point 和 Reset_Handler 填入了 0x00000000 和 0x00000004 这两个位置，这也和《Cortex-M3权威指南》中的说明一致。
查看函数`Reset_Handler`的定义，可以看出stm32上电后运行的 __main 函数以及相关的初始化代码都视为一个中断的处理函数。里面执行完systeminit之后执行_main函数，在执行完_main函数之后会跳转到`$sub$$main`函数，这个函数才是真正的程序入口，在这里面执行完相应操作之后会跳转到`$super$$main`函数，这个函数才是用户自己写的main函数，操作系统也正是在$sub$$main函数里面先把内核拉起来，然后再接管了用户自己的main函数。

```asm
; Reset handler
Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  __main
                IMPORT  SystemInit
                LDR     R0, =SystemInit
                BLX     R0               
                LDR     R0, =__main
                BX      R0
                ENDP
```

### 2.2 SystemInit和用户main函数

`Systeminit`函数不是很重要，进入函数SystemInit的定义，里面一般会进行系统时钟的配置，以及一些向量表地址等声明操作。而本文工程通过是stm32cubmx生成，对时钟的配置（即对RCC寄存器进行设置）生成在main函数的`SystemClock_Config`中。

`mian`函数中，首先重置所有外围设备、初始化闪存接口和Systick，然后配置系统时钟，接着初始化一些外设，接着执行了 os_init()函数，通过 create_task() 创建了一些用户级任务，最后执行 os_start() 开始了真正的操作系统执行。其中，除了os_init和os_start是操作系统本身提供的代码（内核级代码）外，其他都是用户自己写的代码（用户级代码）。在工业级 OS 中，为了保证系统的安全性，通常会严格区分用户级和系统级代码，例如在X86上有独立的寄存器标识当前运行的程序级别（ring0 - ring3），但是此处为了方便没有做那么严格的等级区分。

### 2.3 bootloader

CPU完成初始化后，还需要初始化操作系统的运行环境（比如任务按照时间片轮训调度，需要设置轮训时间间隔；内存管理器，需要设置malloc函数从那个内存区域开始分配内存，生成内存分配链表等等），这些初始化的操作通常就是 bootloader 所做的事情，均在os_init和os_start函数中完成。

```cpp
void os_init() 
{	
    // 1.初始化任务切换器
    init_task(); 
    // 2.初始化 memory 内存管理器
    init_memory(); 
    // 3.初始化 io 接口
    init_io(); 
}

void os_start() 
{
    // 初始化了 PendSV 和 SysTick ，主要用于任务切换
    PendSV_init(); 
    SysTick_init();
}
```

## 3 任务切换器

多任务切换器可以实现**每隔一段时间**就**切换**到**下一个任务**，只要保证每个任务都能每隔一个很短的时间就得到执行，这些任务仿佛在同时运行。

- 每隔一段时间

    STM32 以及主流的 CPU 都有提供定时器（Timer），定时器会在定时结束后发起一个时间中断，可以在这个中断中进行修改 PC 寄存器的任务切换操作。

- 切换

    任何 CPU 都有一个 PC 寄存器，这个寄存器指示着当前 CPU 正在执行内存中哪个地址上的指令。只需要修改 PC 寄存器就能过改变操作系统当前执行的指令。

- 下一个任务

    通过维护一个任务列表，从这个任务列表中依次找到每个任务。本项目中的这个列表叫做 tcb_list。

任务切换一般是在没有任何其他中断执行的情况下，而stm32中 PendSV 的中断抢占优先级最低（意味着如果一旦出现其他中断来临，那么其他中断可以抢占 CPU 的使用权，优先执行完其他较为重要的中断处理以后再回来继续任务切换），所以使用 PendSV 中断来完成任务切换的工作。

### 3.1 初始化任务切换器

这部分代码位于`task.c`、`task.h`以及`task.s`中。

头文件`task.h`中，设置`CONFIG_MAX_TASK_NUM`值为8，表示最大可以执行的任务数量。定义了TCB任务控制块的结构体，名称为`Task_Control_Block_t`，记录了任务的一些执行信息。此外，还有一个成员类型为Task_Control_Block_t的数组名为 `tcb_list`，这就是前面所说的任务列表。

```cpp
#define STACK_IDLE_SIZE 32
stack_t stack_idle[STACK_IDLE_SIZE];
void init_task() {
    create_task(task_idle, 0, stack_idle, STACK_IDLE_SIZE);
    current_TCB = &tcb_list[0];
    __asm {
        // PSP = 0
        MOV R0, 0x0
        MSR PSP, R0
    }
}
```
函数中调用 create_task 函数创建了一个 idle 任务（空闲任务），并且将 current_TCB 变量，也就是当前执行的任务的 TCB 设为我们创建的第一个 idle 任务，然后又内联了一段汇编代码，这个汇编代码将 PSP 的值设置为 0x00000000（这里设置PSP值为0，目的是为了打一个标记，表示这是操作系统第一次调度任务，后续的任务切换器的核心代码需要知道是否是第一次开始调度任务。因为第一次调度的处理裸机和后面无数次调度不一样）。

### 3.2 create_task函数

```cpp
uint16_t create_task(void *function, void *arguements, stack_t *stack, int stack_size) {
    if (next_task_id > CONFIG_MAX_TASK_NUM) {
        return 0;
    }

    stack_t *stack_top = &stack[stack_size];
    // auto save by Cortex-M3
    *(--stack_top) = (stack_t)0x01000000u; // xPSR bit 24 = 1
    *(--stack_top) = (stack_t)function; // R15 PC function entry point
    *(--stack_top) = (stack_t)0x14141414u; // R14 LR
    *(--stack_top) = (stack_t)0x12121212u; // R12
    *(--stack_top) = (stack_t)0x03030303u; // R3
    *(--stack_top) = (stack_t)0x02020202u; // R2
    *(--stack_top) = (stack_t)0x01010101u; // R1
    *(--stack_top) = (stack_t)arguements; // R0
    // manual save by developer
    *(--stack_top) = (stack_t)0x11111111u; // R11
    *(--stack_top) = (stack_t)0x10101010u; // R10
    *(--stack_top) = (stack_t)0x09090909u; // R9
    *(--stack_top) = (stack_t)0x08080808u; // R8
    *(--stack_top) = (stack_t)0x07070707u; // R7
    *(--stack_top) = (stack_t)0x06060606u; // R6
    *(--stack_top) = (stack_t)0x05050505u; // R5
    *(--stack_top) = (stack_t)0x04040404u; // R4
    tcb_list[next_task_id].stack = stack_top;

    return next_task_id++;
}
```
首先判断当前已经创建的任务数量是否已经超过了最大的任务数量了，如果超过的话，就会导致存储任务列表的 tcb_list 数组发生越界，因此就直接返回0，不再执行后续操作。

如果一切正常，那么就创建该任务需要使用的栈段。栈段的初始化为什么是这样写的呢？（原作者说和指令集架构行为有关，没做进一步解释。）

下面还需要了解一个任务在运行时会产生哪些上下文信息，以及这些上下文信息在遇到任务切换（中断请求）的时候会被 Cortex-M3 CPU 做怎样的处理。

### 3.3 Cortex-M3 双栈机制

任何函数在执行的时候都需要有自己的栈段，因为任何一个程序都由多个函数组成，并且都是从 main() 函数开始，依次调用，整个函数就像一条链子一样层层调用下去。由于函数存在嵌套调用的原因，因此需要栈这种数据结构来维护每个嵌套调用中的上下文信息（通常是调用之后的返回值，以及返回时需要恢复的寄存器信息）。

正常情况下通常只需要一个栈段就够了。但是 Cortex-M3 为特权级和用户级函数划分了两个栈段（MSP和PSP。通常中断处理程序、内核级代码都会使用 MSP 栈段，而剩下的用户级代码则使用 PSP 栈段），通过 CONTROL寄存器进行切换，或者在使用 LR 指令返回的时候设置第3个bit为1让中断处理程序返回到普通代码的时候切换所使用的栈段。

![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版2.PNG)

### 3.4 中断中的上下文保存

中断到来时，CPU 中的 NVIC 中断控制器会去查询中断向量表，找到该中断对应的处理程序（Handler），然后调用这个处理程序。这个和函数调用很像。但是区别是中断处理程序是内核级代码，它一定会使用 MSP 这个栈，并且可以执行特权级指令。
此外，在进入中断处理程序之前，会先保存之前的上下文（寄存器）到其 SP 指针所指向的栈段（若进入中断处理程序之前正在执行操作系统的内核级代码，则 SP 会指向 MSP，否则 SP 指向 PSP）。毕竟进入中断处理程序也是一种特殊的函数调用，因此也要像普通函数一样保存上下文，以便处理完中断程序之后恢复之前执行的状态。

![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版3.PNG)

根据《CM3权威指南》可知，CM3 的 CPU 内核只会自动保存 PSR，R0，R1，R2，R3，R12，LR（R15），PC（R15），其他寄存器并不会自动保存。

### 3.5 任务切换步骤

任务切换需要三步：保存上下文 -> 修改栈指针 -> 恢复上下文 。
其中，切换任务的本质是修改 PC 寄存器，PC 寄存器本质上就是 R15 寄存器，它和其他的寄存器 R0 - R14 也同属于上下文的一部分，所以我们可以直接将整个上下文视为一个整体。

`./os/task.s`中的 PendSV_Handler 函数便是整个任务切换的核心代码。arm的汇编指令（无论是真指令还是伪指令）必须要有一格缩进，可以是 tab 也可以是空格。所有没有缩进的代码只有标号（解释下面的代码段所做的功能）和注释。
```asm
PendSV_Handler PROC	
    EXPORT PendSV_Handler
        
; turn off all interrupt
    CPSID I


; check PSP, if PSP == 0, this is the first task switch
; so we can skip 'save context' and 'select next TCB' step
    MRS R0, PSP
    ; if r0 == 0, jump to restore_context
    ; LDR R1, =is_first_switch_task
    CBZ R0, restore_context

save_context
    MRS R0, PSP
    STMDB R0!, {R4-R11}
    LDR R1, =current_TCB
    LDR R1, [R1]
    STR R0, [R1]

select_next_TCB
    PUSH {LR}
    BL switch_current_TCB
    POP {LR}

restore_context
    LDR R0, =current_TCB
    LDR R0, [R0]
    LDR R0, [R0]
    LDMIA R0!, {R4-R11}
    MSR PSP, R0
    ORR LR, LR, #0x4 ; R1 |= 0x04 : lr |= 32'b0000_0000_0000_0100

; turn on all interrupt
    CPSIE I

; return
    BX LR

    ENDP
```
- PendSV_Handler 到 save_context 这两个标号之间的代码分析：
    ```asm
    PendSV_Handler PROC	
        EXPORT PendSV_Handler
        
    ; turn off all interrupt
        CPSID I


    ; check PSP, if PSP == 0, this is the first task switch
    ; so we can skip 'save context' and 'select next TCB' step
        MRS R0, PSP
        ; if r0 == 0, jump to restore_context
        ; LDR R1, =is_first_switch_task
        CBZ R0, restore_context
    ```
    首先，通过 EXPORT 伪指令导出了一个 标识符 PendSV_Handler，这个类似于 C语言中的 extern 关键词，这是为了告诉连接器遇到需要 PendSV_Handler 地址的时候直接从这里取地址；

    然后，使用 Cortex-M3 的 CPSID I 指令关闭了所有中断。因为任务切换涉及到修改栈段的操作，这一步不允许任何操作打断，否则可能会导致恢复或者保存的栈段不完整，甚至破坏请求打断操作的函数执行；

    接着，通过 MRS 指令读取特殊功能寄存器 PSP 的值，通过 CBZ 命令判断是否为 0。如果是 0 则直接跳转到 restore_context 标号下面的代码。
    之前在init_task函数中，设置了 PSP 的值为 0 ，以标记整个 OS 是否是第一次做任务切换。如果PSP为0，即第一次做任务切换时，由于没有任何一个任务被执行过，就不需要执行 save_context 这个保存上下文（保护现场）的操作，只需要恢复需要执行的第一个任务的上下文到寄存器中就好了；否则，如果 PSP 不是 0 的话说明之前已经进行过任务切换的操作了，则需要保存当前任务的上下文，也就是代码继续往下走。

- save_context 保存上下文
    ```asm
    save_context
        MRS R0, PSP
        STMDB R0!, {R4-R11}
        LDR R1, =current_TCB
        LDR R1, [R1]
        STR R0, [R1]
    ```
    首先，通过 MRS 指令将 PSP 的值读入到 R0 寄存器中，然后通过 STMDB 指令将 R4-R11 寄存器的值全部都 push 进 R0 中（CM3本身会自动保存一部分上下文）。STMDB 指令是地址先减而后完成操作，因为 ARM Cortex-M3 中规定 stack 为满减栈，也就是往 CM3 的 stack 中 push 数据实际上栈顶地址是往下减少的。
    然后，将 current_TCB 这个变量的值读入 R1，由于 currentTCB 变量是一个指针变量，因此它的值仅仅是一个 TCB 结构体的内存地址。而这个结构体的第一个成员是一个 stack_t 类型的数组，我们还需要获得这个栈数组本身的地址，所以还需要一个 `LDR R1, [R1]` 指令将这个栈数组的地址读出来，并且存放到 R1 中。最后我们将之前满减压栈之后的新的栈顶地址存放到 R1 中，也就是修改结构体中 stack 成员的指针值。
    总结：保存上下文阶段，我们先将 PSP 的栈顶读出，然后依次保存到当前任务的 TCB 中的 stack 指针指向的栈段数组中，并且修改栈顶位置（栈顶指针的值），然后保存到 TCB 中的 stack 指针的值。

- select_next_TCB 选择下一个待执行的任务
    ```asm
    select_next_TCB
        PUSH {LR}
        BL switch_current_TCB
        POP {LR}
    ```
    switch_current_TCB是一个 C语言函数的指针，这里通过汇编调用C语言函数，因此我们需要手动保存 LR 寄存器，然后再用 BL 指令跳转过去。如果不使用 PUSH 保存 LR 寄存器，那么 C语言编译出来的汇编指令会直接覆盖我们整个 PendSV_Handler 中断处理函数的返回地址。然后最后在调用结束后我们还需要使用 POP LR 指令恢复之前保存的 LR 寄存器中的返回地址，确保 PendSV_Handler 中断处理函数在执行完任务切换操作以后能够正确的返回。

    ```
    void switch_current_TCB() {
        if (current_task_id >= next_task_id - 1) {
            // check if not create any task, switch to idle task
            if (next_task_id >= 2) {
                current_task_id = 1;
            } else {
                current_task_id = 0;
                current_TCB = &tcb_list[0];
                return;
            }
        } else {
            ++current_task_id;
        }
        // check if a task is in delay
        // if all tasks are in waiting for delay
        current_TCB = &tcb_list[0];
        for (int i = current_task_id; i < next_task_id; ++i) {
            Task_Control_Block_t *checking_TCB = &tcb_list[i];
            // BUG!! now ticks is not update
            if (checking_TCB->delay_ticks == 0 || checking_TCB->delay_ticks <= now_tick) {
                checking_TCB->delay_ticks = 0;
                current_TCB = checking_TCB;
                current_task_id = i;
                return;
            }				
        }
    }
    ```
    这个函数中，不停循环重复执行任务列表中的所有任务，即判断当前切换的任务 id 是否已经超过了最大任务数量，**如果超过的话就修改为 1**，也就是从第一个任务开始执行，否则就任务 id 加 1，也就是选择下一个任务的 TCB 作为当前任务TCB 赋值给 current_TCB。此外，还需要判断当前的最大任务数量是否已经超过 1，如果超过 1 说明不止有 idle 任务，否则说明用户没有创建任何任务，整个任务列表里面只有 idle 这一个空闲任务。
    剩下的代码中与sleep函数的实现有关，后面再介绍。
    总结：经过上面的操作，已经将 current_TCB （指向当前任务 TCB 的指针）修改为了下一个待执行任务的 TCB 指针，即告诉后面负责恢复上下文的代码应该要从哪里恢复出上下文。
- restore_context 恢复上下文
    恢复上下文这一个步骤才是真正做了任务切换，里面将新的任务的上下文中事先在栈段里面保存的 PC 值恢复到了 CPU 的 PC 寄存器中。
    ```asm
    restore_context
        LDR R0, =current_TCB
        LDR R0, [R0]
        LDR R0, [R0]
        LDMIA R0!, {R4-R11}
        MSR PSP, R0
        ORR LR, LR, #0x4 ; R1 |= 0x04 : lr |= 32'b0000_0000_0000_0100
    ```
    先去读取 current_TCB 指针指向的 TCB 结构体地址，然后从这个地址中再取出 stack 这个指针类型的成员所指向的栈顶指针，并且 通过 MSR 指令赋值给 PSP 寄存器，也就是将 CPU 的任务栈段“偷梁换柱”替换成了新的任务的栈段。然后等到 PendSV_Handler 等会儿返回的时候，根据 Cortex-M3 自动从栈段中恢复 PC，LR，R0 - R3 的机制，就会自动将 PC 寄存器恢复成新任务的函数指针。这也就实现了任务切换。
    此外，`ORR LR, LR, #0x4 `将 LR 的第 2 个 bit（大端序，从右往左数，从 0 开始计算的话则是第 2 位）为 1，是为了中断处理函数返回的时候则会自动使用 PSP 作为返回以后使用的栈。
    ![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版4.PNG)
- 收尾工作的代码
    ```asm
    ; turn on all interrupt
        CPSIE I

    ; return
        BX LR
    ```
    就是启用全部中断（因为之前禁用过），然后使用 LR 寄存器返回即可。

在这段汇编代码后面还有一个 NOP 指令，是为了保证代码能够 4byte 对齐，这个是编译器的要求，你不对齐也没关系，只是编译器会报一个 warning 警告，不对齐会影响 CPU 取指令的效率等等。

## 4 任务切换的时机

根据前面知道，只要调用 PendSV_Handler 这个函数， OS 就会开始切换任务，但是并没有发现显式调用了 PendSV_Handler 这个函数的地方。那什么时候自动定时切换任务勒？

### 4.1 SysTick

为了便于移植到更多的单片机上，尽可能使用 CPU 内核提供的资源，而非外设。SysTick 就是 ARM 为 Cortex-M3 这款微架构提供的内核级硬件定时器，这里用于定时切换。

![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版5.PNG)

它其实和各种单片机的定时器使用差别不大。都是先往 Val 寄存器中填充一个预装载值，然后定时器便会按照指定的时钟频率对 Val 执行自减一操作（Val = Val - 1），当 Val 被自减到 0 之后便会触发一个中断，然后 NVIC 便会去执行 NVIC 中断向量表中指定偏移的指针指向的那个中断处理函数。

![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版6.PNG)

```cpp
void SysTick_init() {
    SysTick->LOAD  = SYSTICK_LOAD;
    SysTick->VAL   = 0;
    SysTick->CTRL  |=  SysTick_CTRL_CLKSOURCE_Msk | // 0=AHB/8；1=AHB
                      SysTick_CTRL_TICKINT_Msk   |
                      SysTick_CTRL_ENABLE_Msk	|
                      0; 
    NVIC_SetPriority(SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
}
```
初始化 SysTick 的各项寄存器，其中用到了一个 SYSTICK_LOAD 常量，表示每次计数器（SysTick 定时器是自减定时器）都初始化为 SYSTICK_LOAD 这个值。宏定义为：
```cpp
#define SYSTICK_LOAD (CONFIG_OS_TICK_TIME_US * CONFIG_OS_SYSTICK_CLK - 1)
```
```cpp
// unit: MHz
#define CONFIG_SYSCLK 72
// unit: us
#define CONFIG_OS_SYSCLK_DIV 1
#define CONFIG_OS_SYSTICK_CLK (CONFIG_SYSCLK / CONFIG_OS_SYSCLK_DIV)
// 10ms
#define CONFIG_OS_TICK_TIME_US 10000
```
它为系统配置的每个 tick 的时间片大小乘以 SysTick_CLK 系统时钟的大小减 1。
CONFIG_OS_TICK_TIME_US 的单位是 us，表示每个时间片应该拥有多少个微秒； CONFIG_OS_SYSTICK_CLK 的单位是 MHz 。

然后 Ctrl 寄存器中设置了两个 bit 为 1，分别是使能（enable）和需要触发中断。

NVIC_SetPriority 表示设置 SysTick 的优先级，这里设置为最低抢占优先级和最高亚优先级，因为外部的响应应该要优先于任务切换操作。

SysTick 的初始化我们已经做完了，那么我们只需要在 SysTick_Handler 这个函数中执行任务切换的那段汇编代码。

### 4.2 PendSV

继续看到SysTick_Handler函数，先是对 now_tick 这个值进行自增，然后调用了 switch_task() 函数，而 switch_task() 函数中也只是设置了一个寄存器的值为 1，并没有做其他操作。
```cpp
void switch_task() {
    // set pendsv
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void SysTick_Handler(void) {
    ++now_tick;
    if (task_switch_enable) {
        switch_task();
    }
}
```

先了解一下系统调用和可悬起的系统调用两个概念。
在工业级 OS 中，为了保证系统安全性，通常不允许用户级代码直接访问硬件资源，例如直接操作硬件寄存器或者修改 NVIC 中断向量表都是不被允许的。这种操作只能在内核级函数中执行。但是有的时候用户代码有确实需要调用硬件，这该怎么办呢？
OS 为了实现用户代码操作硬件，就事先把一些操作硬件的代码封装成系统调用函数，通过中断的方式请求系统调用，系统调用进入内核级再去操作具体的硬件。由于系统调用是 OS 的开发者编写的，是经过验证和测试的，不会有什么危险操作。因此既满足了用户需要操作硬件的需求，又没有让用户代码直接去操作硬件。
系统调用和其他普通的中断函数一样会抢占其他任务，如果为了保证外部中断能够及时得到响应，最好的办法是将系统调用的抢占优先级设置成最低，亚优先级也设置成最低。

普通的系统调用中这样设置并没有什么问题。但是如果是 SysTick ，它只会在当 Val 被自减到 0 的时候才请求中断，为别的值的时候就会取消中断的请求。
因此当外部中断来临的周期和任务切换的定时周期相近的话，可能会导致每次 SysTick 的值刚刚快被自减到 0 的时候，外部中断来临了并且抢占了 SysTick 的中断，等外部中断处理函数执行完毕以后，SysTick 又重新装载了新的值（因为 SysTick 是内核级定时器，它永远都在不停的自动重装载 Load 寄存器和自减 Val 寄存器，除了时钟停止以外并不会因为任何状况停止计时），那么 SysTick 的中断就会被错过，因此任务切换的操作也就不会被执行。这个情况也称为“中断共振”，指的是两个中断的产生周期几乎一致。

![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版7.PNG)

为了解决这个问题，CM3 微架构专门引入了 PendSV 可悬起系统调用。你只需要设置 PendSV 的悬起位为 1，那么只要系统有空闲（这里的空闲指的是没有任何中断请求发生）的情况下就会执行 PendSV_Handler 函数。因此 SysTick 到点后只需要设置一下 PendSV 的悬起位，而不需要做实际的任务切换操作，而是先执行更高优先级的中断处理函数，执行完以后系统发现有空闲了，并且 PendSV 的悬起位为 1，才会去执行 PendSV 的中断处理函数。

![](https://cdn.jsdelivr.net/gh/zhekunren/blog_image/img/others/2022/CM3权威指南中文版8.PNG)

## 5 sleep实现

### 5.1 指令执行的时间

编译器将代码编译成一条条的机器码指令，这些指令会被 CPU 执行。每条指令执行都需要一段时间。
一条指令具体需要执行多少时间，和 CPU 底层电路设计，以及 CPU 的主频有关，CPU 的底层电路设计决定了一条指令执行需要多少个时钟 tick，而 CPU 的主频决定了每秒有多少个时钟 tick。

### 5.2 在 OS 中实现 sleep 函数

由于OS要同时执行多个任务，因此在 OS 中必须让一个任务在延时的时候，能够让出 CPU 时间片执行其他任务，实现 CPU 最大化利用。

SysTick 定时切换任务，并且每次 SysTick 中断响应时，对 now_tick 变量进行自增。
假设任务 A 需要 sleep 1 秒钟，那么我们可以在 TCB（任务控制块）中将当前 now_tick 加上 1 秒钟的 tick 数量，也就是 1 秒后的 now_tick 值写入 TCB 中的下一次执行时间。每次在任务切换的时候都需要检查一下任务列表中是否有下一次执行时间大于等于 now_tick 的任务，如果有就立即切换到他，否则就按部就班切换到下一个任务，如果全部任务都小于 now_tick ，则说明全部任务都没有到达延时时间，都处于延时状态，这种没有任何任务可以执行的情况下，OS 应该切换到 idle 任务执行。

此外，还需要考虑些边界条件。比如在每次新建任务都将 TCB 中下一次执行时间默认设为 0 ，因为 0 说明下一次执行该任务的时间应该是在 OS 刚刚启动的时候。

```cpp
void sleep(uint32_t us) {
    uint32_t delay_ticks = us / CONFIG_OS_TICK_TIME_US;
    uint32_t target_tick = now_tick + delay_ticks;
    current_TCB->delay_ticks = target_tick;
    switch_task();
}
```

先通过 us / CONFIG_OS_TICK_TIME_US 计算出需要延时的 tick 数量，然后通过 target_tick = now_tick + delay_ticks 计算出延时到达时的 tick 值，再通过 current_TCB->delay_ticks = target_tick 将延时到达时的 tick 值写入到任务列表的响应字段上，最后通过手动执行 switch_task() 发起任务切换，让任务切换器切换到其他能够执行的任务上，也就是主动让出时间片。

为了配合sleep 函数的实现，任务切换器也需要做一些小小的改进，继续分析函数switch_current_TCB。
```cpp
void switch_current_TCB() {
    /** ...... **/
    // check if a task is in delay
    // if all tasks are in waiting for delay
    current_TCB = &tcb_list[0];
    for (int i = current_task_id; i < next_task_id; ++i) {
        Task_Control_Block_t *checking_TCB = &tcb_list[i];
        // BUG!! now ticks is not update
        if (checking_TCB->delay_ticks == 0 || checking_TCB->delay_ticks <= now_tick) {
            checking_TCB->delay_ticks = 0;
            current_TCB = checking_TCB;
            current_task_id = i;
            return;
        }				
    }
}
```
主要改进在 switch_current_TCB() 函数中，这里需要额外增加判断 checking_TCB->delay_ticks 是否小于等于 now_tick 的逻辑，如果小于则将当前任务指针切换到那个小于等于 now_tick 的任务，否则就切换到默认的 0 号任务也就是 idle 任务。

## 6 功能测试

通过 sleep 函数，分别在两个不同的 task 中设置每隔 DELAY_US（500000 us = 500 ms）翻转一次 LED 。

```cpp
/** .... */

#define STACK_SIZE 256

stack_t stack_0[STACK_SIZE];
stack_t stack_1[STACK_SIZE];

#define DELAY_US 1000000

void task_blink_led_0() {
	int n = 0;
	for (;;) {
		n=(n==0)?1:0;
		if(n==0)
		{
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
		}
		else
		{
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);
		}
		sleep(DELAY_US);
	}
}

void task_blink_led_1() {
	int n = 0;
	for (;;) {
		n=(n==0)?1:0;
		if(n==0)
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
		}
		else
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
		}
		sleep(DELAY_US);
	}
}

/** .... **/

int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
	
	os_init();
	
	create_task(task_blink_led_0, 0, stack_0, STACK_SIZE);
	create_task(task_blink_led_1, 0, stack_1, STACK_SIZE);

	os_start();
		
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/** .... **/
```


