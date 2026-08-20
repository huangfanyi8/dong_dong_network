# dong_dong_network_requires_v1 —— 项目需求文档（指挥官）

> **本文件是项目唯一需求来源**：定义"要做什么"。
> 协作方式与执行规范 → `AGENTS.md`；学习方法与验收 → `SKILL.md`。
> 本文档按 v1 基线管理，目标完成后可在"扩展章节"记录二次开发需求，不打断当前目标链。

---

## 1. 项目概述

| 项目 | 内容 |
|---|---|
| 项目名 | `dong_dong`（独立于 muduo 的新网络库） |
| 现状 | 学习者仅有 socket 编程基础 |
| 最终目标 | ① 吃透网络编程核心（Reactor / IO 多路复用 / 定时器 / 多线程）<br>② 写一个跨平台（Windows + Linux）的网络库<br>③ 在此骨架上二次开发（协议、连接池、性能优化等） |
| 参考物 | `muduo/` **不参与构建，仅作学习参考**，随时对照 |
| 开发环境 | Windows（MSVC / CMake），代码保持双平台可移植 |

**定位**：不是"抄 muduo"，而是"从裸 socket 重新发明一遍网络库"。
每引入一个概念，都必须能回答：**它是为了解决哪个具体问题而生的？**

---

## 2. 核心路线：问题链 → 概念链（全图）

| 目标 | 当前能做到 | 遇到的问题（痛点） | 引入的概念/方案 | 主要文件 |
|---|---|---|---|---|
| 00 | 阻塞 socket 单对单 echo | 一次只能服务一个客户端 | 裸 socket 全流程 | `step00/` |
| 01 | 每连接一线程 | 连接多 → 线程爆炸（C10K）、切换开销 | 感受瓶颈（不引入新方案） | `step01/` |
| 02 | 非阻塞 + 轮询所有 fd | 忙等、空转浪费 CPU | 感受忙等 | `step02/` |
| 03 | select 多路复用 | 一次等所有 fd，内核判断就绪 | **select（跨平台原语）** | `step03/` |
| 04 | select 裸写太乱 | fd 就绪后代码散落、难扩展 | **Channel + Poller + EventLoop（Reactor 雏形）** | `step04/` |
| 05 | accept 阻塞卡住循环 | 监听/读写混在一起阻塞 | 非阻塞 accept + 统一事件分发 | `step05/` |
| 06 | 一次 recv 一包 | TCP 字节流：**粘包 / 半包** | **Buffer 环形缓冲** | `step06/` |
| 07 | send 发不完/阻塞 | 写缓冲区满、写事件丢失 | **输出 Buffer + 写事件 + 高水位** | `step07/` |
| 08 | 业务线程想塞任务进循环 | 跨线程唤醒事件循环 | **任务队列 + 唤醒机制（socketpair，跨平台）** | `step08/` |
| 09 | 需要心跳/超时 | 定时任务融入事件循环 | **Timer + TimerQueue** | `step09/` |
| 10 | 连接状态散乱、内存泄漏 | 连接建立/断开/出错无状态管理 | **TcpConnection 状态机 + shared_ptr 生命周期** | `step10/` |
| 11 | 监听与业务耦合 | 每加功能改一堆 | **Acceptor + TcpServer** 组装 | `step11/` |
| 12 | 单线程多核闲置 | 一个 EventLoop 扛所有连接 | **one loop per thread** + 线程池 | `step12/` |
| 13 | select 有上限、O(n) 扫描 | Linux 高性能服务器需求 | **epoll（Linux 后端）+ WSAEventSelect（Windows 后端）** | `step13/` |
| 14 | 沉淀 | 把成果整理成**自己的跨平台库** | 命名/目录/接口打磨 | `step14_final/` |

**内在逻辑（务必理解）**：
- 目标 03 就选 select，因为它**跨平台**，从第一天就在写跨平台代码。
- epoll 直到目标 13 才出现，作为"select 性能不够"的答案 —— 这样学 epoll 时天然理解"为什么要有 epoll"。
- 也顺带看懂：**muduo 为什么只兼容 Linux**（它跳过 select，直接上 epoll + eventfd + timerfd，全是 Linux 专属）。

