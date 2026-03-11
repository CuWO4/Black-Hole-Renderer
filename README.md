# 黑洞渲染器

参考:
> 从零开始搓一个黑洞----glsl编程实战
> <https://zhuanlan.zhihu.com/p/20536269771>
> <https://www.shadertoy.com/view/4XcfR2>
> Baopinsui

---

## 简介

基于相对论的 light-casting 渲染器, 为了视觉效果引入了很多非物理的参数和机制. 

当前这个分支已经改成了一个专用的视频渲染器: 

- 只渲染这一套黑洞场景
- 固定使用 `camera0`
- 相机绕 z 轴环绕黑洞一周
- 输出 H.265 编码的 MP4
- 视频分辨率, 帧率, 时长等参数直接硬编码在 `include/constant.h`

## 效果

![long shot](doc/long-shot.png)
![close shot](doc/close-shot.png)

## 编译 & 使用

### 依赖

- `clang++`
- `make`
- `ffmpeg`(需要带 `libx265` 编码支持)

当前开发环境默认在 Linux 下运行. 

### 构建

```bash
make
```

### 运行

```bash
./debug/main.exe -o output.mp4
```

程序只接受一个输出参数: 

- `-o/--out PATH`: 输出 MP4 路径

例如: 

```bash
./debug/main.exe -o debug/final.mp4
```

渲染过程中会逐帧输出进度.

### 当前视频配置

当前视频参数定义在 `include/constant.h` 的 `constant::video` 中. 

当前分支默认配置为: 

- `1920x1080`
- `30 FPS`
- `20s`
- `H.265 / MP4`
- `CRF 14`

如果要改最终成片参数, 直接修改 `include/constant.h` 即可. 

## 性能

- 渲染: 多线程 tile job(默认 50x50)动态领取, 避免按大块切分导致负载不均. 
- 后处理: Bloom 高斯模糊多线程 job 并行. 

## TODOs

- [x] 严格遵守相对论效应
- [x] 并行计算提升性能(渲染 + Bloom)
