## BIOS

### PC的物理地址空间

第一批PC基于16位的Intel 8088处理器，只能寻址1MB的物理内存（通过段寄存器的基址左移4位加上偏移量，即可支持20位的地址空间访问）。因此，早期PC的物理地址空间从0x00000000开始，到0x000FFFFF结束。内存自低向高分为：

- Low Memory：0x00000000 ~ 0x000A0000（640KB）
- VGA Displacy：0x000A0000 ~ 0x000C0000（768KB）
- 16-bit devices，expansion ROMs：0x000C0000 ~ 0x000F0000（960KB）
- BIOS ROM：0x000F0000 ~ 0x00100000（1MB）

其中，标记为“低内存”区域的640KB是早期PC唯一可以使用的RAM，事实上，最早的PC也只能配置16KB、32KB或64KB的内存。其余部分保留作为访问其他I/O设备等硬件使用。其中最重要的是BIOS部分，其处于物理地址空间中最高的64KB，BIOS负责执行最基本的系统初始化工作，例如激活视频卡和检查安装的内存量。在执行一系列初始化工作后，BIOS从诸如软盘、硬盘、CD-ROM或网络之类的适当位置加载操作系统，并将机器的控制权交给操作系统。

从Intel的80286（16位）和80386（32位）处理器开始，1MB的内存限制被打破，处理器可以支持16MB（段寄存器中保存段选择子而不是简单左移4位，并且内存地址总线是24位）和4GB的物理地址空间。为了保持兼容，现代PC在物理内存中有一个“洞”，即从0x000A0000 ~ 0x00100000，将RAM分为了“低”或“常规”内存（第一个640KB）和“扩展”内存（从0x00100000开始）。并且，在32位物理地址空间的最顶部保留一些空间用作32位PCI设备使用。

### Exercise 2

通过单步调试qemu的启动过程，探究BIOS前一部分指令的执行。

打开两个命令行窗口，一个窗口输入**make qemu-nox-gdb**，另一个输入**make gdb**，在输入**make gdb**的窗口中单步执行BIOS指令。

```assembly
The target architecture is set to "i8086".
[f000:fff0]    0xffff0:	ljmp   $0xf000,$0xe05b
0x0000fff0 in ?? ()
+ symbol-file obj/kern/kernel
```

此时机器在实模式下（real mode）运行，第一条执行的指令处于物理地址0xffff0，可以猜测，PC初始时设置[cs, ip] = [0xf000, 0xfff0]，PC一开机就执行此处的指令，这个地址位域ROM的最顶部，指令的操作是跳转到[0xf000, 0xe05b]。

继续单步执行：

```assembly
(gdb) si
[f000:e05b]    0xfe05b:	cmpl   $0x0,%cs:0x6ac8        // 将代码段的某处值与0比较，不相等则跳转到某处
0x0000e05b in ?? ()
(gdb) 
[f000:e062]    0xfe062:	jne    0xfd2e1                // 此处代码不会跳转
													  // 应该是初始时bios检查内存中特定的位置的值是否有异常
0x0000e062 in ?? ()
(gdb) 
[f000:e066]    0xfe066:	xor    %dx,%dx
0x0000e066 in ?? ()
(gdb) 
[f000:e068]    0xfe068:	mov    %dx,%ss                // 设置栈段地址为0
0x0000e068 in ?? ()
(gdb) 
[f000:e06a]    0xfe06a:	mov    $0x7000,%esp           // 设置堆指针的值为0x7000，显然处于low memory
0x0000e06a in ?? ()
(gdb) 
[f000:e070]    0xfe070:	mov    $0xf34c2,%edx
0x0000e070 in ?? ()
(gdb) 
[f000:e076]    0xfe076:	jmp    0xfd15c                // 跳转到另一处更低的地址处执行
0x0000e076 in ?? ()
(gdb) 
[f000:d15c]    0xfd15c:	mov    %eax,%ecx
0x0000d15c in ?? ()
(gdb) 

```

猜测初始时BIOS检测了某处内存的值是否有异常，然后设置了栈段基址和栈指针，指向低内存区域，并跳转到更低的地址执行。

```assembly
(gdb) 
[f000:d15f]    0xfd15f:	cli                           // 关中断
0x0000d15f in ?? ()
(gdb) 
[f000:d160]    0xfd160:	cld                           // 清零FLAGS寄存器中的DF位
													  // 后续某些操作使变址寄存器SI或DI的地址指针自动增加
0x0000d160 in ?? ()
(gdb) si
[f000:d161]    0xfd161:	mov    $0x8f,%eax             
0x0000d161 in ?? ()
(gdb) 
[f000:d167]    0xfd167:	out    %al,$0x70              // 将$al的值写入port 0x70，此处完成NMI disable的功能
0x0000d167 in ?? ()
(gdb) 
[f000:d169]    0xfd169:	in     $0x71,%al              // 从port 0x71(CMOS RAM data port)读取值到$al
0x0000d169 in ?? ()
(gdb) 
[f000:d16b]    0xfd16b:	in     $0x92,%al              // 从port 0x92(PS/2 system control port A)读取值
0x0000d16b in ?? ()
(gdb) 
[f000:d16d]    0xfd16d:	or     $0x2,%al               // 将该值的第1位置1（从0开始） 
0x0000d16d in ?? ()
(gdb) 
[f000:d16f]    0xfd16f:	out    %al,$0x92              // 写回
0x0000d16f in ?? ()
(gdb) 
[f000:d171]    0xfd171:	lidtw  %cs:0x6ab8             // 加载IDT表
0x0000d171 in ?? ()
(gdb) 
[f000:d177]    0xfd177:	lgdtw  %cs:0x6a74             // 加载GDT表
0x0000d177 in ?? ()
(gdb) 
[f000:d17d]    0xfd17d:	mov    %cr0,%eax              // 读取cr0控制器的值
0x0000d17d in ?? ()
(gdb) 
[f000:d180]    0xfd180:	or     $0x1,%eax              // 置最低位（PE）为1
0x0000d180 in ?? ()
(gdb) 
[f000:d184]    0xfd184:	mov    %eax,%cr0              // 更新cr0寄存器，开启PE，进入保护模式
0x0000d184 in ?? ()
(gdb) 
[f000:d187]    0xfd187:	ljmpl  $0x8,$0xfd18f
0x0000d187 in ?? ()
(gdb) 
The target architecture is set to "i386".
=> 0xfd18f:	mov    $0x10,%eax
0x000fd18f in ?? ()
```

此处指令完成关中断以及置零FLAGS寄存器中DF位的工作。然后，操作了3个端口，分别是0x70、0x71和0x92。

如下是关于port 0x70的描述内容：

```
0070-007F ----	CMOS RAM/RTC (Real Time Clock  MC146818)
0070	w	CMOS RAM index register port (ISA, EISA)
		 bit 7	 = 1  NMI disabled
			 = 0  NMI enabled
		 bit 6-0      CMOS RAM index (64 bytes, sometimes 128 bytes)

		any write to 0070 should be followed by an action to 0071
		or the RTC wil be left in an unknown state.
0071	r/w	CMOS RAM data port (ISA, EISA)
```

如上所属，0070-007F 的端口均属于**CMOS RAM/RTC (Real Time Clock  MC146818)**，而0x70为**CMOS RAM index register port (ISA, EISA)**。即它是CMOS RAM的一个索引寄存器，并且写入它的值中的第7位指示了是否禁用NMI(Non Maskable Interrupt) 不可屏蔽中断。而剩余的0-6bit指示CMOS RAM的索引值。写入0x70后必须紧随一个从0x71读取内容的操作，否则RTC会处于未知状态。这也是为什么后面紧跟一个从0x71端口的读操作，却并没有使用读取到的值，就马上对0x92端口进行读操作了。

