# AGENTS.md - OutputCMake 项目协作说明

## 你的身份与目标

用户是一名正在备赛全国大学生电子设计竞赛的学生，已经有一定单片机基础。后续在本工程中协作时，要把回答和代码都面向“比赛现场能快速复用、能看懂、能改动、能排故”的目标来写。

写代码时不要只给能跑的最短代码，而要兼顾：

- 结构清晰，便于比赛时快速定位问题。
- 注释充分，解释关键寄存器、外设配置、时序、参数含义和易错点。
- 模块化，尽量把外设驱动、应用逻辑、参数配置拆开。
- 可直接调用，常用功能要封装成简单函数，例如“一行函数输出指定频率和波形”。
- 保留 CubeMX 生成代码的 `USER CODE BEGIN/END` 区域规则，避免重新生成工程后丢失用户代码。

## 当前工程概览

本目录是一个 STM32CubeMX/CMake 生成的 STM32F429 工程，工程名为 `OutputCMake`。

主要特点：

- MCU：`STM32F429xx`。
- 框架：STM32 HAL。
- 构建系统：CMake + Ninja。
- 工具链：`gcc-arm-none-eabi`，由 STM32CubeIDE 安装目录提供。
- 当前使用外设：`SPI1`、`GPIOA`。
- 当前重点模块：AD9833 DDS 信号发生器驱动。

## 关键文件

- `Core/Src/main.c`：主程序入口，包含系统初始化、GPIO 初始化、SPI1 初始化；当前已通过 `Output_Init()` 和 `Output_SetWave()` 调用封装后的输出模块。
- `Core/Inc/main.h`：主程序头文件，定义了 `AD9833_CS_Pin` 和 `AD9833_CS_GPIO_Port`。
- `Core/Src/ad9833.c`：AD9833 底层驱动实现，负责 SPI 写控制字、频率字、相位字；保持为通用驱动，不直接绑定具体板子。
- `Core/Inc/ad9833.h`：AD9833 底层驱动公开接口和类型定义。
- `Core/Src/stm32f4xx_hal_msp.c`：HAL MSP 初始化，当前配置 SPI1 的 GPIO 复用引脚。
- `Core/Src/stm32f4xx_it.c`：中断处理文件，目前只有基础 Cortex-M 异常和 SysTick。
- `CMakeLists.txt`：顶层 CMake 文件，当前已将 `Core/Src/ad9833.c` 和 `Core/Src/Output.c` 加入编译。
- `cmake/stm32cubemx/CMakeLists.txt`：CubeMX 生成的源文件、HAL 驱动和 include 路径配置。
- `CMakePresets.json`：Debug/Release 构建预设。
- `.vscode/launch.json`：ST-Link GDB Server 调试配置。
- `.clangd`：clangd 使用 `build/Release` 下的编译数据库。
- `Core/Src/Output.c`：比赛应用层输出模块，绑定当前板子的 `SPI1 + PA4 + 25 MHz MCLK`，封装正弦波/三角波输出、频率设置、相位设置和停止输出。
- `Core/Src/Output.h`：Output 模块公开接口，`main.c` 和比赛代码优先包含这个头文件。

## 当前硬件连接与外设配置

AD9833 当前连接假设：

- `AD9833_CS` 或 `FSYNC`：`PA4`，在 `main.h` 中定义为 `AD9833_CS_Pin` 和 `AD9833_CS_GPIO_Port`。
- `SPI1_SCK`：`PA5`。
- `SPI1_MOSI`：`PA7`。
- 当前未使用 `MISO`，但 `main.c` 中 SPI1 配置为 `SPI_DIRECTION_2LINES`。
- AD9833 MCLK 当前代码按 `25000000UL` 处理，即 25 MHz。

`main.c` 中 SPI1 当前配置：

- Master 模式。
- 8-bit 数据宽度。
- 软件 NSS。
- MSB first。
- BaudRatePrescaler 为 `SPI_BAUDRATEPRESCALER_16`。
- CPOL 当前为 `SPI_POLARITY_HIGH`。
- CPHA 当前为 `SPI_PHASE_1EDGE`。

注意：AD9833 常见配置多使用 SPI Mode 2 或兼容时序，具体 CPOL/CPHA 要以实际模块和示波器波形为准。修改 SPI 时序时，要在注释里说明为什么改，并建议用逻辑分析仪或示波器确认 `SCLK`、`MOSI`、`FSYNC`。

## 当前 AD9833 / Output 代码状态

