# stm32h7_freertos

开发板：Nucleo-H743ZI ,（8M晶振）

1、移植 FreeRTOS V10.2，测试OK.

2、重定向 printf 到 串口3， ST-LINK 虚拟串口 可以直接打印数据,测试OK。

3、刷 ST-LINK 到 jlink 后， 不仅支持 虚拟串口，还能使用 jlink 下的 各种工具包，爽歪歪。
	
	教程:https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/

4、移植 easylogger 功能， 进行log 日志 有颜色的 输出，打开 j-link RTT Viewer 软件 即可看到颜色输出。
	
	easylogger:https://github.com/armink/EasyLogger

5、移植 lwip 2.1.2 版本 ， 并使用 netconn 接口实现 TCP client 功能。
	
	驱动移植参考：https://www.stmcu.org.cn/document/detail/index/id-217958

6、实现 网线 热插拔 ，以及 TCP 客户端 keepalive 功能 