紧接着，BIOS读取port 0x92的值并将其中第1位置为1然后写回，关于port 0x92的描述如下：

```
0092	r/w	PS/2 system control port A  (port B is at 0061)
		 bit 7-6   any bit set to 1 turns activity light on
		 bit 5	   reserved
		 bit 4 = 1 watchdog timout occurred 
		 bit 3 = 0 RTC/CMOS security lock (on password area) unlocked
		       = 1 CMOS locked (done by POST)
		 bit 2	   reserved
		 bit 1 = 1 indicates A20 active
		 bit 0 = 0 system reset or write
			 1 pulse alternate reset pin (alternate CPU reset)
```

更新其第1位会指示激活A20，关于A20的具体来龙去脉我个人不是很清楚，目前理解是：为了运行8086和8088的程序，默认情况下这一根地址线是关闭的，因为此时是可以访问到高于1MB的物理内存的。在进入保护模式时将其打开。

继续单步调试：

```assembly
=> 0xfd18f:	mov    $0x10,%eax                // 设置$eax的值为0x10
0x000fd18f in ?? ()
(gdb) si 
=> 0xfd194:	mov    %eax,%ds                  // 设置$ds、$ss、$fs、$gs的值为0x10
0x000fd194 in ?? ()
(gdb) 
=> 0xfd196:	mov    %eax,%es
0x000fd196 in ?? ()
(gdb) 
=> 0xfd198:	mov    %eax,%ss
0x000fd198 in ?? ()
(gdb) 
=> 0xfd19a:	mov    %eax,%fs
0x000fd19a in ?? ()
(gdb) 
=> 0xfd19c:	mov    %eax,%gs
0x000fd19c in ?? ()
(gdb) 
=> 0xfd19e:	mov    %ecx,%eax
0x000fd19e in ?? ()
(gdb) 
=> 0xfd1a0:	jmp    *%edx                      // 跳转到$edx保存的地址处执行
0x000fd1a0 in ?? ()
(gdb) p /x $edx
$1 = 0xf34c2
```

这一部分主要设置段寄存器的值，此时段寄存器保存的是段选择子。

后续执行的操作猜测应该是反复调用一些函数，应该是完成其余设备的初始化，以及最重要的，找到可引导的设备，并加载可引导设备中的bootloader程序到0x7c00处，最后跳转到此处执行boot loader程序，完成BIOS的任务。

## Boot Loader

如果磁盘是可引导的，则第一个扇区称为引导扇区，因为这是boot loader程序所在的位置。当BIOS找到了可引导的软盘或者硬盘时，它将512字节的引导扇区加载到物理地址为0x7c00到0x7cff的内存区域，并通过jmp指令跳转到此处，将PC的控制权给boot loader。与BIOS的加载地址类似，这些地址都是随意且固定的，是PC的标准化地址。

接着上一节中的对BIOS的单步调试，在0x7c00处设置断点并执行到此处。gdb显示此时CPU处于i8086，即实模式下（个人猜测切换到实模式下是gdb提示架构为i8086，而切换到保护模式则提示架构为i386）。BIOS中切换到保护模式应该是为了检测是否可以进行模式切换，以及开启A20。

起始处执行的指令仍然是关中断和清零FLAGS寄存器的DF位。

```assembly
gdb) b *0x7c00
Breakpoint 1 at 0x7c00
(gdb) c
Continuing.
The target architecture is set to "i8086".
[   0:7c00] => 0x7c00:	cli    

Breakpoint 1, 0x00007c00 in ?? ()
[   0:7c01] => 0x7c01:	cld    
0x00007c01 in ?? ()
```

boot loader的程序由boot/boot.S和boot/main.c组成。

### Exercise 3

单步调试跟踪boot loader程序的执行过程，并理解boot/boot.S和boot/main.c两个源文件中的程序。

#### boot/boot.S

boot.S中的程序就被加载在0x7c00处，因此boot loader首先从此处开始执行。单步调试过程中，另开一个窗口，打开boot/boot.S汇编程序，对比gdb中的指令和boot.S中的指令（尤其是boot.S中的一些变量，gdb中的指令会直接显示其值），同时，boot.S文件中的注释也能帮助我们理解程序的行为。

- step 1：起始处，即start，分别为指令cli和cld。并设置段寄存器的值为0。值得注意的是此时代码段寄存器cs的值也为0

  ```assembly
  (gdb) si
  [   0:7c02] => 0x7c02:	xor    %ax,%ax            // 清零$ax，用其设置$ds、$es、$ss的值
  0x00007c02 in ?? ()
  (gdb) 
  [   0:7c04] => 0x7c04:	mov    %ax,%ds
  0x00007c04 in ?? ()
  (gdb) 
  [   0:7c06] => 0x7c06:	mov    %ax,%es
  0x00007c06 in ?? ()
  (gdb) 
  [   0:7c08] => 0x7c08:	mov    %ax,%ss
  0x00007c08 in ?? ()
  (gdb) p $ss
  $4 = 0
  (gdb) p $es
  $5 = 0
  (gdb) p $cs
  $6 = 0
  ```

