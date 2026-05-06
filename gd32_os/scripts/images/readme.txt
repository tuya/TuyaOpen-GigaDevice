image-all-msdk.bin：		    SDK默认状态下使用GD32 Embedded-Builder编译出来的image，直接烧录进Flash即可使用。
								wifi功能：
									1、softAP模式（最大连接3个clients）
									2、station模式，支持802.11 b|g|n|ax，支持wpa|wpa2|wpa3加密模式
									3、station + softAP共存模式（softAP最大链接两个clients）
									4、monitor模式
									5、支持ping、iperf
								ble功能：
									1、支持peripheral
									2、支持一条链路连接
									3、支持server
									4、支持host和controller
									5、支持EATT
									6、支持微信小程序wifi配网
								更多的example可在目录MSDK/examples下寻找。

image-all-msdk-full-app.bin：   SDK在上述功能的基础上额外开启了以下功能：
							    app层：
									1、AT指令
									2、FATFS文件系统
									3、ssl_demo(使用mbedtls实现的简单https的demo)、
									4、socket_demo(调用lwip socket api的socket demo))
								wifi功能：
									1、支持WPS连接
									2、支持EAP-TLS连接
									3、softAP模式支持最大16个clients
									4、支持IPV6
									5、mqtt、mqtt5
									6、COAP
									7、支持sostAP网页模式配网功能
									8、websocket
								ble功能：
									1、支持central
									2、支持四条链路
									3、支持client
									4、支持周期广播
									5、支持phy update
									6、支持power control
									7、支持ble ping
									8、支持secure connection
以上两个image均为mbl.bin和image-ota.bin的结合，有关mbl.bin和image-ota.bin的说明请参考文档
《AN154 GD32VW553 快速开发指南》的3.1小节。

 image烧录进flash之后使用uart输入命令help可查看所有命令，命令具体使用请参考文档
《AN153 GD32VW553 基本指令用户指南》和《AN151 GD32VW553 AT指令用户指南》

