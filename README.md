# 黑洞渲染器

参考:
> 从零开始搓一个黑洞——glsl编程实战
> <https://zhuanlan.zhihu.com/p/20536269771>
> <https://www.shadertoy.com/view/4XcfR2>
> Baopinsui

---

## 简介

基于相对论的 light-casting 渲染器, 为了视觉效果引入了很多非物理的参数和机制.

## 效果

![long shot](doc/long-shot.png)
![close shot](doc/close-shot.png)

## 编译 & 使用

```bash
make
make run
```

默认会在项目目录生成 `test.png`。

也可以直接运行可执行文件并传参：

```bash
./debug/main.exe --ext jpg -o foo.jpg
./debug/main.exe -o foo.png
./debug/main.exe --conf my.conf -o out.png
```

参数说明：

- `--ext png|jpg|bmp|ppm`：输出格式（默认 `png`，支持 `.png` / `jpeg` 之类输入）
- `-o/--out PATH`：输出路径；如果未显式指定 `--ext`，会从 `PATH` 后缀自动推断
- `--conf CONF`：保留参数，当前忽略；若传入会在 stderr 提示 reserved

## 性能

- 渲染：多线程 tile job（默认 50x50）动态领取，避免按大块切分导致负载不均。
- 后处理：Bloom 高斯模糊多线程 job 并行。
- 进度条：渲染与 Bloom 都以 job 数为总任务数更新。

## TODOs

- [x] 命令行参数解析（`--ext/-o/--conf`）
- [ ] 动态参数加载（`--conf` 预留接口）
- [x] 严格遵守相对论效应
- [x] 并行计算提升性能（渲染 + Bloom）
