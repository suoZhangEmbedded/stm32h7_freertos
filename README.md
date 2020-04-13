# stm32h7_freertos

开发板：armfly H7-TOOL ,（25M晶振）

1、移植 FreeRTOS V10.2，测试OK.

2、移植 easylogger 功能， 进行log 日志 有颜色的 输出，打开 j-link RTT Viewer 软件 即可看到颜色输出。
	

	easylogger:https://github.com/armink/EasyLogger

3、移植 lwip 2.1.2 版本 ， 并使用 netconn 接口实现 TCP client 功能。
	

	驱动移植参考：https://www.stmcu.org.cn/document/detail/index/id-217958

4、实现 网线 热插拔 ，以及 TCP 客户端 keepalive 功能 

5、添加 daplink 源码 2020年4月13日16:40:19