---

## 3. 项目结构

```
D:\C++\3partylib\muduo_cpp20\
├── CMakeLists.txt              # 只 add_subdirectory(dong_dong)
├── AGENTS.md                   # 协作规范（agent 执行）
├── SKILL.md                    # 学习方法与验收
├── dong_dong_network_requires_v1.md  # 本文件（指挥官）
├── muduo\                      # 参考代码，不参与构建，不改动
├── test\                       # 参考测试，不参与构建
└── dong_dong\                  # ★ 本项目
    ├── CMakeLists.txt          # 一个工程，多个可执行 target
    ├── include\dong_dong\      # 对外头文件
    │   └── Platform.h          # 跨平台 socket 包含 + WSAStartup 封装
    └── source\                 # 实现与各目标的可运行程序
        ├── step00\ ~ step14\   # 各阶段独立子目录（可独立编译运行）
        └── ... 逐步演进
```

- 根 CMakeLists 只包含 `dong_dong`；muduo/spdlog/test 移出构建，仅留作参考。
- 每个目标独立子目录，**独立编译、独立运行**。
- 目标 13 的 Poller 双实现版，即等价于"你改造成跨平台的 muduo"。

---

## 4. 目标清单（一目标 = 一次提交 = 一个 PR）

> 进度以 Git 提交 + GitHub PR 记录，仓库：`https://github.com/huangfanyi8/dong_dong_network.git`
> 流程：完成目标 → 本地验证 → 反问通过 → 提交（一次一个目标）→ 推远端 → 开 PR。
> 每个目标包含：目标、动作、验证、反问清单。

### 目标 1：项目骨架 + 认识参考代码
- **目标**：搭建 `dong_dong` 目录与 CMake 骨架；`cmake` 在 Windows 上能生成工程。
- **动作**：
  1. 修改根 CMakeLists：只 `add_subdirectory(dong_dong)`。
  2. 新建 `dong_dong/`：`CMakeLists.txt`、`include/dong_dong/Platform.h`、`source/`。
  3. 浏览 muduo 参考目录，标出 12 个 Linux 专属头文件（这就是要消灭的清单）。
  4. 讲解：WSAStartup 是什么、为什么 Windows 必须调它。
- **验证**：`cmake -B build_dong && cmake --build build_dong` 生成成功（即使 target 还空）。
- **反问清单**：
  - 用你自己的话解释：阻塞 socket 编程里 `bind/listen/accept/recv/send` 各自干什么？
  - muduo 里哪 3 样机制是 Linux 专属、Windows 没有的？
  - 为什么第一步就引入 Platform.h 而不是等跨平台时再说？

### 目标 2：阻塞式单对单 echo（纯裸 socket）
- **目标**：`server` 阻塞监听 1 个连接，`client` 发消息，server 回显。
- **动作**：裸系统调用写 `server.cpp` / `client.cpp`，**不封装**。
- **验证**：双终端跑通回显。
- **反问清单**：
  - `accept` 阻塞时，第二个客户端连进来会发生什么？为什么？
  - `recv` 返回 0 和返回负数分别意味着什么？
  - 阻塞模式下 server 处理慢，会不会拖垮接收？

### 目标 3：每连接一线程（感受 C10K）
- **目标**：改造 server，每个连接一个线程处理。
- **动作**：用 `std::thread` 实现；客户端循环测试多连接。
- **验证**：多客户端能同时连上并各自回显；观察线程数量与内存。
- **反问清单**：
  - "每连接一线程"的瓶颈在哪三方面（数量/切换/资源）？
  - 10000 个连接要多少线程？为什么不可行？

