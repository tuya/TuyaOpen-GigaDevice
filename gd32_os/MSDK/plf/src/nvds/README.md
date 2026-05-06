
NVDS(Non-Volatile Data Storage)非易失性数据存储，本库中操作使用的非易失性存储介质主要为Flash。下文将详细介绍NVDS的基本概念，数据管理，以及API的使用。如需快速了解API的使用，可直接至API参考小节，跳过基础概念知识的介绍并不会影响您对API的正确使用。

---

# 相关概念

目前使用的存储介质主要是芯片SIP Flash的部分空间，简单介绍Flash的特性以便对后续设计能更好的理解。如需要使用芯片外挂QSPI设备，仅需修改Flash操作接口，即可以很方便在QSPI上实现NVDS存储。
- Flash 擦除以Sector为最小单位，目前Flash通用的Sector Size为4KB，下文中所指Sector均默认为4KB。需要说明的是，该库同时支持调整Sector的值，其具体大小可以根据使用Flash的设定来调整。
- Flash另一基本特性是，执行Erase之后Flash数据为全0xFF，即所有位均为1，通过Write操作可将相应位的值修改为0，如需要该位的值修改为1，只能通过Flash Erase操作。
- 不同型号的Flash执行Sector Erase所需时间稍有差别。参照GD25Q64C规格说明书，其执行Sector Erase 耗时典型值为50ms，GD25Q16C 则为45ms。
- 考虑到Flash Erase耗时的因素，NVDS在执行数据写入时，不会立即执行Flash Erase操作，而是采取数据管理策略，减少Flash Erase的次数，提高数据存取效率。
- NVDS在数据组织上采用最小化管理，即按需求使用最少位来定义其功能。
- 鉴于此，该组织管理方式大幅降低了Flash Erase频率，同时在Flash 不同Block间支持磨损均衡，有效降低Block的Erase频率。
- 此外NVDS适合数据量较小时存储，如需存储较大数据，可使用FAT文件系统等其他存储管理方式。

## 1. Key-Value 存储

NVDS中采用Key-Value键值对的方式来存储数据。Key为ASCII字符串，最大长度支持15个字节。Key必须唯一，为现有的键写入新值时，会将旧的值更新为新写入指定的值。
为了防止不同模块或者组件之间键名存在潜在冲突，提供两级隔离，第一级为不同的Flash Partition，即利用存在不同Flash存储空间完成键隔离，并且给每一个对应的Flash Partition指定不同label，以便于区分。此外，在相同的Flash Partition中引入Namespace命名空间进行第二级隔离。即需要保证在相同的Flash Partition，同一Namespace空间内键是唯一的。
Namespce命名空间遵循和键名相同的规则，即最大长度支持15个字节。同一Flash Partition中最多支持定义253个不同的Namespace。

## 2. 存储数据类别

根据Key-Value键值中数据的长度不同，对应3种不同的数据类别。
当数据长度小于8bytes时，Key-Value键值对定义为Small element；当数据长度在8~256bytes之间时，为Middle element；当数据长度大于256bytes时，为Bulk element。
根据上述Key-Value键值对数据长度不同的限制，约定Small element和Middle element必须连续存储，只有Bulk element支持存储在一个或者多个Flash Sector中。以GD32VW5XX SDK中default创建的 innternal NVDS存储为例，该区域为4个 sector，Bulk element的数据长度最大为125 * 4 * 32=16000bytes，但建议最长不超过400 * 32 = 12800bytes。该库中也已值来限制存储最大数据长度。
不同的element由1条或者多条Entry构成，其中Entry为32Bytes, 其基本数据结构如下表。

| Symbol | NS    | Type & FragNo.                      | Length | CRC    | Key     | Value  |
| :----- | :---- | :---------------------------------- | :----- | :----- | :------ | :----- |
| Size   | 1Byte | [7:5] Element Type<br>[4:0] FragNo. | 2Bytes | 4Bytes | 16Bytes | 8Bytes |

其中NS字段指明该key所属的Namespace，Type&FragNo字段中，使用4bit表示不同element type，具体如下：

| Element Type   | Description      |
| -------------- | ---------------- |
| [7:5] = 'b000' | Small element    |
| [7:5] = 'b001' | Middle element   |
| [7:5] = 'b010' | Bulk element     |
| [7:5] = 'b011' | Bulk Information |

