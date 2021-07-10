# FC CPU

这节具体讲解CPU的指令部分。

## 一、寻址方式
所谓寻址方式，就是CPU根据指令中给出的地址信息来寻找有效地址的方式，是确定本条指令的数据地址以及下一条要执行的指令地址的方法。FC中一共有12种寻址方式

### 0、累加器寻址（Accumulator）
操作对象是累加器，实际上是一种隐含寻址，因此与下一条合并为一个

### 1、隐含寻址（IMPlied Addressing）
单字节指令，在指令之中隐含了操作地址，例如：TAX，将A中的值传递给X
寻址函数不需要任何操作，直接return即可。

### 2、立即寻址（IMMediate Addressing）
双字节指令，指令的操作数部分直接给出数据，即立即数。例如：LDA #$0A，即A=0x0A。
（操作数前加#表示立即数）
寻址函数：

```c
address = pc++;
```

### 3、绝对寻址（ABSolute Addressing）
三字节指令，指令的操作数给出的是：操作数在存储器中的有效地址。例如：
LDA $31F6，即A=[$31F6]
寻址函数：

```c
address = READ(pc++);	// 低8位
address |= READ(pc++)<<8;	// 高8位
```

### 4、零页寻址（Zero-Page(0) Absolute Addressing）
上一节说过，前256字节内存被称为零页。
双字节指令，绝对寻址中如果高字节为0，则可变为零页寻址，能够节约一个字节, 速度较快, 所以经常使用的数据可以放在零页.
寻址函数：

```c
address = READ(pc++);
```

### 5、绝对X变址（ABsolute X Indexed Addressing）
三字节指令，这种寻址方式是将一个16位的直接地址作为基地址，然后和变址寄存器X的内容相加，结果就是真正的有效地址。例如：LDA $31F6, X，即A=0x31F6[X]
寻址函数：

```c
address = READ(pc++);	// 低8位
address |= READ(pc++)<<8;	// 高8位
address += X;
```

### 6、绝对Y变址（ABsolute Y Indexed Addressing）
同上，最后改为加Y

### 7、零页X编制（Zero-Page X Indexed Addressing）
双字节指令，与5基本相同，当高地址为0时节约了一个字节
寻址函数：

```c
address = READ(pc++);
address += X;
address &= 0xFF;
```

### 8、零页Y编制（Zero-Page Y Indexed Addressing）
同上，X改为Y

### 9、间接寻址（INDirect Addressing）
三字节指令，在 6502中，仅仅用于无条件跳转指令JMP。这条指令该寻址方式中，操作数给出的是间接地址，间接地址是指存放操作数有效地址的地址。例如：
JMP ($215F)，即跳转至$215F地址开始两字节指向的地址（地址的地址）
比如说内存中有如下信息：
1、地址$215F存储值$76
2、地址$2160存储值$30
3、则JMP最终跳转至$3076

该指令存在**硬件缺陷**，JMP xxFF将无法正常工作。当我们尝试读取$10FF时，理论上应当读取10FF和1100两个字节，但是由于高位变化引起换页，FC会错误的读取$10FF和$1000这两个字节。我们需要模拟这个bug，否则会引起错误。
寻址函数：

```c
address_lo8 = READ(pc++); 
address_hi8 = READ(pc++) << 8;
tmp = address_hi8 | address_lo8;
if(address_lo8 == 0xFF)	// 刻意复现bug
    address = (READ(tmp & 0xFF00) << 8) + (READ(tmp));
else
    address = (READ(tmp + 1) << 8) + (READ(tmp));
```

### 10、间接X变址IZX（Pre-Indexed indirect Addressing）
双字节指令，例如：LDA ($3E, X)，首先$3E+X获得一个间接地址，然后分别以[$3E+X]作为低8位，[$3E+X+1]作为高8位，获得16位有效地址。
寻址函数：

```c
ptr = READ(pc++);
lo8 = READ((ptr + X) & 0x00FF);
hi8 = READ((ptr + X + 1) & 0x00FF);
address = (hi8 << 8) + lo8;
```

### 11、间接Y变址IZY（Post-Indexed indirect Addressing）
双字节指令，例如：LDA ($4C), Y，首先对$4C指出的零页地址做一次间接寻址，得到低8位；然后对$4C+1做一次间接寻址，得到高8位；合并为16位地址后，加上Y得到有效地址。
寻址函数：

