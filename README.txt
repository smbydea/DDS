DDDS - STM32F429IGT6 双通道 AD9833 DDS 可移植模块
==================================================

一、这个文件夹有什么
--------------------

本文件夹把已完成项目中的 DDS 部分单独打包，目标是复制到另一台电脑或另一个
STM32CubeMX/HAL 工程后，可以快速恢复双通道 AD9833 输出。

目录结构：

DDDS/
  Inc/DDS.h              对外公开接口，应用层只需要包含此文件
  Inc/DDS_GPIO.h         FSYNC 引脚内部接口
  Inc/DDS_SPI.h          SPI 内部接口
  Src/DDS.c              AD9833 控制、频率、相位和波形逻辑
  Src/DDS_GPIO.c         两路 FSYNC GPIO 初始化与控制
  Src/DDS_SPI.c          共用 SPI 及 SCK/MOSI 初始化
  examples/main_usage.c.txt  main.c 调用示例，不会被编译器自动当作源码
  README.txt             移植、接线、调用和排错说明
  AGENTS.md              后续电赛项目的协作备忘

模块依赖 STM32F4 HAL，但不包含 HAL 驱动、启动文件、链接脚本和系统时钟代码。
这些内容应由目标 STM32CubeMX 工程生成。

为避免与 CubeMX 工程中常见的 gpio.c 和 spi.c 重名，打包版已将内部文件命名为
DDS_GPIO.c 和 DDS_SPI.c。原项目不受影响。


二、默认硬件和接线
------------------

当前代码支持两片 AD9833 共用 SCLK、SDATA，各自使用独立 FSYNC：

STM32F429IGT6 PA5  -> 两片 AD9833 的 SCLK
STM32F429IGT6 PA7  -> 两片 AD9833 的 SDATA
STM32F429IGT6 PA4  -> AD9833 通道 1 的 FSYNC/CS
STM32F429IGT6 PA3  -> AD9833 通道 2 的 FSYNC/CS
STM32 GND          -> 两片 AD9833 的 GND
AD9833 VCC         -> 按模块标称电压供电

两片 AD9833 与 STM32 必须共地。MISO 不使用。

默认假定 AD9833 模块的 MCLK 为 25 MHz。若模块晶振不是 25 MHz，必须修改
Src/DDS.c 顶部的 DDS_AD9833_MCLK_HZ，否则输出频率会按比例偏差。


三、复制到另一台电脑或另一个工程
--------------------------------

1. 将整个 DDDS 文件夹复制到目标电脑，保留 README 和 AGENTS 作为说明。
2. 把 DDDS/Inc 中的 3 个 .h 文件复制到目标工程的 Core/Inc。
3. 把 DDDS/Src 中的 3 个 .c 文件复制到目标工程的 Core/Src。
4. 在 IDE 或构建系统中确认 DDS.c、DDS_GPIO.c、DDS_SPI.c 已加入编译。
5. 确认目标工程启用了 HAL_SPI_MODULE_ENABLED 和 HAL_GPIO_MODULE_ENABLED。
6. 在 main.c 的 USER CODE BEGIN Includes 区域加入：

   #include "DDS.h"

7. 在 HAL_Init()、SystemClock_Config() 之后调用 DDS_Init()。
8. 编译下载，再用示波器检查两路 AD9833 的 VOUT。

CubeIDE 工程通常会自动编译 Core/Src 中的新 .c 文件。CMake 工程若没有自动收集
源码，需要在 target_sources 或工程源文件列表中加入：

  Core/Src/DDS.c
  Core/Src/DDS_GPIO.c
  Core/Src/DDS_SPI.c

并确保 Core/Inc 已在 include 路径中。


四、CubeMX 中要做的操作
----------------------

打包版会在 DDS_Init() 内自行初始化 SPI1、PA5、PA7、PA4、PA3，因此最省事的
方式是让这些引脚不被其他外设占用，并由本模块接管。检查以下事项：

1. 新建或打开 STM32F429IGT6 工程。
2. 在 Pinout 中确认 PA3、PA4、PA5、PA7 没有被其他功能占用。
3. 若你希望在 CubeMX 图上明确标记，可将 PA3、PA4 配成 GPIO_Output，将 PA5
   配成 SPI1_SCK、PA7 配成 SPI1_MOSI；但不要在应用中用另一套配置反复改写 SPI1。
4. Project Manager -> Code Generator 中保留用户代码区域，重新生成时把应用调用
   放在 USER CODE BEGIN/END 之间。
5. 若 SPI1 已被其他设备使用，先判断 SPI 参数能否共用；不能共用时，应将 DDS
   改到另一个 SPI 外设，或在访问不同设备前重新配置总线。

本模块当前 SPI 参数：主机、8 bit、MSB first、软件 NSS、CPOL High、第一边沿、
分频 16。两个 AD9833 共用 SPI 时，只通过各自 FSYNC 区分通道。


