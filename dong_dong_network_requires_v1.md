# dong_dong_network_requires_v1 —— 项目需求文档（指挥官）

> 本文件是项目的**需求基线**：定义项目要做什么、每个目标完成到什么程度。
> 项目工程规则由 `AGENTS.md` 定义；学习方法与理解验收由 `.opencode/skills/dong-dong-learning/SKILL.md` 定义。
> 本文件按 v1 基线管理。二次开发方向记录在“扩展需求”中，不自动打断当前目标链。

---

## 1. 项目概述

| 项目 | 内容 |
|---|---|
| 项目名 | `dong_dong`（独立于 muduo 的新网络库） |
| 当前基础 | 具备 socket 编程基础 |
| 最终目标 | ① 吃透网络编程核心（Reactor / IO 多路复用 / 定时器 / 多线程）<br>② 写一个跨平台（Windows + Linux）的网络库<br>③ 在此骨架上进行二次开发（协议、连接池、性能优化等） |
| 参考物 | `muduo/` **不参与构建，仅作学习参考** |
| 开发环境 | Windows（MSVC / CMake）为当前主要开发环境，同时保持 Linux 可移植性 |

### 项目定位

这不是“抄 muduo”，而是从裸 socket 逐步重新设计一套自己的网络库。

学习路线必须保留清晰的问题链：

> 当前实现能做什么 → 遇到什么痛点 → 为什么现有方案不够 → 引入什么机制 → 新机制解决什么问题 → 新机制又有什么局限

每个重要概念都应该能够回答：**它解决了哪个具体问题？**

---

## 2. 路线总览：问题链 → 概念链

| 目标 | 当前能力 | 当前痛点 | 引入的概念 / 方案 | 主要目录 |
|---|---|---|---|---|
| 01 | 尚无工程骨架 | 缺少跨平台基础与工程骨架，无法在 Windows 构建 | CMake、`Platform.h`、WSAStartup | `step01/` |
| 02 | 阻塞 socket 单对单 echo | 一次只能服务一个客户端 | 裸 socket 全流程 | `step02/` |
| 03 | 每连接一线程 | 连接多后线程数量、切换和资源开销增长 | `std::thread`，感受 C10K | `step03/` |
| 04 | 非阻塞 + 轮询所有连接 | 忙等、空转浪费 CPU | 非阻塞 socket + 轮询 | `step04/` |
| 05 | select 多路复用 | 轮询浪费 CPU | `select`（跨平台多路复用原语） | `step05/` |
| 06 | select 基础上的事件循环雏形 | fd、事件和业务逻辑散落，扩展困难 | Channel + Poller + EventLoop（Reactor 雏形） | `step06/` |
| 07 | 统一监听与读写事件 | accept 仍可能阻塞事件循环 | 非阻塞 accept + 统一事件分发 | `step07/` |
| 08 | 输入数据缓冲 | TCP 字节流导致消息边界不可靠 | Buffer | `step08/` |
| 09 | 输出数据缓冲 | send 可能一次发不完，直接阻塞或丢失后续发送机会 | 输出 Buffer + 写事件 + 高水位机制 | `step09/` |
| 10 | 跨线程向 EventLoop 提交任务 | EventLoop 阻塞等待时无法被其他线程及时唤醒 | 任务队列 + Wakeup Channel | `step10/` |
| 11 | EventLoop 中的定时任务 | 心跳、超时等定时任务无法自然融入事件循环 | Timer + TimerQueue | `step11/` |
| 12 | 单条连接的统一生命周期 | 状态散乱、连接销毁与回调生命周期复杂 | TcpConnection 状态机 + `shared_ptr` 生命周期管理 | `step12/` |
| 13 | 完整服务器组装 | 监听、连接和业务逻辑耦合 | Acceptor + TcpServer | `step13/` |
| 14 | 多 Reactor 线程 | 单 EventLoop 无法充分利用多核 | one loop per thread + 线程池 | `step14/` |
| 15 | 双平台事件后端 | select 的扩展性有限，平台后端需要隔离 | Poller 抽象 + Linux `epoll` + Windows `WSAEventSelect` | `step15/` |
| 16 | 最终跨平台库 | 前面各阶段需要整理为可复用产品形态 | 公共 API、目录、示例、平台封装 | `step16_final/` |

### 路线设计意图

目标 05 使用 `select`，原因是它可以让学习者在 Windows 环境先理解跨平台 IO 多路复用。