```c
ptr = READ(pc++);
lo8 = READ(ptr & 0x00FF);
hi8 = READ((ptr + 1) & 0x00FF);
address = (hi8 << 8) + lo8 + Y;
```

### 12、相对寻址（RELative Addressing）
该寻址仅用于条件转移指令，指令长度为2个字节。第1字节为操作码，第2字节为条件转移指令的跳转步长，又叫偏移量D。D用补码表示，可正可负。例如：
BEQ $A7（由于第7号位置为1，故为补码表示的-39。当Z=1时，PC-39；否则不跳转，顺序执行）
寻址函数：

```c
address_rel = READ(pc++);
if(address_rel & 0x80)	// 保证补码规则，负数高位补1
    address_rel |= 0xFF00;
```

注意上面的函数中我只获取了相对地址，或者说偏移量。实际地址将在操作码中处理。

以上便是FC的12种寻址方式，不同的操作码可以和不同的寻址方式组合，FC的指令集和一共有256个，不过有105个为非官方指令（或未定义指令）。
PS：上述寻址方式中，有些涉及页面切换会导致所需时钟周期增加，代码中进行了还原。

## 二、操作码
FC一共有56个操作码，用一个表格显示如下：
|       |       |       |       |       |       |       |       |       |       |       |       |       |       |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
|  ADC  |  AND  |  ASL  |  BCC  |  BCS  |  BEQ  |  BIT  |  BMI  |  BNE  |  BPL  |  BRK  |  BVC  |  BVS  |  CLC  |
|  CLD  |  CLI  |  CLV  |  CMP  |  CPX  |  CPY  |  DEC  |  DEX  |  DEY  |  EOR  |  INC  |  INX  |  INY  |  JMP  |
|  JSR  |  LDA  |  LDX  |  LDY  |  LSR  |  NOP  |  ORA  |  PHA  |  PHP  |  PLA  |  PLP  |  ROL  |  ROR  |  RTI  |
|  RTS  |  SBC  |  SEC  |  SED  |  SEI  |  STA  |  STX  |  STY  |  TAX  |  TAY  |  TSX  |  TXA  |  TXS  |  TYA  |


显然我不会在此详细描述56种操作码的具体功能，可以在6502 reference上查看具体信息（包括某些情况导致的额外时钟周期）。
但是我们可以发现规律，很容易实现大部分指令：

1. STA、STX、STY，均为将某寄存器的数据存入内存给定地址处
2. LDA、LDX、LDY，均为将内存给定地址处数据存入某寄存器
3. TAX、TAY、TSX、TXA、TXS、TYA，均为两个寄存器之间的数据交换
4. INC、INX、INY、DEC、DEX、DEY，均为某寄存器的自增或自减
5. BMI、BNE、BPL、BVC、BVS，均为根据某标志位决定是否跳转
6. CLC、CLD、CLI、CLV、SEC、SED、SEI，均为设置某标志位的值
   
综上，大部分指令都非常简单，实际上，整个实现过程中最困难的指令是ADC和SBC
即加法和减法，因为溢出标志的设置很讲究。

我们以加法为例，讨论下什么情况下是溢出：

1. 正数 + 正数 = 正数，正确
2. 正数 + 正数 = 负数，溢出
3. 正数 + 负数，不可能溢出
4. 负数 + 负数 = 负数，正确
5. 负数 + 负数 = 正数，溢出

我们以真值表的形式把上面几种情况写出来：（0正1负，^表示异或，~表示非）

| A(ccumulator) | M(emory) | R(esult) | V(overflow) |  A^R  |  M^R  |
| :-----------: | :------: | :------: | :---------: | :---: | :---: |
|       0       |    0     |    0     |      0      |   0   |   0   |
|       0       |    0     |    1     |    **1**    | **1** | **1** |
|       0       |    1     |    0     |      0      |   0   |   1   |
|       0       |    1     |    1     |      0      |   1   |   0   |
|       1       |    0     |    0     |      0      |   1   |   0   |
|       1       |    0     |    1     |      0      |   0   |   1   |
|       1       |    1     |    0     |    **1**    | **1** | **1** |
|       1       |    1     |    1     |      0      |   0   |   0   |

由上面的真值表，我们可以令`V=(A^R) & (M^R)`
减法的分析过程类似，略。