### 目标 4：非阻塞 + 轮询（感受忙等）
- **目标**：把 socket 设成非阻塞，主线程循环遍历所有连接。
- **动作**：`ioctlsocket`/`fcntl` 设非阻塞；写轮询循环。
- **验证**：能同时处理多连接，但观察 CPU 占用（忙等空转）。
- **反问清单**：
  - 非阻塞模式和阻塞模式的核心区别是什么？
  - 轮询为什么浪费 CPU？时间复杂度是多少？
  - 需求：能不能"一次等待所有 fd，内核告诉我谁就绪"？

### 目标 5：select 多路复用
- **目标**：用 `select` 重写 server，一次等所有 fd。
- **动作**：构建 fd_set，处理 `select` 返回值与就绪 fd。
- **验证**：多连接 + 低 CPU；对比目标 04 的忙等。
- **反问清单**：
  - select 相比轮询解决了什么、还有什么不足（1024 上限、每次重建 fd_set、O(n) 扫描）？
  - select 为什么是跨平台的？（Windows 也有）

### 目标 6：Reactor 雏形（Channel + Poller + EventLoop）
- **目标**：把 select 裸写封装成 `Channel` / `Poller` / `EventLoop` 三类。
- **动作**：Poller 封装 select；Channel 封装 fd+事件+回调；EventLoop 跑 `poll→分发→回调` 循环。
- **验证**：echo 功能不变，但代码结构清晰、可扩展。
- **反问清单**：
  - Channel 解决了什么问题？（fd 与业务解耦）
  - Poller 抽象基类为什么是跨平台的"缝"？
  - `EventLoop::loop()` 的循环三步是什么？画出流程图。

### 目标 7：非阻塞 accept + 统一事件分发
- **目标**：accept 也纳入事件循环，监听与读写统一分发。
- **动作**：监听 fd 注册读事件；有连接就绪才 accept。
- **验证**：新连接到达时回调触发，循环不被阻塞。
- **反问清单**：
  - accept 为什么也必须非阻塞？
  - 如果监听 fd 不注册进 poller，会发生什么？

### 目标 8：Buffer（解决粘包/半包）
- **目标**：实现环形 `Buffer`，替换裸 `recv` 直接消费。
- **动作**：读数据进 Buffer，提供 `peek/retrieve/append/readFd`。
- **验证**：连发多条消息 / 分多次发一条，验证收包完整性。
- **反问清单**：
  - 为什么 TCP 是字节流，会有粘包和半包？
  - Buffer 的 `prependable / readable / writable` 三个区各干什么？
  - 为什么要用 `readv` 一次读两处（buffer+extrabuf）？

### 目标 9：输出 Buffer + 写事件
- **目标**：send 发不完时存进输出 Buffer，注册写事件继续发。
- **动作**：发送逻辑改造：直接写 / 写不完进 Buffer / 就绪后继续。
- **验证**：大块数据 + 慢速对端，验证无丢失、无阻塞。
- **反问清单**：
  - 为什么直接 `send` 可能发不完？
  - 写事件（EPOLLOUT）什么时候注册、什么时候注销？
  - 高水位回调是解决什么的？

### 目标 10：跨线程唤醒（任务队列）
- **目标**：别的线程能安全地把任务塞进 EventLoop，并唤醒它。
- **动作**：`mutex + queue` 存任务；用 socketpair 唤醒阻塞的 `select`。
- **验证**：业务线程调 `runInLoop`，事件循环线程立即执行。
- **反问清单**：
  - 为什么单线程事件循环里不能直接跨线程执行回调？
  - 为什么必须"唤醒"？select 阻塞时怎么被叫醒？
  - socketpair 为什么能跨平台（Windows 可用 TCP 本地对）？

### 目标 11：Timer + TimerQueue
- **目标**：定时任务融入事件循环。
- **动作**：`Timer`（时刻+间隔+回调）、`TimerQueue`（set 排序 + 借 select 超时触发）。
- **验证**：`runAfter` / `runEvery` 回调按时触发，可取消。
- **反问清单**：
  - 定时器为什么能借 select 的 timeout 实现？
  - 到期时间如何排序、如何精确取出过期定时器？