- step 2：开启A20，猜测可能BIOS跳转到boot loader前又关了。或者BIOS仅仅只是想测试之前是否可以打开A20。boot.S中将其分为seta20.1和seta20.2两部分。

  gdb单步调试seta20.1：

  ```
  (gdb) si                                                  // 先等待
  [   0:7c0a] => 0x7c0a:	in     $0x64,%al                  // 读取port 0x64的值
  0x00007c0a in ?? ()
  (gdb) 
  [   0:7c0c] => 0x7c0c:	test   $0x2,%al                   // 检测其第1bit是否置位
  0x00007c0c in ?? ()
  (gdb) 
  [   0:7c0e] => 0x7c0e:	jne    0x7c0a                     // 若置位则再次读取，此处loop直到第1bit为0才停止
  0x00007c0e in ?? ()
  (gdb) 
  [   0:7c10] => 0x7c10:	mov    $0xd1,%al
  0x00007c10 in ?? ()
  (gdb) 
  [   0:7c12] => 0x7c12:	out    %al,$0x64
  0x00007c12 in ?? ()
  ```

  关于port 0x64的描述如下：

  ```
  0064	r	KB controller read status (ISA, EISA)
  		 bit 7 = 1 parity error on transmission from keyboard
  		 bit 6 = 1 receive timeout
  		 bit 5 = 1 transmit timeout
  		 bit 4 = 0 keyboard inhibit
  		 bit 3 = 1 data in input register is command
  			 0 data in input register is data
  		 bit 2	 system flag status: 0=power up or reset  1=selftest OK
  		 bit 1 = 1 input buffer full (input 60/64 has data for 8042)
  		 bit 0 = 1 output buffer full (output 60 has data for system)
  		 ...
  0064	w	KB controller input buffer (ISA, EISA)
  		  D1	dbl   write output port. next byte written  to 0060
  			      will be written to the 804x output port; the
  			      original IBM AT and many compatibles use bit 1 of
  			      the output port to control the A20 gate.
  ```

  可见，0x64端口的bit 1指示其输入缓冲区是否为满，程序通过检测该位可得知端口是否busy，然后向端口写入0xd1，这是一个command，即命令，0xd1命令的功能是，下一个写入0x60端口的byte会被写入到804x输出端口，并且其第一个bit的值用来指示A20门。因此seta.20.1的功能是给0x64端口写入一个命令。

  继续单步调试seta20.2：

  ```assembly
  [   0:7c14] => 0x7c14:	in     $0x64,%al            // wait no busy
  0x00007c14 in ?? ()
  (gdb) si
  [   0:7c16] => 0x7c16:	test   $0x2,%al
  0x00007c16 in ?? ()
  (gdb) 
  [   0:7c18] => 0x7c18:	jne    0x7c14
  0x00007c18 in ?? ()
  (gdb) 
  [   0:7c1a] => 0x7c1a:	mov    $0xdf,%al             // 写入0xdf到port 0x60
  0x00007c1a in ?? ()
  (gdb) 
  [   0:7c1c] => 0x7c1c:	out    %al,$0x60
  0x00007c1c in ?? ()
  ```

  port 0x60的描述如下：

  ```
  0060-006F ----	Keyboard controller 804x (8041, 8042)  (or PPI (8255) on PC,XT)
  		 XT uses 60-63,	 AT uses 60-64
  
  		 AT keyboard controller input port bit definitions
  		  bit 7	  = 0  keyboard inhibited
  		  bit 6	  = 0  CGA, else MDA
  		  bit 5	  = 0  manufacturing jumper installed
  		  bit 4	  = 0  system RAM 512K, else 640K
  		  bit 3-0      reserved
  
  		 AT keyboard controller input port bit definitions by Compaq
  		  bit 7	  = 0  security lock is locked
  		  bit 6	  = 0  Compaq dual-scan display, 1=non-Compaq display
  		  bit 5	  = 0  system board dip switch 5 is ON
  		  bit 4	  = 0  auto speed selected, 1=high speed selected
  		  bit 3	  = 0  slow (4MHz), 1 = fast (8MHz)
  		  bit 2	  = 0  80287 installed, 1= no NDP installed
  		  bit 1-0      reserved
  
  		 AT keyboard controller output port bit definitions
  		  bit 7 =    keyboard data output
  		  bit 6 =    keyboard clock output
  		  bit 5 = 0  input buffer full
  		  bit 4 = 0  output buffer empty
  		  bit 3 =    reserved (see note)
  		  bit 2 =    reserved (see note)
  		  bit 1 =    gate A20
  		  bit 0 =    system reset
  		Note:	bits 2 and 3 are the turbo speed switch or password
  			  lock on Award/AMI/Phoenix BIOSes.  These bits make
  			  use of nonstandard keyboard controller BIOS
  			  functionality to manipulate
  			    pin 23 (8041 port 22) as turbo switch for AWARD
  			    pin 35 (8041 port 15) as turbo switch/pw lock for
  				Phoenix
  ```

  可见，AT键盘控制器输出端口的bit1确实是指示gate A20。

  Note：**此处开启A20 gate是通过8042键盘控制器，而非BIOS中对port 0x92的操作（Fast A20），暂不清楚原因**。

- step 3：从实模式切换到保护模式。

  ```assembly
  (gdb) 
  [   0:7c1e] => 0x7c1e:	lgdtw  0x7c64                                    // 设置gdt
  0x00007c1e in ?? ()
  (gdb) si
  [   0:7c23] => 0x7c23:	mov    %cr0,%eax                                 // 置位$cr0中的PE，开启保护模式
  0x00007c23 in ?? ()
  (gdb) 
  [   0:7c26] => 0x7c26:	or     $0x1,%eax
  0x00007c26 in ?? ()
  (gdb) 
  [   0:7c2a] => 0x7c2a:	mov    %eax,%cr0
  0x00007c2a in ?? ()
  (gdb) 
  [   0:7c2d] => 0x7c2d:	ljmp   $0x8,$0x7c32                              // 设置$cs的段选择子，以及$ip
  0x00007c2d in ?? ()
  (gdb)                                                                    // 进入实模式
  The target architecture is set to "i386".
  ```

  此处和BIOS中的部分指令类似，完成模式切换的工作。

  对比boot.S中的汇编代码，可以看到gdt的内容被直接写在了文件中，包括gdt的内容和lgdt需要的参数的值：

  ```assembly
  
  # Bootstrap GDT
  .p2align 2                                # force 4 byte alignment
  gdt:
    SEG_NULL				# null seg
    SEG(STA_X|STA_R, 0x0, 0xffffffff)	# code seg
    SEG(STA_W, 0x0, 0xffffffff)	        # data seg
  
  gdtdesc:
    .word   0x17                            # sizeof(gdt) - 1
    .long   gdt                             # address gdt
  ```

  在inc/mmu.h中包含了SEG宏定义。可见，我们设置代码段可读可执行，而数据段可读写，并且段的基址都是0x0，此时段寄存器基本是可有可无了，因为寄存器此时是32-bit，支持访问全部地址空间，现如今的段寄存器应该只是为了向后兼容。

  ```
  #define SEG(type,base,lim)					\
  	.word (((lim) >> 12) & 0xffff), ((base) & 0xffff);	\
  	.byte (((base) >> 16) & 0xff), (0x90 | (type)),		\
  		(0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)
  ```

  **puzzle：为什么模式切换都是在jmp指令后完成，我记得《x86汇编语言：从实模式到保护模式》中有解答**

- step 4：设置保护模式下的段寄存器的值。

  ```assembly
  => 0x7c32:	mov    $0x10,%ax                   // 将$ds、$es、、$fs、$gs、$ss设为0
  0x00007c32 in ?? ()
  (gdb) si
  => 0x7c36:	mov    %eax,%ds
  0x00007c36 in ?? ()
  (gdb) 
  => 0x7c38:	mov    %eax,%es
  0x00007c38 in ?? ()
  (gdb) 
  => 0x7c3a:	mov    %eax,%fs
  0x00007c3a in ?? ()
  (gdb) 
  => 0x7c3c:	mov    %eax,%gs
  0x00007c3c in ?? ()
  (gdb) 
  => 0x7c3e:	mov    %eax,%ss
  0x00007c3e in ?? ()
  (gdb)  
  => 0x7c40:	mov    $0x7c00,%esp               // 设置栈指针的值为0x7c00
  0x00007c40 in ?? ()
  (gdb) 
  => 0x7c45:	call   0x7d19                     // 跳转到此处执行，0x7d19是bootmain的地址
  0x00007c45 in ?? ()
  ```

  因为段寄存器中的第三位存储其他控制信息，而高位是段索引。因此$cs=8意味着其段索引为1，而其他数据段的索引为2，也正对应着step3中gdt的内容。栈指针设为.start的值，即代码起始处，因此栈底刚好在boot loader代码的下方，并往下增长。最后，call指令调用某一处地址的函数，通过查看boot.S源文件可知，此处为bootmain函数地址，即调用bootmain函数，加载内核。

#### boot/main.c

main.c文件负责将kernel读取到内存中的指定区域。首先阅读并理解代码，然后单步调试查看读取到的内核相关的信息。由main.c中的注释可知，kernel image存放在第二个sector开始处。

main.c中共四个函数，分别为bootmain、readseg、waitdisk和readsect，显然他们完成加载内核、读取段、等待磁盘以及读取扇区的功能。从易到难分别解读：

