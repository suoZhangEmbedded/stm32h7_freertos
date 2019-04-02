# stm32h7_freertos

开发板：STM32H743 Nucleo-144 8M晶振

1、移植 FreeRTOS V10.2，测试OK.

2、重定向 printf 到 串口3， ST-LINK 虚拟串口 可以直接打印数据,测试OK。

3、刷 ST-LINK 到 jlink 后， 不仅支持 虚拟串口，还能使用 jlink 下的 各种工具包，爽歪歪。

教程:https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/