目标 15 再引入 Linux `epoll` 和 Windows `WSAEventSelect`，重点不只是“换一个 API”，而是理解：

- 上层 `EventLoop` 为什么不应该依赖具体系统 API；
- 平台后端为什么需要隔离；
- 不同事件模型在接口和语义上有哪些差异。

---

## 3. 目标清单

> 一个目标的“功能完成”由本节中的目标、动作和验证定义。
> Git 提交、push、PR 不属于需求完成条件。

### 目标 01：项目骨架 + 认识参考代码

**目标**：建立 `dong_dong` 目录与 CMake 骨架，并确认 Windows 构建环境可用。

**动作**：

1. 修改根 `CMakeLists.txt`，只 `add_subdirectory(dong_dong)`。
2. 引入 `spdlog` 作为日志库参与构建。
3. 创建 `dong_dong/`：`CMakeLists.txt`、`include/dong_dong/Platform.h`、`source/`。
4. 浏览 muduo 参考目录，识别与 Linux 平台强相关的接口/头文件。
5. 理解 WSAStartup：它解决什么问题，为什么 Windows socket 生命周期与 Unix socket 初始化不同。

**验证**：

```bash
cmake -B build
cmake --build build
```

构建生成成功即可。

**理解重点**：

- 阻塞 socket 基本流程；
- Windows socket 初始化（WSAStartup）；
- 为什么要从第一阶段就建立平台封装。

> 认知预告（不设关卡）：muduo 参考代码里的 epoll/eventfd/timerfd 是 Linux 专属机制，属于目标 05/15 的学习内容，现在只需知道"它们存在"。

---

### 目标 02：阻塞式单对单 echo（纯裸 socket）

**目标**：实现阻塞监听 1 个连接的 echo server/client。

**动作**：裸系统调用实现 `server.cpp` / `client.cpp`，暂不引入网络库抽象。

**验证**：双终端运行，client 发送消息后 server 回显。

**理解重点**：

- `bind/listen/accept/recv/send` 各自做什么；
- `accept` 和 `recv` 的阻塞行为；
- `recv == 0` 与错误返回的区别。

---

### 目标 03：每连接一线程（感受 C10K）

**目标**：改造 server，使每个连接由一个 `std::thread` 处理。

**动作**：

- 每连接创建线程；
- 多客户端循环连接测试；
- 观察线程数量、资源开销和可扩展性。

**验证**：多个客户端可以并行 echo，并能够观察线程规模增长。

**理解重点**：线程数量、上下文切换和资源占用为什么限制这种模型扩展到大量连接。

---

### 目标 04：非阻塞 + 轮询（感受忙等）

**目标**：使用非阻塞 socket，主线程循环检查多个连接。

**动作**：Windows 使用 `ioctlsocket`，Linux 使用 `fcntl`；实现轮询循环。

**验证**：多个连接可以同时处理，并观察空闲情况下 CPU 占用。

**理解重点**：

- 阻塞与非阻塞的区别；
- 为什么轮询导致忙等；
- 为什么需要“让内核一次等待多个 fd”。

---

### 目标 05：select 多路复用

**目标**：使用 `select` 代替主动轮询。

**动作**：

- 管理 `fd_set`；
- 等待读事件；
- 处理监听 socket 与连接 socket。

**验证**：多客户端正常工作，空闲时 CPU 占用明显低于目标 04。

**理解重点**：

- `select` 解决了忙等什么问题；
- 为什么仍有 fd 集合管理与扫描成本；
- 为什么它适合作为跨平台学习入口。

---

### 目标 06：Reactor 雏形（Channel + Poller + EventLoop）

**目标**：把目标 05 的 select 代码拆出最小事件循环结构。

**动作**：

- `Poller`：负责等待 IO 事件；
- `Channel`：关联 socket、关注事件和回调；
- `EventLoop`：组织等待、分发和执行回调。

**验证**：echo 功能保持不变，代码结构更清晰，并能够新增事件处理而不修改大量业务代码。

**理解重点**：

- 为什么 fd 和业务回调应该解耦；
- `Poller` 为什么是跨平台实现的边界；
- `EventLoop::loop()` 的基本循环。

---

### 目标 07：非阻塞 accept + 统一事件分发

**目标**：让监听 socket 也纳入统一事件循环。