- waitdisk：代码如下，通过读取某个状态端口的数据判断是否当前磁盘“空闲”（应该说是相关的运输总线）。

  ```c
  void
  waitdisk(void)
  {
  	// wait for disk reaady
  	while ((inb(0x1F7) & 0xC0) != 0x40)
  		/* do nothing */;
  }
  ```

  关于port 0x1F7的描述如下：

  ```
  01F7	r	status register
  		 bit 7 = 1  controller is executing a command
  		 bit 6 = 1  drive is ready
  		 bit 5 = 1  write fault
  		 bit 4 = 1  seek complete
  		 bit 3 = 1  sector buffer requires servicing
  		 bit 2 = 1  disk data read successfully corrected
  		 bit 1 = 1  index - set to 1 each disk revolution
  		 bit 0 = 1  previous command ended in an error
  ```

  因此，该函数通过检查状态寄存器的bit 6是否为1且bit 7是否为0，判断磁盘是否空闲。

- readsect：

  ```c
  void
  readsect(void *dst, uint32_t offset)
  {
  	// wait for disk to be ready
  	waitdisk();
  
  	outb(0x1F2, 1);		// count = 1
  	outb(0x1F3, offset);
  	outb(0x1F4, offset >> 8);
  	outb(0x1F5, offset >> 16);
  	outb(0x1F6, (offset >> 24) | 0xE0);
  	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors
  
  	// wait for disk to be ready
  	waitdisk();
  
  	// read a sector
  	insl(0x1F0, dst, SECTSIZE/4);
  }
  ```

  找到相关的端口的描述内容：

  ```
  01F0	r/w	data register
  01F2	r/w	sector count
  01F3	r/w	sector number
  01F4	r/w	cylinder low
  01F5	r/w	cylinder high
  
  01F6	r/w	drive/head
  		 bit 7	 = 1
  		 bit 6	 = 0
  		 bit 5	 = 1
  		 bit 4	 = 0  drive 0 select
  			 = 1  drive 1 select
  		 bit 3-0      head select bits
  
  01F7	r	status register
  		 bit 7 = 1  controller is executing a command
  		 bit 6 = 1  drive is ready
  		 bit 5 = 1  write fault
  		 bit 4 = 1  seek complete
  		 bit 3 = 1  sector buffer requires servicing
  		 bit 2 = 1  disk data read successfully corrected
  		 bit 1 = 1  index - set to 1 each disk revolution
  		 bit 0 = 1  previous command ended in an error
  01F7	w	command register
  		commands:
  		...
  		20	read sectors with retry
  		...
  ```

  首先，等待磁盘空闲，准备进行“地址”信息的写入，每次读取1个扇区，因此设置port 0x1F2的值为1，offset的值为32位宽，由低到高的每个字节分别为扇区号、磁盘柱面低位字节、磁盘柱面高位字节，最后一个字节的低四位标识磁盘头的选择bit，第4位标识驱动的选择，通过与0xE0或操作，将高三位bit置为1（暂不清楚缘由，按照上面描述信息，貌似bit 6可以为0，即或0xA0也可，但是这样的话重编译卡死了）。最后等待磁盘控制器准备好数据，然后从0x1F0数据寄存器中读取数据。

  insl函数接收三个参数：第一个指明I/O端口，第二个指明内存地址，而第三个则指明操作次数。其内部使用内嵌汇编完成。

  ```c
  static inline void
  insl(int port, void *addr, int cnt)
  {
  	asm volatile("cld\n\trepne\n\tinsl"
  		     : "=D" (addr), "=c" (cnt)
  		     : "d" (port), "0" (addr), "1" (cnt)
  		     : "memory", "cc");
  }
  ```

  通过cld指令设置$di递增，repne递减$ecx的值并调用insl直到$ecx为0。

- readseg：接收三个参数pa，count和offset。从内核读取偏移量为offset处的count字节到物理地址pa处（注意不是从磁盘偏移为offset的地方开始读）。因为kernel从磁盘的1扇区开始，所以读取的时候需要将指定磁盘的扇区号设为offset / SECTSIZE + 1。具体详见代码。

- bootmain：紧接着boot.S的程序执行完调用bootmain，继续单步调试。在加载内核之前，bootmain会读取一些内核相关的控制信息，可在gdb中查看。同时对比反汇编文件obj/boot/boot.asm，该文件中还包括汇编指令对应的c语句，可以帮助我们理解汇编程序。

  - 根据bootmain的c程序可知，首先需要读取内核elf文件头部的信息（也处于kernel image的最前端），共8个扇区的大小，读取到指定好的地址0x10000。

    ```assembly
     7d19:	55                   	push   %ebp
        7d1a:	89 e5                	mov    %esp,%ebp
        7d1c:	56                   	push   %esi
        7d1d:	53                   	push   %ebx
    	readseg((uint32_t) ELFHDR, SECTSIZE*8, 0);
        7d1e:	52                   	push   %edx
        7d1f:	6a 00                	push   $0x0              // 压栈第三个参数offset = 0
        7d21:	68 00 10 00 00       	push   $0x1000           // 压栈第二个参数count = 512 * 8 = 1<<12
        7d26:	68 00 00 01 00       	push   $0x10000          // 压栈第一个参数pa = 0x10000
        7d2b:	e8 aa ff ff ff       	call   7cda <readseg>
    ```

  - 此时对比头部信息中的magic，判断是否为一个合法的头部结构：

    ```assembly
    if (ELFHDR->e_magic != ELF_MAGIC)
        7d30:	83 c4 10             	add    $0x10,%esp
        7d33:	81 3d 00 00 01 00 7f 	cmpl   $0x464c457f,0x10000   // 对比立即数与内存0x10000处的值
        7d3a:	45 4c 46 
        7d3d:	75 38                	jne    7d77 <bootmain+0x5e>
    ```

    查看内核使用的elf头部结构体，我们可以得知magic存放在头部的开始四个字节，我们将内核的前八个扇区读入0x10000后，magic的值就存放在0x10000：

    ```c
    #define ELF_MAGIC 0x464C457FU	/* "\x7FELF" in little endian */
    
    struct Elf {
    	uint32_t e_magic;	// must equal ELF_MAGIC
    	...
    ```

    通过gdb查看0x10000处的内容也可以验证：

    ```assembly
    => 0x7d33:	cmpl   $0x464c457f,0x10000
    0x00007d33 in ?? ()
    (gdb) x /x 0x10000
    0x10000:	0x464c457f
    ```

    读取程序头部表的偏移和数目，通过程序头部表中的每个条目的信息，将每个“段”加载到内存中。通过gdb单步调试，将每个寄存器的值写在了注释部分，如下：

    ```assembly
    ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
        7d3f:	a1 1c 00 01 00       	mov    0x1001c,%eax      // 读取ELFHDR->e_phoff的值存入$eax，0x34
    	eph = ph + ELFHDR->e_phnum;
        7d44:	0f b7 35 2c 00 01 00 	movzwl 0x1002c,%esi      // 读取ELFHDR->e_phnum的值存入$esi，0x3
    	ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
        7d4b:	8d 98 00 00 01 00    	lea    0x10000(%eax),%ebx // $ebx存储ph的值，即第一个程序头部表条目地址
    	eph = ph + ELFHDR->e_phnum;
        7d51:	c1 e6 05             	shl    $0x5,%esi         // 左移5位$esi，猜测每个条目大小为1<<5
        7d54:	01 de                	add    %ebx,%esi         // $esi存储eph的值，即最后一个条目后地址
    	for (; ph < eph; ph++)                                   
        7d56:	39 f3                	cmp    %esi,%ebx
        7d58:	73 17                	jae    7d71 <bootmain+0x58>
    		readseg(ph->p_pa, ph->p_memsz, ph->p_offset);         // 根据逐个条目的信息，读取相应的扇区内容到
    		                                                      // 条目安排好的物理地址处
        7d5a:	50                   	push   %eax
    ```
  
    读取e_phoff和e_phnum的值时，根据字段在ELF头部结构中的偏移确定其物理地址。偏移加上ELF头的物理地址，即字段的物理地址。查看Elf的结构体定义，计算得到各个字段的偏移（需要考虑align，每个字段需要align其类型大小）。 
  
    ```c
    
    struct Elf {                                    // 字段偏移
    	uint32_t e_magic;	// must equal ELF_MAGIC // 0
    	uint8_t e_elf[12];                          // 4
    	uint16_t e_type;                            // 4 + 12 = 16
    	uint16_t e_machine;                         // 18 
    	uint32_t e_version;                         // 20
    	uint32_t e_entry;                           // 24
    	uint32_t e_phoff;                           // 28 = 0x1c
    	uint32_t e_shoff;                           // 32
    	uint32_t e_flags;                           // 36
    	uint16_t e_ehsize;                          // 40
    	uint16_t e_phentsize;                       // 42
    	uint16_t e_phnum;                           // 44 = 0x2c
    	...
    ```
  
    $ebx中保存当前所需要读取的段对应的程序头部表条目，条目中的控制信息包含调用readseg函数所需的参数信息：
  
    ```assembly
    for (; ph < eph; ph++)
        7d5b:	83 c3 20             	add    $0x20,%ebx            // $ebx此时指向下一个条目
    		readseg(ph->p_pa, ph->p_memsz, ph->p_offset);            // readseg参数压栈
        7d5e:	ff 73 e4             	push   -0x1c(%ebx)           // 获取条目中段的offset
        7d61:	ff 73 f4             	push   -0xc(%ebx)            // 获取条目中的段的count
        7d64:	ff 73 ec             	push   -0x14(%ebx)           // 获取条目中段的指定加载地址pa
        7d67:	e8 6e ff ff ff       	call   7cda <readseg>
    	for (; ph < eph; ph++)
        7d6c:	83 c4 10             	add    $0x10,%esp
        7d6f:	eb e5                	jmp    7d56 <bootmain+0x3d>
    ```
  
    如下是ELF关于程序头部表条目的结构体定义，结构体大小为32字节，这也是为什么每次$ebx的值需要递增0x20（32）以及求$esi时，需要将初始$ebx加上条目数左移5位（乘以32）：
  
    ```c
    struct Proghdr {
    	uint32_t p_type;
    	uint32_t p_offset;        // offset
    	uint32_t p_va;
    	uint32_t p_pa;            // pa
    	uint32_t p_filesz;
    	uint32_t p_memsz;         // count
    	uint32_t p_flags;
    	uint32_t p_align;
    };
    ```
  
    每次读取段前，递增$ebx，因此汇编指令中访问当前条目使用的是$ebx减去“条目大小减去相应字段在结构体中的偏移”的值。
  
  - 跳转到内核elf指示的程序入口点，开始执行内核代码。
  
    ```assembly
    ((void (*)(void)) (ELFHDR->e_entry))();
        7d71:	ff 15 18 00 01 00    	call   *0x10018   // 程序入口点的地址，0x0010000c
        => 0x7d71:	call   *0x10018
    ```
  
    单步执行到此处，gdb过程如下：
  
    ```bash
    0x00007d71 in ?? ()
    (gdb) x /x $0x10018
    Value can't be converted to integer.
    (gdb) x /x 0x10018
    0x10018:	0x0010000c
    (gdb) ni
    => 0x10000c:	movw   $0x1234,0x472
    0x0010000c in ?? ()
    ```