五、修改引脚、SPI 和 MCLK
-------------------------

更换两路 FSYNC：修改 Src/DDS_GPIO.c 顶部宏：

  DDS_FSYNC1_GPIO_PORT / DDS_FSYNC1_GPIO_PIN
  DDS_FSYNC2_GPIO_PORT / DDS_FSYNC2_GPIO_PIN
  对应的 GPIO 时钟使能宏

更换 SPI：修改 Src/DDS_SPI.c 顶部宏：

  DDS_SPI_INSTANCE
  DDS_SPI_CLK_ENABLE()
  DDS_SPI_GPIO_CLK_ENABLE()
  DDS_SPI_GPIO_PORT
  DDS_SPI_SCK_PIN
  DDS_SPI_MOSI_PIN
  DDS_SPI_GPIO_AF

当前 DDS_SPI.c 假定 SCK 和 MOSI 位于同一个 GPIO 端口。若换到不同端口，需要把
GPIO 初始化拆成两组，并分别打开对应 GPIO 时钟。

更换 AD9833 主时钟：修改 Src/DDS.c：

  #define DDS_AD9833_MCLK_HZ 25000000UL


六、main.c 中的最小用法
----------------------

初始化两路 DDS：

  if (DDS_Init() != HAL_OK)
  {
      Error_Handler();
  }

通道 1 输出 1 kHz 正弦波：

  DDS_SetWaveChannel(DDS_CHANNEL_1, DDS_WAVE_SINE, 1000.0, 0.0);

通道 2 输出 5 kHz 三角波，相位 90 度：

  DDS_SetWaveChannel(DDS_CHANNEL_2, DDS_WAVE_TRIANGLE, 5000.0, 90.0);

快捷写法：

  DDS_SineChannel(DDS_CHANNEL_1, 1000.0, 0.0);
  DDS_TriangleChannel(DDS_CHANNEL_2, 5000.0, 90.0);

停止输出：

  DDS_StopChannel(DDS_CHANNEL_1);
  DDS_StopChannel(DDS_CHANNEL_2);

所有关键调用都应检查 HAL_StatusTypeDef 返回值。旧的无通道参数接口仍保留，
但只控制 DDS_CHANNEL_1。


七、接口功能摘要
----------------

DDS_Init()：初始化共用 SPI、FSYNC GPIO 和两个 AD9833 通道。
DDS_SetWaveChannel()：一次设置指定通道的波形、频率、相位并启动。
DDS_SineChannel()：指定通道输出正弦波。
DDS_TriangleChannel()：指定通道输出三角波。
DDS_StopChannel()：指定通道进入 reset，停止当前输出。
DDS_SetFrequencyChannel()：写指定通道的 FREQ0/FREQ1。
DDS_SetPhaseChannel()：写指定通道的 PHASE0/PHASE1。
DDS_SelectFrequencyRegisterChannel()：选择当前频率寄存器。
DDS_SelectPhaseRegisterChannel()：选择当前相位寄存器。
DDS_SleepChannel()：控制指定通道休眠。

有效频率范围由 AD9833 MCLK 限制，代码拒绝高于 MCLK/2 的频率。实际可用波形
质量还受到模块低通滤波器、供电、布线和示波器带宽影响。


八、比赛现场排错顺序
--------------------

1. 检查供电和共地。
2. 检查 DDS_Init() 及输出函数返回值是否为 HAL_OK。
3. 检查代码宏与实际 PA3、PA4、PA5、PA7 接线是否一致。
4. 用示波器看 FSYNC：写通道 1 时 PA4 应有低脉冲，写通道 2 时 PA3 应有低脉冲。
5. 看 PA5 是否有 SCLK，PA7 是否有串行数据。
6. 若 SPI 有数据但无输出，核对 CPOL/CPHA、FSYNC 时序和 AD9833 供电。
7. 若频率按固定比例偏差，核对 DDS_AD9833_MCLK_HZ 与模块实际晶振。
8. 若只有一路工作，交换两路 FSYNC 或模块，区分软件、接线和硬件故障。
9. 若输出波形幅度或形状异常，检查 VOUT 后级滤波、负载阻抗及探头衰减档位。


九、已知边界
------------

- 当前只支持正弦波和三角波，没有开放方波接口。
- 默认固定为两路 AD9833；增加通道要同步扩展通道枚举和 FSYNC 表。
- 模块使用阻塞式 HAL_SPI_Transmit，适合配置 DDS，不适合在高频中断内反复调用。
- DDS_SetWaveChannel 会连续写多次 SPI，不应在 ADC/DMA 等高优先级中断回调中调用。
- 本模块面向 STM32F4 HAL；移植到其他 STM32 系列时需替换 HAL 入口头文件并核对 AF。