**动作**：监听 fd 注册读事件；仅在就绪时执行 accept；新连接设置为合适的 socket 模式。

**验证**：没有新连接时 EventLoop 不会卡在 accept；新连接到达后正常创建连接处理逻辑。

**理解重点**：为什么 accept 也必须纳入事件驱动模型，以及监听 socket 与连接 socket 的共同点和区别。

---

### 目标 08：Buffer（解决消息边界问题）

**目标**：引入可增长的字节缓冲区，让接收数据与业务消息处理解耦。

**动作**：实现类似：

- `peek`
- `retrieve`
- `append`
- `readFd`

并将 socket 读取的数据先进入 Buffer，再由业务层消费。

**验证**：

- 连续发送多条消息；
- 分多次发送一条逻辑消息；
- 业务层能够正确按照协议边界取出消息。

**理解重点**：TCP 是字节流，`recv` 返回次数不代表业务消息边界。

> 本阶段的 Buffer 不预设为“环形 Buffer”。具体内部数据结构由实现需要决定，重点是理解 readable / writable / prependable 等区域以及缓冲与协议解析之间的关系。

---

### 目标 09：输出 Buffer + 写事件

**目标**：处理一次 `send` 无法完成全部数据的情况。

**动作**：

- 可以直接发送的数据立即发送；
- 剩余数据进入输出 Buffer；
- 在需要时注册写事件；
- 写完后及时取消写事件。

**验证**：大数据发送、慢速对端等场景下无数据丢失，并且不会因为单次 send 未完成而阻塞 EventLoop。

**理解重点**：

- 为什么 `send` 不保证一次发送全部数据；
- 写事件什么时候注册和注销；
- 高水位事件解决什么问题。

---

### 目标 10：跨线程唤醒（任务队列）

**目标**：让其他线程可以安全提交任务并唤醒 EventLoop。

**动作**：

- `mutex + queue` 管理待执行任务；
- 实现 `runInLoop` / `queueInLoop` 一类机制；
- 使用跨平台 Wakeup Channel 唤醒阻塞中的 IO 等待。

Linux 可以使用 `socketpair` 等机制；Windows 使用等价的本地 TCP 通道等方式实现。

**验证**：业务线程提交任务后，EventLoop 能够被立即唤醒并执行任务。

**理解重点**：

- 为什么不能随意从其他线程直接修改 EventLoop 数据；
- 为什么任务队列还不够，必须有唤醒机制；
- 为什么 Wakeup Channel 应该成为平台相关实现点。

---

### 目标 11：Timer + TimerQueue

**目标**：将定时任务融入 EventLoop。

**动作**：

- `Timer`：记录到期时间、间隔和回调；
- `TimerQueue`：管理待执行定时器；
- 使用当前 IO 等待机制的 timeout 触发定时器检查；
- 支持一次性和周期性任务，以及取消。

**验证**：`runAfter` / `runEvery` 类功能能够按预期触发和取消。

**理解重点**：定时器如何与 IO 等待协同，以及如何管理到期任务。

---

### 目标 12：TcpConnection 状态机

**目标**：封装单条连接的生命周期。

**动作**：

- 建立；
- 可读；
- 可写；
- 关闭；
- 错误；
- 连接状态迁移；
- 生命周期管理。

可使用 `shared_ptr` 与 `enable_shared_from_this`，但必须明确它们分别解决什么生命周期问题，不能为了模仿 muduo 而机械引入。

**验证**：

- 对端主动关闭；
- 本端主动关闭；
- 读写错误；
- 关闭流程多次触发。

要求不崩溃、无明显资源泄漏、状态迁移明确。

---

### 目标 13：Acceptor + TcpServer 组装

**目标**：将监听与业务连接管理分离。

**动作**：

- `Acceptor` 管理监听 socket 和 accept；
- `TcpServer` 管理连接集合、回调和服务器生命周期。

**验证**：形成一个完整 echo server，监听、连接和业务层职责清晰。

**理解重点**：为什么监听连接和单条业务连接应该分离，以及服务器如何安全管理连接集合。

---

### 目标 14：one loop per thread

**目标**：引入多 Reactor 线程模型。

**动作**：

- `EventLoopThread`；
- `EventLoopThreadPool`；
- 新连接在多个 IO EventLoop 之间分配；
- 使用目标 10 的跨线程唤醒机制切换到目标 IO 线程。