​			  此时boot loader完成所以任务，加载kernel image后，将控制权交给内核代码。

### Question

- At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?

   cr0寄存器中的第0位是保护允许位PE(Protedted Enable)，用于启动保护模式，如果PE位置1，则保护模式启动，如果PE=0，则在实模式下运行。如下指令段设置了cr0的bit0：

  ```bash
  (gdb) 
  [   0:7c2a] => 0x7c2a:	mov    %eax,%cr0
  0x00007c2a in ?? ()
  (gdb) 
  [   0:7c2d] => 0x7c2d:	ljmp   $0x8,$0x7c32   
  ```

  在0x7c2d处的ljmp，开始执行32位代码（也可能在ljmp后的一条）。

- What is the *last* instruction of the boot loader executed, and what is the *first* instruction of the kernel it just loaded?

  0x7d71为boot loader的最后一条指令地址，而内核的第一条指令是movw   $0x1234,0x472，处于0x10000c。

- *Where* is the first instruction of the kernel?

  0x10000c，此处为内核代码入口。

- How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?

  通过读取elf文件的头部信息，获取程序头部表的条目数目以及程序头部表的地址，每个条目中保存了内核的每个段的信息，如加载地址、扇区数等等。我们也可以通过readelf -h查看可执行文件的ELF头部信息：

  ```bash
  readelf -h obj/kern/kernel                    23:36:24
  ELF Header:
    Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00 
    Class:                             ELF32
    Data:                              2's complement, little endian
    Version:                           1 (current)
    OS/ABI:                            UNIX - System V
    ABI Version:                       0
    Type:                              EXEC (Executable file)
    Machine:                           Intel 80386
    Version:                           0x1
    Entry point address:               0x10000c         // 程序入口地址
    Start of program headers:          52 (bytes into file)
    Start of section headers:          87424 (bytes into file)
    Flags:                             0x0
    Size of this header:               52 (bytes)
    Size of program headers:           32 (bytes)        // 程序头部表条目大小 
    Number of program headers:         3                 // 程序头部表条目数目
    Size of section headers:           40 (bytes)
    Number of section headers:         15
    Section header string table index: 14                         
  ```

### Loading the Kernel

在gdb中查看加载的三个段的信息，包括读取的段的物理地址、扇区数目以及内核偏移：

```bash
# 第1个段
(gdb) x /x $ebx-0x1c
0x10038:	0x00001000      // offset
(gdb) x /x $ebx-0xc
0x10048:	0x00007ac9      // count
(gdb) x /x $ebx-0x14
0x10040:	0x00100000      // pa

# 第2个段
(gdb) x /x $ebx-0x1c
0x10058:	0x00009000
(gdb) x /x $ebx-0xc
0x10068:	0x0000b961
(gdb) x /x $ebx-0x14
0x10060:	0x00108000
# 第3个段
0x00007d67 in ?? ()
(gdb) x /x $ebx-0x1c
0x10078:	0x00000000
(gdb) x /x $ebx-0xc
0x10088:	0x00000000
(gdb) x /x $ebx-0x14
0x10080:	0x00000000
```

可见，我们需要读取三个段（严格来说应该是两个段，第三个段大小为0），每个段的链接地址和加载地址以及字节数都保存在段对应的程序头部表条目中。

我们也可以通过objdump命令指定相关的参数，查看每个section的信息（多个section组成一个program segment）。为了区分segment和section，我们称segment为段，而section为节。其中大部分节包含调试信息，不会被加载器加载到内存中。通过与gdb中查看的信息对比，可以发现，第1个段从.text节开始，为内核的代码段，包括了.text、.rodata、.stab、.stabstr（.stabstr节的下一个加载地址为0x0010638d+0x173c，减去.text的加载地址刚好是0x7ac9）；第2个段从.data开始，是内核的数据段，包括了从.data到.bss（（.bss节的下一个加载地址为0x113300+0x661，减去.data节的加载地址刚好是0xb961）。

