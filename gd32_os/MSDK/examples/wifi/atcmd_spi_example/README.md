# ATCMD over SPI Example

# Overview
	This example is based on the GD32VW553K-START board, it shows SPI master exec ATCMD to SPI slave using fullduplex
communication with polling & dma mode.
	Two board is used, board0 acts as SPI Master role, which is responsible for exec ATCMD and waiting ATCMD response, send data to the network.
	board1 acts as SPI slave role, which is support ATCMD over SPI interface, mainly to process the ATCMD, connect the network, forward data from SPI Master to the TCP server.

## Topology of example

   Board0                          Board1                            PC
┌─────────────┐                 ┌─────────────┐                   ┌─────────────┐
│             │                 │192.168.3.101│   WiFi Connected  │192.168.3.100│
│             │                 │             │ ◄───────────────► │ 5201        │
│             │       ATCMD     │             │                   │             │
│       SPI Master ─────────►   │SPI Slave    │                   │TCP Server   │
│             │                 │      TCP Client  ─ ─ ─ ─ ─ ─►   │             │
│             │    ◄──RSP────   │             │     data transfer │             │
│             │                 │             │                   │             │
│             │                 │             │                   │             │
└─────────────┘                 └─────────────┘                   └─────────────┘

The Example is based on the aboeve topology. Two GD32VW553K-START board is requred, Board1 should be connected with the PC with an AP.
while the PC should run an TCP Server.
The AP info (SSID, PWD) and the TCP Server info(IP addresss, port) should be specified in the SDK before complile.

# VW553 board0 acting as SPI Master Role

/*==== Please check the following setting before build project =======*/
/*========================== USER CONFIGURATION ======================*/

/* Please set the correct AP information */
#define SSID            				"xxx"
#define PASSWORD        				"xxxxxxx"

/* Please set the correct TCP server information */
#define TCP_SERVER_IP                   "xxx.xxx.xxx.xxx"
#define TCP_SERVER_PORT                 xxxx

/* Please set round number */
#define TEST_ROUND            			10
/*========================= USER CONFIGURATION END ====================*/

## Build/download Example

This example can not be moved to other directory unless the example project configuration is modified.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import example project, select the directory living.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\wifi\atcmd_spi_example\Eclipse_project\atcmd_spi_example\atcmd_spi_example.bin
* use GDLINK/JLINK or dragging it into the USB disk to download image to board0.


# VW553 board1 acting as SPI Slave Role

The feature of ATCMD over SPI slave interface is implemented in MSDK. Please enable the following macros in the file MSDK\app\app_cfg.h

/* Please enable the following macros */
#define CONFIG_ATCMD
#define CONFIG_ATCMD_SPI

## Build/download Example

Building the MSDK project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import the MSDK project.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory scripts\images\image-all.bin
* use GDLINK/JLINK or dragging it into the USB disk to download image to board1.


# Example Usage

## Prepare

### Connect the SPI PIN and sync GPIO PIN

      SPI Master(Board0)		SPI Slave(Board1)
	  MOSI/PA0					MOSI/PA0
	  MISO/PA1					MISO/PA1
	  SCK/PA2				    SCK/PA2
	  NSS/PA4					NSS/PA4
	  SYNC GPIO/PA12            SYNC GPIO/PA5
	  GND						GND

### Connect PC with the specified AP, start TCP server on the specified port.

## Test Steps

### Power up the board1, the board will initialized as a SPI slave, and waiting for atcmd over the SPI interface.

### Power up the board2, the board will initialized as a SPI master. The master is an controller to issue the atcmd to connect to the target AP, TCP Server.

### After connected to the target TCP Server, the SPI master will directy send data to the SPI slave. The SPI slave then forword the data to TCP Server.

### Watchout the log during the test, the failed case and test result will show in log.