当前已经完成 AD9833 输出逻辑的模块化封装：

- `ad9833.c/.h` 保持为底层通用驱动，只负责 AD9833 寄存器和 SPI 通信。
- `Output.c/.h` 是比赛应用层模块，绑定当前板子的 `hspi1`、`AD9833_CS_GPIO_Port`、`AD9833_CS_Pin` 和 25 MHz MCLK。
- `main.c` 不再直接调用一长串 `AD9833_Init`、`AD9833_SetFrequency`、`AD9833_SetPhase`、`AD9833_Start`，而是在 `USER CODE BEGIN 2` 中调用 `Output_Init()` 和 `Output_SetWave()`。
- `CMakeLists.txt` 已把 `Core/Src/Output.c` 加入编译。
- 已执行 `cmake --build --preset Debug`，Debug 构建通过，生成 `OutputCMake.elf`。

当前 `main.c` 示例输出：

```c
if (Output_Init() != HAL_OK)
{
  Error_Handler();
}
if (Output_SetWave(OUTPUT_WAVE_SINE, 20000.0, 0.0) != HAL_OK)
{
  Error_Handler();
}
```

比赛现场优先使用这些接口：

```c
HAL_StatusTypeDef Output_Init(void);
HAL_StatusTypeDef Output_SetWave(Output_Waveform waveform, double frequency_hz, double phase_deg);
HAL_StatusTypeDef Output_Sine(double frequency_hz, double phase_deg);
HAL_StatusTypeDef Output_Triangle(double frequency_hz, double phase_deg);
HAL_StatusTypeDef Output_SetPhase(double phase_deg);
HAL_StatusTypeDef Output_Stop(void);
```

典型调用方式：

```c
Output_Init();
Output_SetWave(OUTPUT_WAVE_SINE, 20000.0, 0.0);      // 正弦波，20 kHz，0 度相位
Output_SetWave(OUTPUT_WAVE_TRIANGLE, 1000.0, 90.0);  // 三角波，1 kHz，90 度相位

Output_Sine(20000.0, 0.0);                           // 快捷正弦波接口
Output_Triangle(1000.0, 90.0);                       // 快捷三角波接口
```

`Output.c` 中当前集中管理的比赛常改参数：

```c
#define OUTPUT_AD9833_MCLK_HZ        25000000UL
#define OUTPUT_DEFAULT_FREQ_HZ       20000.0
#define OUTPUT_DEFAULT_PHASE_DEG     0.0
```

如果 AD9833 模块实际晶振不是 25 MHz，优先修改 `OUTPUT_AD9833_MCLK_HZ`。

## 代码风格要求

后续写 C 代码时，遵守以下约定：

- 使用 STM32 HAL 风格，返回值尽量使用 `HAL_StatusTypeDef`。
- 外设初始化失败时不要静默忽略，要返回错误或进入 `Error_Handler()`。
- 关键参数必须检查范围，例如频率不能为负，不能超过 AD9833 理论上限 `MCLK / 2`。
- 对比赛常改参数使用宏集中管理，例如：

```c
#define AD9833_MCLK_HZ        25000000UL
#define DDS_DEFAULT_FREQ_HZ   20000.0
#define DDS_DEFAULT_PHASE_DEG 0.0
```

- 不要把大量业务逻辑堆在 `while (1)` 或 `USER CODE BEGIN 2` 中。
- 尽量让 `main.c` 只负责系统启动和调度，把外设细节放到模块文件里。
- 保留 CubeMX 的自动生成结构，只在 `USER CODE` 区域内增加代码，除非用户明确要求重构生成区外的代码。
- 新增模块时同步修改 CMake，把 `.c` 文件加入 `target_sources` 或合适的 CubeMX/CMake 源文件列表。

## 注释要求

用户希望在学习和备赛过程中能看懂代码，所以注释要比普通工程更详细，但不要写废话。

应该重点注释：

- 每个模块文件开头说明模块用途、硬件依赖、使用步骤。
- 每个对外函数说明功能、参数、返回值、典型调用场景。
- AD9833 频率字计算公式：`freq_word = frequency_hz * 2^28 / mclk_hz`。
- AD9833 相位字计算公式：`phase_word = phase_deg * 4096 / 360`。
- `FSYNC` 为什么要手动拉低/拉高。
- SPI CPOL/CPHA、片选时序、MCLK、输出幅度偏置等调试易错点。
- 比赛现场可能要改的地方，比如频率、波形、引脚、MCLK。