**验证**：多个连接能够分布到多个 IO 线程，并能够观察线程之间的负载分布。

**理解重点**：为什么一个 EventLoop 不能无限扩展，以及新连接如何分发到 IO 线程。

---

### 目标 15：双后端（epoll + WSAEventSelect）

**目标**：建立 Poller 抽象，并分别实现 Linux 与 Windows 的事件等待后端。

**动作**：

- `Poller` 抽象接口；
- Linux `epoll` 实现；
- Windows `WSAEventSelect` 实现；
- `EventLoop` 不依赖具体平台 API；
- 按平台选择具体 Poller。

**验证**：上层 Server / TcpConnection 等代码保持基本一致，并分别在 Windows 与 Linux 上编译运行。

**理解重点**：

- 为什么具体系统 API 应该隐藏在 Poller 层；
- epoll 与 select 在事件注册与就绪事件处理方式上的差异；
- WSAEventSelect 与 epoll 的事件模型差异；
- 为什么平台无关的上层设计比直接复制某个平台实现更重要。

---

### 目标 16：沉淀为最终跨平台库

**目标**：将前面阶段的代码整理为可独立使用的网络库。

**动作**：

- 稳定公共 API；
- 整理 `include/dong_dong/`；
- 收敛内部实现目录；
- 补齐 Platform 相关封装；
- 提供最小使用示例；
- 清理仅用于教学的临时代码。

**验证**：创建一个新的小程序，只通过 include + 链接即可使用最终库完成基本网络功能，并分别验证 Windows / Linux。

**理解重点**：

- 从裸 socket 到最终库的完整问题链；
- 哪些设计与 muduo 相似，哪些不同；
- 哪些部分是为了学习，哪些部分适合作为真实库长期保留。

---

## 4. 里程碑

| 里程碑 | 达成标准 | 对应目标 |
|---|---|---|
| M1 | 跑通裸 socket 单对单 echo | 目标 02 |
| M2 | 依次感受线程模型（03）、忙等（04）、select（05）三者差异 | 目标 05 |
| M3 | Reactor 核心闭环（Channel / Poller / EventLoop） | 目标 06 |
| M4 | 完整输入 / 输出 Buffer 与写事件 | 目标 09 |
| M5 | 跨线程唤醒 + 定时器 | 目标 11 |
| M6 | 完整服务器框架（Acceptor / TcpServer） | 目标 13 |
| M7 | 多 Reactor 线程模型 | 目标 14 |
| M8 | 双平台 Poller 后端跑通 | 目标 15 |
| M9 | 整理为个人跨平台网络库，开始二次开发选题 | 目标 16 |

---

## 5. 进度记录

> 仅记录项目进度，不定义 Git 操作流程。

- [x] 前置：项目开发方案与仓库初始化 —— commit `dde0199`
- [x] 前置：拆分协作文档为 requires / SKILL / AGENTS —— commit `2280214`
- [x] 前置：按 opencode 规范重构协作文档 —— commit `2911402`
- [x] 前置：需求文档只保留学习路线与目标，spdlog 参与编译 —— commit `a39e566`
- [x] 目标 01：项目骨架 + 认识参考代码
- [ ] 目标 02：阻塞式单对单 echo
- [ ] 目标 03：每连接一线程
- [ ] 目标 04：非阻塞 + 轮询
- [ ] 目标 05：select 多路复用
- [ ] 目标 06：Reactor 雏形
- [ ] 目标 07：非阻塞 accept + 统一事件分发
- [ ] 目标 08：Buffer
- [ ] 目标 09：输出 Buffer + 写事件
- [ ] 目标 10：跨线程唤醒
- [ ] 目标 11：Timer + TimerQueue
- [ ] 目标 12：TcpConnection 状态机
- [ ] 目标 13：Acceptor + TcpServer
- [ ] 目标 14：one loop per thread
- [ ] 目标 15：双后端
- [ ] 目标 16：最终跨平台库

---

## 6. 扩展需求

学习过程中提出但不属于当前目标链的功能，只记录在此，不自动实现。

当前暂无。

---

## 7. 版本与变更

| 版本 | 说明 |
|---|---|
| v1 | 初始基线：16 个目标，从裸 socket 到跨平台双后端，最后沉淀为个人网络库 |

后续修改应明确记录：

- 修改了哪个目标；
- 修改原因；
- 是否改变学习路线；
- 是否影响已有验证标准。