- **Small element**

如上文所述，当数据长度小于8byte时，仅需要一条Entry即可表示Small element，Small element数据结构如下表所示。

| NameSpace | Type & FragNo.                      | Length | CRC    | Key     | Value  |
| :-------- | :---------------------------------- | :----- | :----- | :------ | :----- |
| 1Byte     | [7:5] Element Type<br>[4:0] FragNo. | 2Bytes | 4Bytes | 16Bytes | 8Bytes |

其中Length字段指明数据实际长度，Key和Value字段分别存储键值对信息，并且利用4bytes CRC进行数据校验。确保Small element所有数据有效。

- **Middle element**

当数据长度在8~256bytes之间时，需要使用多条Entry来存储Middle element，其中第一条Entry用来保存键名，命名空间，数据长度信息，其后Entry i 用来存储数据，并且32byte均用来存储数据，即所需Entry数目为 (Length + 31) / 32.
Middle element中第一条Entry，Entry 0的数据结构如下。

| NameSpace | Type & FragNo.                      | Length | CRC1   | Key     | Reserved | CRC2   |
| :-------- | :---------------------------------- | :----- | :----- | :------ | :------- | :----- |
| 1Byte     | [7:5] Element Type<br>[4:0] FragNo. | 2Bytes | 4Bytes | 16Bytes | 4Bytes   | 4Bytes |

其中Length字段指明数据实际长度，Key字段存储键名，并且利用4bytes CRC1校验该Entry数据有效性，CRC2字段用来校验存储Value信息的有效性。

- **Bulk element**

当数据长度大于256bytes时，Bulk element支持存储在不同的Flash Sector中，即被分成不同Fragment来存储。其中Bulk element的第一条Entry和Middle Element的第一条Entry结构相同。

| NameSpace | Type & FragNo.                      | Length | CRC1   | Key     | Reserved | CRC2   |
| :-------- | :---------------------------------- | :----- | :----- | :------ | :------- | :----- |
| 1Byte     | [7:5] Element Type<br>[4:0] FragNo. | 2Bytes | 4Bytes | 16Bytes | 4Bytes   | 4Bytes |

Element Type[7:5] = 'b011 的Bulk Information来记录存储Bulk element被分片的个数，以及Bulk element总数据长度。Bulk Information的数据结构如下表所示，其中FragNo记录当前分片号，FragCnt记录总被分片个数，BulkSize字段保存Bulk element数据总长度。通常每一个分片数据需要与一个Bulk Information配合来记录分片数据信息，其中Bulk Information数据中CRC字段用来校验分片数据的有效性。

| NameSpace | Type & FragNo.                      | FragCnt | CRC    | Key     | Reserved | BulkSize |
| :-------- | :---------------------------------- | :------ | :----- | :------ | :------- | :------- |
| 1Byte     | [7:5] Element Type<br>[4:0] FragNo. | 2Bytes  | 4Bytes | 16Bytes | 4Bytes   | 4Bytes   |

## 3. Sector 结构

如前文所述，Flash Sector 大小通常为4096Bytes, Sector的结构如下表所示，由Header、Entry states table以及多条Entry构成。

| Description        | Length  |
| :----------------- | :------ |
| Header             | 32Bytes |
| Entry states table | 32Bytes |
| Entry 0            | 32Bytes |
| Entry 1            | 32Bytes |
| ...                | ...     |
| Entry 125          | 32Bytes |

Header 一共32Bytes，由5部分数据构成，其中4bytes Magic Code用来标识该Sector为NVDS存储区域。Version用来记录该库的版本，State记录Sector的存储状态，Sequence用来记录该Sector的序列号，同样，CRC字段用来校验Header数据的有效性。其具体结构如下所示。

| Magic Code | Version | State  | Sequence | Reserved | CRC    |
| :--------- | :------ | :----- | :------- | :------- | :----- |
| 4Bytes     | 2Bytes  | 2Bytes | 4Bytes   | 16Bytes  | 4Bytes |