避免这类无效注释：

```c
// i 加 1
i++;
```

更好的注释示例：

```c
// AD9833 的一次写入固定为 16 bit，这里拆成两个 8 bit 数据交给 HAL SPI 发送。
tx[0] = (uint8_t)(word >> 8);
tx[1] = (uint8_t)(word & 0xFFU);
```

## 模块化状态与建议

当前模块化已经采用 `Output.c/.h` 方案：

1. `ad9833.c/.h` 作为 AD9833 底层驱动，不绑定具体板子的 `hspi1` 或 `PA4`。
2. `Output.c/.h` 作为比赛应用层输出模块，绑定本板 `SPI1 + PA4 + 25 MHz MCLK`。
3. `main.c` 只负责系统初始化和调用 `Output_Init()`、`Output_SetWave()`。
4. 如果后续支持按键、OLED、ADC、PWM、定时器等，也按同样方式独立成模块，不要互相硬耦合。

当前推荐目录形式：

```text
Core/Inc/ad9833.h      // AD9833 底层驱动接口
Core/Src/ad9833.c      // AD9833 底层驱动实现
Core/Src/Output.h      // 比赛应用层输出接口
Core/Src/Output.c      // 绑定本板 SPI/CS/MCLK 的输出封装
Core/Src/main.c        // 系统初始化 + 调用 Output 模块
```

注意：`Output.h` 当前按用户要求放在 `Core/Src` 文件夹中。因为 `main.c` 与 `Output.h` 同目录，当前 Debug 构建可以正常找到该头文件。

## AD9833 驱动理解

`ad9833.c` 当前已经实现：

- `AD9833_Init`：初始化句柄，保存 SPI、FSYNC GPIO、MCLK，并让 AD9833 进入 reset 状态。
- `AD9833_SetFrequency`：写 `FREQ0` 或 `FREQ1`，内部使用 28 位频率字，先写低 14 位，再写高 14 位。
- `AD9833_SetPhase`：写 `PHASE0` 或 `PHASE1`，内部使用 12 位相位字。
- `AD9833_SetWaveform`：支持正弦波和三角波。
- `AD9833_Start` / `AD9833_Reset`：控制 reset 位。
- `AD9833_Sleep`：控制休眠位。

当前未实现：

- 方波输出。
- 幅度控制。AD9833 本身不直接提供数字幅度设置，通常需要外部放大、衰减、电位器、VGA 或 DAC 控制模拟链路。
- 扫频、调频、按键调参、屏幕显示等比赛常见功能。

如果后续添加方波，要先查 AD9833 数据手册，正确处理 `OPBITEN`、`DIV2`、`MODE` 位，并在注释中说明输出来源和限制。

## 构建与调试

常用构建预设：

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

或：

```powershell
cmake --preset Release
cmake --build --preset Release
```

当前 `.vscode/launch.json` 使用 ST-Link GDB Server 调试。调试前确认：

- ST-Link 已连接。
- 板卡供电正常。
- 工程已成功编译生成 `.elf`。
- AD9833 模块与 STM32 共地。

## 比赛场景下的协作原则

- 用户赶时间时，优先给出能直接复制进工程、能编译、能测波形的代码。
- 每次修改后说明改了哪些文件、哪些函数、如何调用、如何验证。
- 对涉及硬件的功能，给出示波器或万用表检查点。
- 遇到不确定的硬件参数时，代码中使用清晰宏定义，并提醒用户按实际硬件修改。
- 不随意删除 CubeMX 生成代码，不随意改动启动文件、链接脚本、HAL 驱动源码。
- 如果修改 CMake，要保持工程仍能用 Debug/Release 预设构建。
- 如果用户让“封装成模块”，默认同时提供 `.h`、`.c`、CMake 修改和 `main.c` 调用示例。

## 当前后续优先任务建议

Output 模块已经完成并通过 Debug 构建。后续如果继续增强，建议按比赛常见需求推进：

1. 增加按键或串口命令，调用 `Output_SetWave()` 动态切换频率、相位和波形。
2. 增加 OLED/LCD 显示，把当前频率、相位、波形显示出来。
3. 如需方波输出，先扩展 `ad9833.c/.h` 的波形枚举和控制位，再在 `Output.c/.h` 增加快捷接口。
4. 如需扫频，新增独立模块或函数，不要把扫频循环直接塞进 `main.c`。
5. 每次新增模块后同步更新 `CMakeLists.txt` 并运行 `cmake --build --preset Debug` 验证。