### 目标 12：TcpConnection 状态机
- **目标**：封装单条连接：状态机 + shared_ptr 生命周期。
- **动作**：连接建立/读/写/关闭/出错回调；`enable_shared_from_this` 防止悬垂。
- **验证**：断连、对端关闭、并发关闭都不崩溃、无泄漏。
- **反问清单**：
  - 连接有哪些状态？状态迁移在哪些时机发生？
  - 为什么用 shared_ptr 管理连接？`enable_shared_from_this` 解决什么问题？

### 目标 13：Acceptor + TcpServer 组装
- **目标**：监听与业务分离，形成完整服务器框架。
- **动作**：`Acceptor` 管 listen/accept；`TcpServer` 管理所有连接与回调。
- **验证**：一个能 echo 的完整 server，代码分层清晰。
- **反问清单**：
  - Acceptor 和 TcpConnection 为什么必须分开？
  - TcpServer 怎么管理连接集合、怎么在关闭时安全移除？

### 目标 14：one loop per thread
- **目标**：多 Reactor 线程模型 + 线程池。
- **动作**：`EventLoopThread`（一线程一循环）、`EventLoopThreadPool`（负载均衡取 loop）。
- **验证**：多连接分散到多个线程，多核跑满。
- **反问清单**：
  - 为什么一个 EventLoop 扛不住高并发？
  - 新连接怎么分发到某个 IO 线程？（跨线程唤醒登场）

### 目标 15：双后端（epoll + WSAEventSelect）
- **目标**：Poller 接口下实现 Linux epoll 与 Windows WSAEventSelect 两个后端。
- **动作**：Poller 变为抽象接口；两个实现类；`EventLoop` 按平台创建。
- **验证**：**同一份 Server 代码在 Windows 和 Linux 上都能编译运行**。
- **反问清单**：
  - epoll 比 select 强在哪？（O(1)、无上限、只返回就绪 fd）
  - 为什么目标 03 不直接学 epoll？现在学它和那时学有什么本质不同？
  - WSAEventSelect 和 epoll 的事件模型差异是什么？

### 目标 16：沉淀为最终跨平台库
- **目标**：把成果整理成规范的库：命名、目录、公共接口、示例。
- **动作**：整理 `include/dong_dong` 对外 API；写简单使用示例；补齐 Platform.h。
- **验证**：一个全新的小程序只需 include + 链接即可用你的库。
- **反问清单**：
  - 从头回忆整条"问题→方案"链，能否一口气讲完？
  - 你的库和 muduo 相比，哪里相同、哪里不同、哪里更好？
  - 二次开发想加的第一个功能是什么？（下一步选题）

---

## 5. 里程碑

| 里程碑 | 达成标准 | 对应目标 |
|---|---|---|
| M1 | 跑通裸 socket 单对单 echo | 目标 2 |
| M2 | 理解 select + 感受 C10K/忙等 | 目标 5 |
| M3 | Reactor 核心闭环（Channel/Poller/EventLoop） | 目标 6 |
| M4 | 完整 Buffer + 写事件，无丢包 | 目标 9 |
| M5 | 跨线程唤醒 + 定时器 | 目标 11 |
| M6 | 完整服务器框架（Acceptor/TcpServer） | 目标 13 |
| M7 | **跨平台跑通（Win + Linux 同一份代码）** | 目标 15 |
| M8 | 整理为个人跨平台网络库，开始二次开发选题 | 目标 16 |

---

## 6. 进度记录

> 每完成一个目标，在此追加一行（含 PR 链接），作为项目进度追踪。

- [x] 目标 0（前置）：项目开发方案与仓库初始化 —— commit `dde0199`，PR 待补
- [ ] 目标 1：项目骨架 + 认识参考代码
- [ ] 目标 2 ~ 16：待完成

---

## 7. 扩展需求（二次开发方向，记录不打断当前目标链）

> 学习者在学习过程中提出的二次开发想法，只在此记录，不在当前目标链中实现。

（暂无）

---

## 8. 版本与变更

| 版本 | 说明 |
|---|---|
| v1 | 初始基线：16 个目标，从裸 socket 到跨平台双后端，最后沉淀为个人网络库 |
