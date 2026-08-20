# AGENTS.md —— dong_dong 网络库项目规则（opencode 自动加载）

本文件是 opencode 的项目指令：随会话自动载入，聚焦 agent 在本仓库工作必须知道的事情。
详细需求与目标 → 按需读取 `dong_dong_network_requires_v1.md`（指挥官，需求唯一来源）。
学习方法与验收标准 → 按需加载技能 `dong-dong-learning`。

## 项目

把 muduo 从裸 socket 一步步重写为**属于自己的跨平台网络库**（Windows + Linux），同时系统学习网络编程。学习者仅有 socket 基础：**先讲"痛"，再给名词，不堆术语**。

## 构建与验证

- 根 `CMakeLists.txt` 只 `add_subdirectory(dong_dong)`；muduo/spdlog/test 不参与构建、禁止修改，仅作参考。
- 项目代码只写于 `dong_dong/`：`include/dong_dong/` 对外头文件，`source/` 实现与可运行程序。
- 每个目标一个独立子目录（`dong_dong/source/step0X/`），独立编译、独立运行。
- 验证命令：`cmake -B build_dong && cmake --build build_dong`。

## git 规范（一目标一提交一 PR）

- 提交切分点 = **目标完成且反问通过**，不是时间。
- 一次提交只含一个目标；消息格式：`目标N: <简短动词短语>`。
- 每个目标：`git add` 相关文件 → `git commit` → `git push` 到远端 `main` → 开 PR。
- 远端：`https://github.com/huangfanyi8/dong_dong_network.git`。

## 协作流程（每个目标的执行步骤）

```
① 目标回顾   —— 从 requires v1 读取当前目标，讲清"要解决什么痛"
② 痛点讲解   —— 大白话 + 可配图；不预讲下一个概念
③ 读参考     —— 引导学习者读 muduo 对应文件，标出关键行
④ 写代码     —— 学习者先尝试，agent 补充/修正，逐行解释平台差异
⑤ 验证       —— 编译 + 运行 demo，核对 requires v1 的"验证"条款
⑥ 反问       —— 按技能 dong-dong-learning 的反问细则提问
⑦ 判定       —— 通过→提交；不通过→回到④/②重走，不开新目标
⑧ 提交       —— git 一次提交 + 推远端 + 开 PR
```

## 外部文件加载

- 开始任何目标前：用 Read 读取 `dong_dong_network_requires_v1.md` 的当前目标章节，以它为唯一需求来源。
- 进行反问/验收时：加载技能 `dong-dong-learning`。
- 不必预载全部内容，按需读取。
