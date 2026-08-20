# dong_dong 跨平台网络库 —— 学习与开发方案

> 一个从零开始、边学边重构的项目。
> 以最简单 socket 通信为起点，用"问题驱动"的方式逐步引入新概念，
> 最终把 muduo 重写成一个**属于自己的跨平台网络库**。

---

## 1. 项目概述

| 项目 | 内容 |
|---|---|
| 项目名 | `dong_dong`（新目录，独立于 muduo） |
| 现状 | 仅有 socket 编程基础 |
| 最终目标 | ① 吃透网络编程核心（Reactor / IO 多路复用 / 定时器 / 多线程）<br>② 写一个跨平台（Windows + Linux）的网络库<br>③ 在此骨架上二次开发（协议、连接池、性能优化等） |
| 参考物 | `muduo/` 目录**不参与构建，仅作学习参考**，随时对照 |
| 开发环境 | Windows（MSVC / CMake），代码保持双平台可移植 |

**定位**：这不是"抄 muduo"，而是"从裸 socket 重新发明一遍网络库"。
每引入一个概念，都必须能回答：**它是为了解决哪个具体问题而生的？**

---

## 2. 学习方法论（三条铁律）

### 2.1 问题驱动，循序渐进
不预讲任何概念。每次只面对一个具体的"痛"：
1. 先写能跑的代码 → 2. 跑出问题 → 3. 讨论痛点 → 4. 引入解决方案（新概念）。

所有新概念都是**解决方案**，不是凭空的知识点。

### 2.2 反问模式（确保真懂）
每完成一步，必须通过"反问"才能进入下一步：
- 我（助手）提问，你**用自己的话回答**，不许背概念。
- 答不出 / 答错 → 不批评，回到对应的代码和痛点**重新走一遍**，直到能讲清"问题→方案"因果链。
- 反问通过的标准：你能**给不懂的人**讲明白这一步，且能指出 muduo 参考代码里对应的实现。

### 2.3 以目标为单位推进（一个目标 = 一次提交 + 一个 PR）
- 每个目标完成后，有一个可编译、可运行的成果，**立即提交 git 并打一个 PR**。
- 不以时间切分进度，**以"目标完成"为切分点**：完成一个，提交一个，PR 一个。
- 宁可慢，不可跳。每步结尾有明确验证动作 + 反问确认。
- 当前目标没完成 → 不提交、不开下一个目标。

---

## 3. 核心路线：问题链 → 概念链（全图）

| 步 | 当前能做到 | 遇到的问题（痛点） | 引入的概念/方案 | 主要文件 |
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
| 13 | select 有上限、O(n) 扫描 | Linux 高性能服务器需求 | **epoll（Linux 后端）+ WSAEventSelect（Windows 后端）**，Poller 接口双实现 | `step13/` |
| 14 | 沉淀 | 把成果整理成**自己的跨平台库** | 命名/目录/接口打磨 | `step14_final/` |

**内在逻辑（务必理解）**：
- 第 3 步就选 select，因为它**跨平台**，从第一天就在写跨平台代码。
- epoll 直到第 13 步才出现，作为"select 性能不够"的答案 —— 这样学 epoll 时天然理解"为什么要有 epoll"。
- 也顺带看懂：**muduo 为什么只兼容 Linux**（它跳过了 select，直接上 epoll + eventfd + timerfd，全是 Linux 专属）。

---

## 4. 项目结构规划

```
D:\C++\3partylib\muduo_cpp20\
├── CMakeLists.txt              # 只 add_subdirectory(dong_dong)
├── muduo\                      # 参考代码，不参与构建，不改动
├── test\                       # 参考测试，不参与构建
└── dong_dong\                  # ★ 本项目
    ├── CMakeLists.txt          # 一个工程，多个可执行 target
    ├── include\dong_dong\      # 对外头文件
    │   └── Platform.h          # 跨平台 socket 包含 + WSAStartup 封装
    └── source\                 # 实现与各 step 的可运行程序
        ├── server.cpp / client.cpp   # step00
        ├── step01\ ~ step14\          # 各阶段独立子目录（可独立编译运行）
        └── ... 逐步演进
```

