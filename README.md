# C++ 高性能线程池库

从零实现的 C++ 线程池库，包含 **3 种线程池 + 4 版同步队列 + 定时器系统**，逐层迭代演进。

## 项目介绍

不使用任何第三方库，基于 C++ 标准库多线程原语（mutex、condition_variable、future、packaged_task）自底向上构建了完整的线程池框架。

### 架构概览

```
                        ┌─────────────────────────────┐
                        │      TimerManage             │
                        │   (epoll 事件循环线程)        │
                        │   runAt / runAfter / runEvery │
                        └────────────┬────────────────┘
                                     │ 投递回调
                                     ▼
┌──────────────────────────────────────────────────────────┐
│                    线程池 (3 种策略)                       │
│                                                          │
│  FixedThreadPool      CachedTreadPool    WorkStealingPool │
│  [固定线程+全局队列]   [动态伸缩+回收]    [per-thread队列] │
│       │                    │                   │         │
│       └────────────────────┴───────────────────┘         │
│                          │                               │
│                          ▼                               │
│              SyncQueue (4 版进化)                          │
│              deque + mutex + condition_variable           │
└──────────────────────────────────────────────────────────┘
```

### SyncQueue 进化路线

```
syncQueue_1 ──→ syncQueue_2 ──→ syncQueue_3 ──→ syncQueue_4
 (基础队列)     (+优雅关闭)     (+返回状态码)    (+多队列数组)
    │               │               │               │
    ▼               ▼               ▼               ▼
FixedThreadPool     │         CachedTreadPool  WorkStealingPool
                    └───────────────┴───────────────┘
```

## 功能列表

- [x] FixedThreadPool — 固定线程数 + 全局单队列
- [x] CachedTreadPool — 动态伸缩 + 空闲线程自动回收
- [x] WorkStealingPool — per-thread 队列 + 工作窃取
- [x] submit() 支持带返回值异步调用（future/packaged_task）
- [x] 4 版同步队列（有界阻塞 / 优雅关闭 / 状态码 / 多队列）
- [x] Timer — timerfd 内核定时器
- [x] TimerManage — epoll 事件循环 + runAt/runAfter/runEvery
- [x] Timestamp — 微秒精度时间戳

## 技术栈

- C++20（std::latch 等）
- CMake 构建
- Linux timerfd / epoll
- 多线程：mutex、condition_variable、atomic、future、packaged_task
- 设计模式：RAII、单例、策略模式

## 目录结构

```
.
├── CMakeLists.txt
├── include/
│   ├── FixedThreadPool.hpp      # 固定线程池
│   ├── CachedTreadPool.hpp      # 可缓存伸缩线程池
│   ├── WorkStealingPool.hpp     # 工作窃取线程池
│   ├── syncQueue_1.hpp          # 基础有界阻塞队列
│   ├── syncQueue_2.hpp          # + 优雅关闭
│   ├── syncQueue_3.hpp          # + 返回状态码
│   ├── syncQueue_4.hpp          # + 多队列数组
│   ├── Timer.hpp                # timerfd 定时器
│   ├── TimerManage.hpp          # epoll 定时器管理器
│   ├── Timestamp.hpp            # 时间戳
│   └── LogCommon.hpp            # 日志
├── src/
│   ├── Timer.cpp
│   ├── TimerMange.cpp
│   └── Timestamp.cpp
├── example/
│   ├── CMakeLists.txt
│   ├── threadpoolTest1.cpp      # FixedThreadPool 测试
│   ├── threadpoolTest2.cpp      # CachedTreadPool 测试
│   ├── threadpoolTest3.cpp      # WorkStealingPool 测试
│   └── threadpoolTest4.cpp      # Timer 测试
└── bin/                         # 编译产物
```

## 编译运行

### 1. 编译

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

编译产物在 `bin/`：`threadpool1` / `threadpool2` / `threadpool3` / `threadpool4`。

### 2. 运行测试

```bash
# FixedThreadPool — 10000 个随机向量生成+排序，单线程 vs 线程池对比
./bin/threadpool1

# CachedTreadPool — 动态伸缩验证
./bin/threadpool2

# WorkStealingPool — 工作窃取验证
./bin/threadpool3

# Timer — 定时器测试
./bin/threadpool4
```

## 三种线程池对比

| 维度 | FixedThreadPool | CachedTreadPool | WorkStealingPool |
|------|----------------|-----------------|-------------------|
| 线程数 | 恒定 | [core, max] 动态 | 恒定 |
| 队列结构 | 1 个全局 deque | 1 个全局 deque | N 个 per-thread deque |
| 锁竞争 | 高（全局锁） | 高（全局锁） | 低（独立队列） |
| 扩容 | 无 | 空闲耗尽时创建 | 无 |
| 缩容 | 无 | 空闲超时 detach | 无 |
| 负载均衡 | 单队列自然平均 | 单队列自然平均 | 轮询投递 + 窃取 |
| 适用场景 | 负载稳定 | 波峰波谷 | 任务粒度不均 |

## 核心设计决策

| 层面 | 决策 | 原因 |
|------|------|------|
| 线程管理 | `shared_ptr<thread>` | 防止 `thread` 析构时未 join 导致 `terminate` |
| 停止保护 | `call_once` + `once_flag` | 多线程可能同时调 `Stop()` |
| 队列满 | 同步执行降级 | 宁可阻塞当前线程也不丢任务 |
| 缩容 | 线程自己 `detach` | 线程退出只能自己操作自己 |
| 窃取策略 | `move` 整个队列 | 减少锁持有次数，批量处理 |
| 定时器 | `timerfd` + epoll | 内核高精度定时，不自己造轮子 |

## 使用示例

### FixedThreadPool

```cpp
#include "FixedThreadPool.hpp"

pool::FixedThreadPool pool(2000, thread::hardware_concurrency());

// 无返回值
pool.AddTask([]{ cout << "hello" << endl; });

// 有返回值
auto result = pool.submit([](int a, int b) { return a + b; }, 3, 4);
cout << result.get() << endl;  // 7
```

### CachedTreadPool

```cpp
#include "CachedTreadPool.hpp"

// 核心 4 线程，最大 hardware+2，空闲 5 秒退出
pool::CachedTreadPool pool(4, 500, 5);

pool.AddTask([]{ /* 任务 */ });
// 空闲线程自动扩缩，无需手动管理
```

### WorkStealingPool

```cpp
#include "WorkStealingPool.hpp"

pool::WorkStealingPool pool(500, 8);  // 8 线程，队列容量 500

pool.submit([]{ /* 任务 */ });
// 自动轮询分配队列 + 空闲线程窃取
```

### Timer

```cpp
#include "TimerManage.hpp"
#include "FixedThreadPool.hpp"

pool::TimerManage tm;
tm.Init(100);  // 100ms epoll 超时

tm.runAfter(3, []{ cout << "3秒后执行" << endl; });
tm.runEvery(1, []{ cout << "每秒执行" << endl; });
```
