# pthread Wrapper for FreeRTOS

## 概述

这是一个基于FreeRTOS的POSIX Threads (pthread) API包装层实现，允许您在FreeRTOS环境中使用标准的pthread接口。

## 文件列表

- `pthread.h` - pthread API头文件
- `pthread.c` - pthread API实现文件
- `pthread_example.c` - 使用示例

## 功能特性

### 支持的pthread功能

#### 1. 线程管理
- ✅ `pthread_create()` - 创建新线程
- ✅ `pthread_exit()` - 退出当前线程
- ✅ `pthread_join()` - 等待线程结束
- ✅ `pthread_detach()` - 分离线程
- ✅ `pthread_self()` - 获取当前线程句柄
- ✅ `pthread_equal()` - 比较线程ID

#### 2. 线程属性
- ✅ `pthread_attr_init()` - 初始化线程属性
- ✅ `pthread_attr_destroy()` - 销毁线程属性
- ✅ `pthread_attr_setstacksize()` - 设置栈大小
- ✅ `pthread_attr_getstacksize()` - 获取栈大小
- ✅ `pthread_attr_setdetachstate()` - 设置分离状态
- ✅ `pthread_attr_getdetachstate()` - 获取分离状态

#### 3. 互斥锁 (Mutex)
- ✅ `pthread_mutex_init()` - 初始化互斥锁
- ✅ `pthread_mutex_destroy()` - 销毁互斥锁
- ✅ `pthread_mutex_lock()` - 加锁
- ✅ `pthread_mutex_trylock()` - 尝试加锁
- ✅ `pthread_mutex_unlock()` - 解锁
- ✅ `pthread_mutexattr_init()` - 初始化互斥锁属性
- ✅ `pthread_mutexattr_destroy()` - 销毁互斥锁属性
- ✅ `pthread_mutexattr_settype()` - 设置互斥锁类型
- ✅ `pthread_mutexattr_gettype()` - 获取互斥锁类型

支持的互斥锁类型：
- `PTHREAD_MUTEX_NORMAL` - 普通互斥锁
- `PTHREAD_MUTEX_RECURSIVE` - 递归互斥锁
- `PTHREAD_MUTEX_ERRORCHECK` - 错误检查互斥锁

#### 4. 条件变量
- ✅ `pthread_cond_init()` - 初始化条件变量
- ✅ `pthread_cond_destroy()` - 销毁条件变量
- ✅ `pthread_cond_wait()` - 等待条件变量
- ✅ `pthread_cond_timedwait()` - 带超时的等待
- ✅ `pthread_cond_signal()` - 唤醒一个等待线程
- ✅ `pthread_cond_broadcast()` - 唤醒所有等待线程
- ✅ `pthread_condattr_init()` - 初始化条件变量属性
- ✅ `pthread_condattr_destroy()` - 销毁条件变量属性

## 配置选项

在使用前，可以在项目中定义以下宏来自定义行为：

```c
/* 默认线程栈大小(单位：字) */
#define PTHREAD_DEFAULT_STACK_SIZE    2048

/* 默认线程优先级 */
#define PTHREAD_DEFAULT_PRIORITY      (tskIDLE_PRIORITY + 1)
```

## 使用方法

### 1. 添加文件到项目

将以下文件添加到您的项目中：
- `pthread.h`
- `pthread.c`

### 2. 包含头文件

```c
#include "pthread.h"
```

### 3. 基本示例

#### 创建和等待线程

```c
void *my_thread_function(void *arg)
{
    int value = *(int *)arg;
    printf("Thread running with value: %d\n", value);
    return (void *)(long)(value * 2);
}

void create_thread_example(void)
{
    pthread_t thread;
    int input = 42;
    void *retval;

    // 创建线程
    pthread_create(&thread, NULL, my_thread_function, &input);

    // 等待线程完成
    pthread_join(thread, &retval);

    printf("Thread returned: %ld\n", (long)retval);
}
```

#### 使用互斥锁保护共享资源