- 根 CMakeLists 只包含 `dong_dong`；muduo/spdlog/test 全部移出构建，仅留作参考。
- 每步一个子目录，**独立编译、独立运行**，学崩不了。
- step13 的 Poller 双实现版，即等价于"你改造成跨平台的 muduo"。

---

## 5. 目标推进计划（每个目标 = 一个 PR）

> 进度以 Git 提交 + GitHub PR 记录，仓库：`https://github.com/huangfanyi8/dong_dong_network.git`
> 流程：完成目标 → 本地验证 → 反问通过 → 提交（一次一个目标）→ 推远端 → 开 PR。

### 目标 1：项目骨架 + 认识参考代码
- **目标**：搭建 `dong_dong` 目录与 CMake 骨架；`cmake` 在 Windows 上能生成工程。
- **动作**：
  1. 修改根 CMakeLists：只 `add_subdirectory(dong_dong)`。
  2. 新建 `dong_dong/`：`CMakeLists.txt`、`include/dong_dong/Platform.h`、`source/`。
  3. 浏览 muduo 参考目录，标出 12 个 Linux 专属头文件（这就是你要消灭的清单）。
  4. 讲解：WSAStartup 是什么、为什么 Windows 必须调它。
- **验证**：`cmake -B build_dong && cmake --build build_dong` 生成成功（即使 target 还空）。
- **反问清单**：
  - 用你自己的话解释：阻塞 socket 编程里 `bind/listen/accept/recv/send` 各自干什么？
  - muduo 里哪 3 样机制是 Linux 专属、Windows 没有的？
  - 为什么我们的第一步就引入 Platform.h 而不是等跨平台时再说？

### 目标 2：阻塞式单对单 echo（纯裸 socket）
- **目标**：`server` 阻塞监听 1 个连接，`client` 发消息，server 回显。
- **动作**：裸系统调用写 `server.cpp` / `client.cpp`，**不封装**。
- **验证**：双终端跑通回显。
- **反问清单**：
  - `accept` 阻塞时，第二个客户端连进来会发生什么？为什么？
  - `recv` 返回 0 和返回负数分别意味着什么？
  - 阻塞模式下 server 处理慢，会不会拖垮接收？

### 目标 3：每连接一线程（感受 C10K）
- **目标**：改造 server，每个连接 fork 一个线程处理。
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
- **验证**：多连接 + 低 CPU；对比 step02 的忙等。
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
  - 为什么第 3 步不直接学 epoll？现在学它和那时学有什么本质不同？
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

## 6. 反问模式细则

1. **时机**：每天代码验证通过后、进入次日之前。
2. **形式**：我根据当天内容提问（见每步"反问清单"），你**口头或文字用自己的话回答**。
3. **判定**：
   - 通过 → 进入下一步。
   - 答不出/含糊 → 一起回到对应代码重新走一遍，讲清痛点与方案，次日重试。
4. **加分项**：你能在 muduo 参考代码里找到这一步对应的实现并指出文件名。

> 反问不是考试，是"复述确认"。能讲出来，才算你的。

---

## 7. 目标完成流程（固定节奏）

```
① 目标回顾（当前目标要解决什么痛）     约5分钟
② 讲解痛点 + 读参考代码                约20分钟
③ 一起写代码                           约40分钟
④ 编译 + 验证 demo                     约20分钟
⑤ 反问确认                            约15分钟
⑥ 提交 git + 推远端 + 开 PR（一目标一提交）
```

---

## 8. 里程碑

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

## 9. 与 muduo 的关系约定

- `muduo/`、`test/`、`spdlog/` **不参与构建、不改动**，仅作随时查阅的参考。
- 每一步先自己写，写完再对照 muduo 看它怎么做的、为什么那样做。
- 本方案允许在 16 天后继续扩展（step15+ 进入二次开发阶段，届时再定方向：HTTP / WebSocket / 连接池 / 性能优化等）。

---

> **目标 1**：搭好 `dong_dong` 骨架 + 根 CMakeLists 只含 dong_dong，`cmake` 能生成，并讲清 WSAStartup 与 muduo 的 Linux 三件套。完成后提交 git + PR。Ready 后开工。