```bash
objdump -h obj/kern/kernel                    09:34:32

obj/kern/kernel:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00001d21  f0100000  00100000  00001000  2**4    // 第1个段从.text节开始
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .rodata       00000ab8  f0101d40  00101d40  00002d40  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 .stab         00003b95  f01027f8  001027f8  000037f8  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .stabstr      0000173c  f010638d  0010638d  0000738d  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .data         00009300  f0108000  00108000  00009000  2**12   // 第2个段从.data开始
                  CONTENTS, ALLOC, LOAD, DATA
  5 .got          00000008  f0111300  00111300  00012300  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  6 .got.plt      0000000c  f0111308  00111308  00012308  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  7 .data.rel.local 00001000  f0112000  00112000  00013000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  8 .data.rel.ro.local 00000300  f0113000  00113000  00014000  2**5
                  CONTENTS, ALLOC, LOAD, DATA
  9 .bss          00000661  f0113300  00113300  00014300  2**5
                  CONTENTS, ALLOC, LOAD, DATA
 10 .comment      0000002b  00000000  00000000  00014961  2**0
```

通过objdump -x查看“program headers”信息：

```bash
objdump -x obj/kern/kernel                    10:24:29

obj/kern/kernel:     file format elf32-i386
obj/kern/kernel
architecture: i386, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x0010000c

Program Header:
    LOAD off    0x00001000 vaddr 0xf0100000 paddr 0x00100000 align 2**12
         filesz 0x00007ac9 memsz 0x00007ac9 flags r-x
    LOAD off    0x00009000 vaddr 0xf0108000 paddr 0x00108000 align 2**12
         filesz 0x0000b961 memsz 0x0000b961 flags rw-
   STACK off    0x00000000 vaddr 0x00000000 paddr 0x00000000 align 2**4
         filesz 0x00000000 memsz 0x00000000 flags rwx
```

与我们在gdb中查看的信息一致，并且显示第三个段为”Stack“。暂不清楚这个段是怎么来的。

这里解释一下链接地址和加载地址。以我的理解，链接地址是一个编译范畴的概念，是链接器赋予的程序中各个全局变量以及函数的地址，程序编译时，如果引用了其他文件中定义的全局变量和函数，则会空着，留给链接器来填写，也就是链接中的重定位。因此链接地址是程序期望该变量或者函数所在的地址。而加载地址则是应该存放在内存中的实际地址。我们平时写的程序编译链接后，VMA和LMA是相同的，因为此时我们的程序中使用的地址均为虚拟地址，加载地址也是一个虚拟地址，加载器和操作系统会帮助我们在可执行文件中的相关段和段的虚拟加载地址之间建立关联。实际运行程序时，将相关的段的内容读入物理内存，并将虚拟加载地址映射到实际的物理内存地址。

此处我们使用的仍然是物理地址，因此加载地址则指明将该段加载到指定的物理地址。

### Exercise 5

BIOS将boot loader加载到0x7c00，这是boot loader的加载地址。在boot/Makefrag中，通过-Ttext 0x7C00告知，链接boot loader时的链接地址，如果我们修改此值，会影响程序中的跳转指令。因为实际上BIOS还是把boot loader加载到了0x7c00，而boot loader中的跳转指令，仍然以为当前boot loader是处于一个不同于0x7c00的物理地址，因此会跳转到错误的物理地址执行。

- 首先我们验证第一点，即BIOS仍然是把boot loader加载到0x7c00。

  gdb查看0x7c00处的内容：

  ```bash
  x /20x 0x7c00
  0x7c00:	0xc031fcfa	0xc08ed88e	0x64e4d08e	0xfa7502a8
  0x7c10:	0x64e6d1b0	0x02a864e4	0xdfb0fa75	0x010f60e6
  0x7c20:	0x0f7c6416	0x8366c020	0x220f01c8	0x7c32eac0
  0x7c30:	0xb8660008	0xd88e0010	0xe08ec08e	0xd08ee88e
  0x7c40:	0x007c00bc	0x00cfe800	0xfeeb0000	0x00000000
  ```

- 修改-Ttext 0x7C00为-Ttext 0x7D00，执行如下命令重新编译：

  ```bash
  > make clean                                    11:21:12
  rm -rf obj .gdbinit jos.in qemu.log
  > make    
  ```

  gdb调试：

  ```bash
  (gdb) b *0x7c00
  Breakpoint 1 at 0x7c00
  (gdb) c
  Continuing.
  [   0:7c00] => 0x7c00:	cli    
  
  Breakpoint 1, 0x00007c00 in ?? ()
  (gdb) x /20x 0x7c00
  0x7c00:	0xc031fcfa	0xc08ed88e	0x64e4d08e	0xfa7502a8
  0x7c10:	0x64e6d1b0	0x02a864e4	0xdfb0fa75	0x010f60e6
  0x7c20:	0x0f7d6416	0x8366c020	0x220f01c8	0x7d32eac0
  0x7c30:	0xb8660008	0xd88e0010	0xe08ec08e	0xd08ee88e
  0x7c40:	0x007d00bc	0x00cfe800	0xfeeb0000	0x00000000
  (gdb) x /20x 0x7d00
  0x7d00:	0x00c38153	0xe8000002	0xffffff6c	0xeb10c483
  0x7d10:	0xf4658de7	0x5d5f5e5b	0xe58955c3	0x6a525356
  0x7d20:	0x10006800	0x00680000	0xe8000100	0xffffffaa
  0x7d30:	0x8110c483	0x0100003d	0x4c457f00	0xa1387546
  0x7d40:	0x0001001c	0x2c35b70f	0x8d000100	0x01000098
  ```

  可以发现，此时0x7c00的内容和未修改boot/Makefrag之前是相同的，因此BIOS仍然是把boot loader加载到了0x7c00，并且gdb中的设置断点的地址也是加载地址而非链接地址。

  查看编译后反汇编的obj/boot/boot.asm：

  ```assembly
  .globl start
  start:
    .code16                     # Assemble for 16-bit mode
    cli                         # Disable interrupts
      7d00:	fa                   	cli    
    cld                         # String operations increment
      7d01:	fc                   	cld    
      ...
  ```

  显示的地址为链接地址，从0x7d00开始。对于普通的指令，如简单的寄存器操作等，不会有影响，因为这些指令不依赖内存的状态。而对于跳转指令，其使用的地址是链接器安置好的链接地址，因此会跳转到错误的地址执行。

  ```bash
  (gdb) c
  Continuing.
  
  Program received signal SIGTRAP, Trace/breakpoint trap.
  [   0:7c2d] => 0x7c2d:	ljmp   $0x8,$0x7d32   // 指令跳转到错误的地址
  ```

  最后，恢复boot/Makefrag的内容并重新编译。

### Exercise 6

查看内核的程序入口点，即第一条执行的内核程序的地址：

```bash
objdump -f obj/kern/kernel                    11:30:47

obj/kern/kernel:     file format elf32-i386
architecture: i386, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x0010000c
```

gdb单步调试程序，在boot loader的地址和内核的程序入口点地址设置断点，分别在进入boot loader后和进入内核后查看0x100000处的内容，此处是内核的代码段起始处：

```bash
(gdb) b *0x7c00
Breakpoint 1 at 0x7c00
(gdb) b *0x10000c
Breakpoint 2 at 0x10000c
(gdb) c
Continuing.
[   0:7c00] => 0x7c00:	cli    

Breakpoint 1, 0x00007c00 in ?? ()
(gdb) x /8x 0x100000
0x100000:	0x00000000	0x00000000	0x00000000	0x00000000
0x100010:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) c
Continuing.
The target architecture is set to "i386".
=> 0x10000c:	movw   $0x1234,0x472

Breakpoint 2, 0x0010000c in ?? ()
(gdb) x /8x 0x100000
0x100000:	0x1badb002	0x00000000	0xe4524ffe	0x7205c766
0x100010:	0x34000004	0x2000b812	0x220f0011	0xc0200fd8
(gdb) x /x $pc
0x10000c:	0x7205c766
(gdb) x /i $pc                              // 第二个断点处是内核执行的第一条指令
=> 0x10000c:	movw   $0x1234,0x472
```