每一条Entry均处于空(2'b11)、有效(2'b10)、无效(2'b00)这3种状态之一。Entry states table (32byte) 用来记录Entry 0~125的状态。当该Entry还未写入任何内容时，则处于空，即初始化状态(2'b11)。当写入键值对之后，状态被更新为有效(2'b10)。当删除该键值时，原Entry对应的状态修改为无效(2'b00)，如果需要更新修改键值，则除了将原状态修改为无效外，继续在Flash 可用空间顺序记录新的键值，并将新的键值状态更新为有效(2'b10)。
除以上3中状态外，Entry states table中其他值被认为不合法值，并同时将状态修改为无效(2'b00)，检索键值则会忽略无效状态的Entry。

## 4. AES加密

为保护NVDS中存储数据的安全性，本库提供了AES功能支持对NVDS存储数据进行加解密，即存储在Flash中的数据不再是明文形式，而是以密文形式存储。AES key由HUK和NVDS label信息派生而来，因此保证不同NVDS区域使用的Key是不同的。值得一提的是：
- 本库并不提供Flash防擦除保护。
- 另外可以与芯片安全启动+AES加密共存。

---

# API参考

Header File
- MSDK\plf\src\nvds\nvds_flash.h
- 在该文件中包含相关API声明。

下文简单介绍每个API的功能和使用方法。

## 1. 初始化

```
void *nvds_flash_init(uint32_t start_addr, uint32_t size, const char *label);
```

```
void nvds_flash_deinit(void *handle);
```

nvds_flash_init函数完成对NVDS environment的初始化，参考包括指定该NVDS区域的start adress，size和label信息。其中区域位置信息的指定需要遵从config_gdm32.h中Flash Layout，防止区域错误，导致功能运行不正常。该函数的返回值为操作该区域的handle句柄，操作同一区域需要使用相同的handle句柄。

nvds_flash_deinit函数完成对NVDS environment使用SRAM空间的释放，注意该函数并不会擦除Flash空间。

## 2. 读取键值

```
int nvds_data_get(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t *length);
```

该函数根据指定的handle句柄，namespace命令空间，以及key键名，返回键值数据，和数据实际length。其中namespace字符串最大长度为15byte，key最大长度为15byte。
注意保存键值数据的buffer由使用者申请和释放，nvds并不负责管理该buffer。
针对不确定键值数据长度时，可以先获取长度信息，然后根据获取的长度来申请内存空间，最后获取键值数据。
函数的返回值参考Error Code说明。

```
 // Example (without error checking) of using nvds_data_get to get element value:

 uint32_t required_size;

 int ret = nvds_data_get(NULL, namespace, key, NULL, &required_size);

 char *buffer = malloc(required_size);

 int ret = nvds_data_get(NULL, namespace, key, buffer, &required_size);

 free(buffer);
```

## 3. 更新键值

```
int nvds_data_put(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t length);
```

参数包含handle句柄，namespace命令空间，key键名以及data数据和数据长度信息，其中namespace字符串最大长度为15byte，key最大长度为15byte。
该函数用来写入新的键值对，或者更新已存在键值的数据。
函数的返回值参考Error Code说明。

## 4. 删除键值

```
int nvds_data_del(void *handle, const char *namespace, const char *key);
```

参数包含handle句柄，namespace命令空间，key键名，其中namespace字符串最大长度为15byte，key最大长度为15byte。
该函数将已存在键值的数据删除。
函数的返回值参考Error Code说明。

---

# Error Code

```
// NVDS status OK

NVDS_OK = 0,

// Not use flash, only used for not define NVDS_FLASH_SUPPORT

NVDS_E_NOT_USE_FLASH,

// generic NVDS status

NVDS_E_FAIL,

// NVDS invalid parameter

NVDS_E_INVAL_PARAM,

// flash read/write/erase api fail

NVDS_E_FLASH_IO_FAIL,

// NVDS data element not found

NVDS_E_NOT_FOUND,

// NVDS invalid length when get data

NVDS_E_INVALID_LENGTH,

// No space(flash/sram) for NVDS

NVDS_E_NO_SPACE,

// NVDS security config setting fail

NVDS_E_SECUR_CFG_FAIL,

// NVDS decryption failed while reading data

NVDS_E_DECR_FAIL,

// NVDS encryption failed while writing data

NVDS_E_ENCR_FAIL,
```