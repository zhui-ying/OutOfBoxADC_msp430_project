使用说明：
note：在原有OutOfBox工程的基础上面新增 USER MODE，原有功能完全保留
操作步骤：
1、参考文档project note.txt连接相关硬件到demo板上，注意RN41作为蓝牙从机模块连接到demo板上，HC05作为主模块通过USB转串口模块连接到PC。
2、demo板和串口模块上电，两个蓝牙模块会自动连接。
3、同时长按S1，S2进入stopwatch mode。
4、S1启动和停止秒表，S2清除计数。
5、同时长按S1，S2进入temperature mode。
6、S1锁定显示，S2切换显示单位。
7、同时长按S1，S2进入user mode。
8、S1锁定显示，S2切换显示芯片内部温度值和电位器设置值。
9、打开上位机显示软件 ‘temperatureTest.exe’,选择Baud Rate：384000，COM#为设备管理器显示的COM口，然后点击 open,即可同步显示芯片内部温度值和设置的温度值。