## Kernel

类似boot loader，内核也需要执行一部分汇编代码，设置相关的环境，以让后续的C程序可以执行，在我们的程序中，主要是开启虚拟内存机制，这部分工作由kern/entry.S完成。

### Virtual memory

查看kern/entry.S汇编程序，可以发现，上面显示的内核入口点0x10000c处的指令就是entry.S的第一条指令，而内核代码段从0x100000处开始，entry.S起始处定义了三个long，分别是MULTIBOOT_HEADER_MAGIC、MULTIBOOT_HEADER_FLAGS和CHECKSUM，这三个常量的数值和0x100000处的内容是一致的：

```

# The Multiboot header
.align 4
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long CHECKSUM

# '_start' specifies the ELF entry point.  Since we haven't set up
# virtual memory when the bootloader enters this code, we need the
# bootloader to jump to the *physical* address of the entry point.
.globl		_start
_start = RELOC(entry)

.globl entry
entry:
	movw	$0x1234,0x472			# warm boot
```

在entry.S中的开始时，指令使用的地址还是物理地址，而内核的链接地址和物理地址并不一致，因此如果需要访问内存，必须通过符号的链接地址得到其实际的物理地址（即加载地址）。此处的RELOC宏完成了该转换工作：

```bash
#define	RELOC(x) ((x) - KERNBASE)   // KERNBASE为0xF0000000，定义在inc/memlayout.h
```

获取页表的物理地址并将其加载到页表寄存器$cr3中，并开启$cr0中的PG位：

```assembly
# Load the physical address of entry_pgdir into cr3.  entry_pgdir
	# is defined in entrypgdir.c.
	movl	$(RELOC(entry_pgdir)), %eax
	movl	%eax, %cr3
	# Turn on paging.
	movl	%cr0, %eax
	orl	$(CR0_PE|CR0_PG|CR0_WP), %eax
	movl	%eax, %cr0
```

此时PC的分页机制已经开启，我们使用的地址将是虚拟地址，即内核的链接地址，此处直接jmp到jmp指令的下一条指令，完成对pc寄存器的重写，此后使用的指令地址均为虚拟地址：

```assembly
mov	$relocated, %eax
	jmp	*%eax
relocated:
```

设置$ebp和$esp的值，等同于初始化栈基寄存器和栈指针寄存器，$ebp的值为0可以帮助我们在后面的backtrace程序中知道什么时候不需要再回退栈帧（因为已经是最早的栈帧了），栈的顶部位于：

```assembly
# Clear the frame pointer register (EBP)
	# so that once we get into debugging C code,
	# stack backtraces will be terminated properly.
	movl	$0x0,%ebp			# nuke frame pointer

	# Set the stack pointer
	movl	$(bootstacktop),%esp

	# now to C code
	call	i386_init
	...
.data
###################################################################
# boot stack
###################################################################
	.p2align	PGSHIFT		# force page alignment  定义在inc/mmu.h 为12
	.globl		bootstack
bootstack:
	.space		KSTKSIZE    # 定义在inc/mmu.h 为8 * PGSIZE，PGSIZE为1<<12
	.globl		bootstacktop   
bootstacktop:
```

bootstack需要从entry.S的.data节（位于0xf0108000）后的与页对齐地址开始，即0xf0108000开始，大小为8页，因此栈顶地址为0xf0108000+0x8000 = 0xf0110000。

gdb查看栈顶地址：

```bash
=> 0xf0100034 <relocated+5>:	mov    $0xf0110000,%esp
relocated () at kern/entry.S:77
77		movl	$(bootstacktop),%esp
(gdb) si
=> 0xf0100039 <relocated+10>:	call   0xf01000a6 <i386_init>
80		call	i386_init
(gdb) p $esp
$1 = (void *) 0xf0110000 <entry_pgtable>
(gdb) p /x $esp
$2 = 0xf0110000
(gdb) p /x $esp - 0x8000
$3 = 0xf0108000
```

使用readelf查看链接时各符号（全局变量、静态变量、全局函数）的链接地址：

```bash
> readelf -s obj/kern/kernel | grep boot        23:24:29
   106: f0110000     0 NOTYPE  GLOBAL DEFAULT    5 bootstacktop
   108: f0108000     0 NOTYPE  GLOBAL DEFAULT    5 bootstack
```

与推测的一致。最后，调用i386_init，开始执行c程序。

### Exercise 7

kern/kernel.ld脚本指定了内核的链接地址和加载地址，告知链接器内核的链接地址从0xF0100000开始，并且指示.text段的加载地址为0x100000，以告知boot loader将.text加载到此处。

```
SECTIONS
{
	/* Link the kernel at this address: "." means the current address */
	. = 0xF0100000;

	/* AT(...) gives the load address of this section, which tells
	   the boot loader where to load the kernel in physical memory */
	.text : AT(0x100000) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}
```

entry.S中使用的页表为手工设定的、静态的页表。定义在 `kern/entrypgdir.c`中。

```c
__attribute__((__aligned__(PGSIZE)))
pde_t entry_pgdir[NPDENTRIES/* 1024 */] = {
	// Map VA's [0, 4MB) to PA's [0, 4MB)
	[0]
		= ((uintptr_t)entry_pgtable - KERNBASE) + PTE_P,
	// Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
	[KERNBASE>>PDXSHIFT/* 22 */]
		= ((uintptr_t)entry_pgtable - KERNBASE) + PTE_P + PTE_W
};

// Entry 0 of the page table maps to physical page 0, entry 1 to
// physical page 1, etc.
__attribute__((__aligned__(PGSIZE)))
pte_t entry_pgtable[NPTENTRIES] = {
	0x000000 | PTE_P | PTE_W,
	0x001000 | PTE_P | PTE_W,
	0x002000 | PTE_P | PTE_W,
	...
	0x3fe000 | PTE_P | PTE_W,
	0x3ff000 | PTE_P | PTE_W,
};
```

通过文件中的注释可知，页表将*[KERNBASE, KERNBASE+4MB)*和*[0, 4MB)* 两段虚拟地址区域映射到物理地址区域*[0, 4MB)*。之所以选择4MB，是因为刚好一个页表可以映射4MB的地址空间（页表占一页，每个页表项为32-bit，因此一个页表可以容纳1024个页表项，完成1024*4096个字节的地址映射）。 页表项的低12bit存储一些控制信息，比如该页是否可读可写、是否映射到物理地址空间等。

gdb单步调试到设置页表寄存器处，查看前后 0x00100000与0xf0100000处内存：

```bash
(gdb) b *0x100025
Breakpoint 1 at 0x100025
(gdb) c
Continuing.
The target architecture is set to "i386".
=> 0x100025:	mov    %eax,%cr0                                               # 开启分页前，此时使用的是物理地址

Breakpoint 1, 0x00100025 in ?? ()
(gdb) x /8x 0x100000 
0x100000:	0x1badb002	0x00000000	0xe4524ffe	0x7205c766
0x100010:	0x34000004	0x2000b812	0x220f0011	0xc0200fd8
(gdb) x /8x 0xf0100000 
0xf0100000 <_start-268435468>:	0x00000000	0x00000000	0x000000000x00000000
0xf0100010 <entry+4>:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) si                                                                      # 开启分页后，此时使用的是虚拟地址
=> 0x100028:	mov    $0xf010002f,%eax                                       
0x00100028 in ?? ()
(gdb) x /8x 0x100000 
0x100000:	0x1badb002	0x00000000	0xe4524ffe	0x7205c766
0x100010:	0x34000004	0x2000b812	0x220f0011	0xc0200fd8
(gdb) x /8x 0xf0100000                                                       # 与0x100000处内容相同
0xf0100000 <_start-268435468>:	0x1badb002	0x00000000	0xe4524ffe0x7205c766
0xf0100010 <entry+4>:	0x34000004	0x2000b812	0x220f0011	0xc0200fd8
```

