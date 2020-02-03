
# opencan
linux系统下开源的can测试工具
基于linux驱动中slcan协议实现socketcan的通信  

![Image text](https://github.com/JiaDuo/opencan/blob/master/hardware/usb2can/hardware2.png)

## 硬件介绍  
硬件使用MicroUSB和PC主机通信，支持一路CAN 2.0B，最大波特率1Mbps
## 更新固件
使用stlink/jlink连接到J2引脚，CLK DIO GND  

linux系统下安装stlink-tools  
apt install stlink-tools  

下载固件到MCU中  
st-flash --reset write ./build/usb2can.bin 0x08000000  

## 使用
基于slcan/socketcan的软件都可以应用到当前设备，python-can、can-utils等     
* 安装can-utils  
apt install can-utils
* 建立socketcan设备
slcand -o -s8 /dev/ttyACM0 can0   
ip link set up can0
* 收发报文
可以使用ifconfig查看can网络状态，使用can-utils中提供的软件来进行报文收发  
eg：  
candump can0  
cansniffer -c can0  
cansend can0 123#R3  

## 一些参考文档
[slcan]https://elinux.org/Bringing_CAN_interface_up  
[slcan-api]https://github.com/linux-can/can-misc/blob/master/docs/SLCAN-API.pdf