```c
pthread_mutex_t my_mutex;
int shared_counter = 0;

void thread_safe_increment(void)
{
    pthread_mutex_lock(&my_mutex);
    shared_counter++;
    pthread_mutex_unlock(&my_mutex);
}

void mutex_example(void)
{
    // 初始化互斥锁
    pthread_mutex_init(&my_mutex, NULL);

    // 使用互斥锁...
    thread_safe_increment();

    // 销毁互斥锁
    pthread_mutex_destroy(&my_mutex);
}
```

#### 使用条件变量实现生产者-消费者模式

```c
pthread_mutex_t mutex;
pthread_cond_t cond;
int data_ready = 0;

void *producer(void *arg)
{
    pthread_mutex_lock(&mutex);
    // 生产数据
    data_ready = 1;
    pthread_cond_signal(&cond);  // 通知消费者
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *consumer(void *arg)
{
    pthread_mutex_lock(&mutex);
    while (!data_ready) {
        pthread_cond_wait(&cond, &mutex);  // 等待数据就绪
    }
    // 消费数据
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void producer_consumer_example(void)
{
    pthread_t prod, cons;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&cons, NULL, consumer, NULL);
    pthread_create(&prod, NULL, producer, NULL);

    pthread_join(cons, NULL);
    pthread_join(prod, NULL);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}
```

## 实现说明

### 映射关系

| pthread API | FreeRTOS API |
|------------|--------------|
| pthread_create | xTaskCreate |
| pthread_mutex | Mutex Semaphore |
| pthread_cond | Counting Semaphore |
| pthread (recursive_mutex) | Recursive Mutex |

### 注意事项

1. **栈大小单位**: pthread中栈大小以字节为单位，内部会自动转换为FreeRTOS的字(word)单位

2. **优先级**: 可通过线程属性设置优先级，默认为`tskIDLE_PRIORITY + 1`

3. **返回值**: 所有函数返回0表示成功，非0表示错误码

4. **内存管理**: 使用FreeRTOS的`pvPortMalloc()`和`vPortFree()`进行内存分配

5. **条件变量实现**: 基于计数信号量实现，适用于大多数场景

6. **线程分离**: 分离的线程结束后会自动清理资源

7. **超时处理**: `pthread_cond_timedwait()`的超时时间处理是简化版本，实际使用中可能需要更精确的时间转换

## 限制和已知问题

1. **pthread_exit()**: 当前实现可能无法完全保存返回值，建议使用return从线程函数返回

2. **取消点**: 未实现pthread_cancel()及相关取消点功能

3. **线程本地存储**: 未实现pthread_key_xxx相关TLS功能

4. **读写锁**: 未实现pthread_rwlock_xxx读写锁功能

5. **信号量**: 未实现pthread_sem_xxx POSIX信号量（可直接使用FreeRTOS信号量）

6. **屏障**: 未实现pthread_barrier_xxx屏障功能

7. **自旋锁**: 未实现pthread_spin_xxx自旋锁功能

## 性能考虑

- 每个线程创建时会分配控制块（PCB），使用完毕后需正确join或detach以释放资源
- 互斥锁和条件变量也会分配控制块，使用后需要destroy
- 建议根据实际需求调整默认栈大小和优先级

## 移植到其他RTOS

如需移植到其他RTOS，主要需要修改`pthread.c`中的以下部分：
- 任务创建和删除
- 信号量操作
- 内存分配
- 时间转换

## 示例程序

完整的示例代码请参考`pthread_example.c`，包含：
1. 简单线程创建和join
2. 使用线程属性
3. 互斥锁保护共享资源
4. 递归互斥锁
5. 条件变量（生产者-消费者）
6. 条件变量broadcast

运行示例：
```c
void app_main(void)
{
    pthread_examples_main();
}
```

## 许可证

本代码基于FreeRTOS许可证，可自由使用和修改。

## 版本历史

- v1.0.0 (2026-01-28): 初始版本
  - 实现基本的线程管理功能
  - 实现互斥锁（普通和递归）
  - 实现条件变量
  - 提供完整示例代码

## 联系方式

如有问题或建议，请联系项目维护者。