如果我们注释到开启分页的指令，第一条出错的指令会是`jmp	*%eax`后的指令，即`movl	$0x0,%ebp`，因为jmp指令实际会跳转到物理内存高地址处执行（与开启分页前相同，全是0），执行的指令是非法的。

### Print and Console

### Exercise 8

参考16进制的输出实现，设置base=8即可：

```c
case 'o':
			// Replace this with your code.
			// putch('X', putdat);
			// putch('X', putdat);
			// putch('X', putdat);
			// break;
            num = (unsigned long long)
				(uintptr_t) va_arg(ap, void *);
			base = 8;
			goto number;
```

- question 1：

- question 2：试解释如下代码片段的含义

  ```c
  #define CRT_ROWS	25
  #define CRT_COLS	80
  #define CRT_SIZE	(CRT_ROWS * CRT_COLS)
  ...
  
  // What is the purpose of this?
  if (crt_pos >= CRT_SIZE)
  {
      int i;
  
      // 将除第一行外的内容往上移动一行
      memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t)); 
      // 清空最后一行
      for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
          crt_buf[i] = 0x0700 | ' ';
      crt_pos -= CRT_COLS; // 光标移动到最后一行的行首
  }
  ```

  CRT_SIZE的宏定义为显示屏幕的长宽乘积，即屏幕大小，而crt_pos变量应该是指示当前光标的位置。如果光标已经到了屏幕所能显示的最后的一个位置之后，结合现实生活中使用编辑器、查看文档的经验，此时需要删除第一行，将后面的行往上移，，清空最后一行，并且将当前光标的位置调整到最后一行的起始处。这段程序中的完成对应功能的语句如注释标注所示。

- question 3：单步调试如下程序，并回答相应问题（为了便于调试，将如下代码段以及后续几个question中涉及的代码段都贴入kern/monitor.c中，重新编译）：

  ```c
  int x = 1, y = 3, z = 4;
  cprintf("x %d, y %x, z %d\n", x, y, z);
  ```

  - 在对cprintf（）的调用中，fmt指向什么？ap指的是什么？

    fmt是指向常量字符串"x %d, y %x, z %d\n"的指针，

  - 列出（按执行顺序）对cons_putc、va_arg和vcprintf的每个调用。对于cons_putc，也列出其参数。对于va_arg，请列出调用前后ap指向的内容。对于vcprintf，列出其两个参数的值。

### Stack

### Exercise 9

根据**Virtual Memory**节的分析，可知栈指针的设置由entry.S中的汇编代码设定，地址为f0110000，大小为8页。预留方式为通过在.data段中的.space伪指令申请指定大小的空间。我们可以通过readelf指令查看etext和edata的数值，验证栈（0xf0108000 ~ 0xf0110000）是否处于.data段中：

```shell
> readelf -s obj/kern/kernel | grep etext       23:30:59
    91: f0101d21     0 NOTYPE  GLOBAL DEFAULT    1 etext
> readelf -s obj/kern/kernel | grep edata       23:31:07
    75: f0113300     0 NOTYPE  GLOBAL DEFAULT   10 edata
```

### Exercise 10

单步调试，直接在test_backtrace函数设置断点即可，不必找到该函数的地址然后设断点。

```bash
(gdb) p /x $esp
$1 = 0xf010ffbc
(gdb) c
Continuing.
=> 0xf0100040 <test_backtrace>:	push   %ebp

Breakpoint 1, test_backtrace (x=3) at kern/init.c:13
13	{
(gdb) p /x $esp
$2 = 0xf010ff9c
(gdb) c
Continuing.
=> 0xf0100040 <test_backtrace>:	push   %ebp

Breakpoint 1, test_backtrace (x=2) at kern/init.c:13
13	{
(gdb) p /x $esp
$3 = 0xf010ff7c
(gdb) c
Continuing.
=> 0xf0100040 <test_backtrace>:	push   %ebp

Breakpoint 1, test_backtrace (x=1) at kern/init.c:13
13	{
(gdb) p /x $esp
$4 = 0xf010ff5c
```

可见每次进入函数，栈都下降了0x20个字节，查看test_backtrace函数反汇编的程序：

```assembly
void
test_backtrace(int x)
{
f0100040:	55                   	push   %ebp                              # 压栈上一个栈帧的栈基址寄存器 4字节
f0100041:	89 e5                	mov    %esp,%ebp
f0100043:	56                   	push   %esi                              # 压栈$esi $ebx的值，即保存调用函数中这两个寄存器的值
f0100044:	53                   	push   %ebx
f0100045:	e8 72 01 00 00       	call   f01001bc <__x86.get_pc_thunk.bx>
f010004a:	81 c3 be 12 01 00    	add    $0x112be,%ebx
f0100050:	8b 75 08             	mov    0x8(%ebp),%esi
	cprintf("entering test_backtrace %d\n", x);
f0100053:	83 ec 08             	sub    $0x8,%esp
f0100056:	56                   	push   %esi
f0100057:	8d 83 38 0a ff ff    	lea    -0xf5c8(%ebx),%eax
f010005d:	50                   	push   %eax
f010005e:	e8 b8 0a 00 00       	call   f0100b1b <cprintf>
	if (x > 0)
f0100063:	83 c4 10             	add    $0x10,%esp
f0100066:	85 f6                	test   %esi,%esi
f0100068:	7e 29                	jle    f0100093 <test_backtrace+0x53>
		test_backtrace(x-1);
f010006a:	83 ec 0c             	sub    $0xc,%esp                      # 栈指针下移0xc字节
f010006d:	8d 46 ff             	lea    -0x1(%esi),%eax
f0100070:	50                   	push   %eax                           # 压栈参数 4字节
f0100071:	e8 ca ff ff ff       	call   f0100040 <test_backtrace>      # 调用函数，自动压栈返回地址，4字节
f0100076:	83 c4 10             	add    $0x10,%esp
```

猜测相隔的0x20字节，即8个32-bit字中，从低地址到高地址依次为返回地址4字节、参数4字节、空闲区12字节、调用函数保存的$ebx、$esi、$ebp的值共12字节。

gdb单步调试：

```bash
b test_backtrace
Breakpoint 1 at 0xf0100040: file kern/init.c, line 13.
(gdb) c
Continuing.
The target architecture is set to "i386".
=> 0xf0100040 <test_backtrace>:	push   %ebp

Breakpoint 1, test_backtrace (x=5) at kern/init.c:13
13	{
(gdb) p /x $esp
$1 = 0xf010ffdc
(gdb) ni
=> 0xf0100041 <test_backtrace+1>:	mov    %esp,%ebp
0xf0100041	13	{
(gdb) p /x $esp
$2 = 0xf010ffd8
(gdb) b *f010006a
No symbol "f010006a" in current context.
(gdb) c
Continuing.
=> 0xf0100040 <test_backtrace>:	push   %ebp

Breakpoint 1, test_backtrace (x=4) at kern/init.c:13
13	{
(gdb) p /x $esp
$3 = 0xf010ffbc
```

