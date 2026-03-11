# 黑洞视频渲染器

参考:

- 从零开始搓一个黑洞 — GLSL 编程实战
- <https://zhuanlan.zhihu.com/p/20536269771>
- <https://www.shadertoy.com/view/4XcfR2>
- Baopinsui

---

## 简介

当前仓库已经收敛成一条专用的 CUDA 视频渲染管线:

- 只渲染单一黑洞吸积盘场景
- 固定使用 `constant::camera0`
- 相机绕黑洞做环绕运动
- 光线追踪、Bloom 和 `rgb24` 打包都在 CUDA 侧完成
- 通过 `ffmpeg + hevc_nvenc` 输出 H.265 MP4

旧的 CPU 场景渲染主路径和相关接口已经移除，当前入口就是 CUDA 视频渲染。

## 效果

![long shot](doc/long-shot.png)
![close shot](doc/close-shot.png)

## 依赖

- `g++`
- `nvcc`
- `make`
- `ffmpeg`，并且需要包含 `hevc_nvenc`
- NVIDIA 驱动与 CUDA 运行时

当前开发流程默认在 Linux / WSL 下执行。

## 构建

```bash
make
```

## 运行

```bash
./debug/main_cuda -o debug/final.mp4
```

程序只接受一个参数:

- `-o` / `--out PATH`: 输出 MP4 路径

渲染过程中会逐帧打印进度和 ETA。

## 配置入口

主要参数都集中在 `include/constant.h`:

- `constant::video`: 分辨率、帧率、时长、CRF
- `constant::output`: ffmpeg / NVENC 输出参数
- `constant::camera0`: 相机轨道与镜头参数
- `constant::cuda_renderer`: tile、frame batch、TAA、Bloom、双缓冲与后处理参数
- `constant::model`: 吸积盘与噪声模型参数

## 当前渲染管线

1. CUDA kernel 直接生成每个像素/每帧的初始光线
2. CUDA 追踪黑洞场景并写入设备端批帧缓冲
3. CUDA 执行三层 Bloom
4. CUDA 直接把结果打包成 `rgb24`
5. 双缓冲异步下载到主机
6. 独立写线程把帧送给 `ffmpeg`
7. `hevc_nvenc` 完成最终编码

## 调参建议

- 想改最终成片参数，优先修改 `constant::video`
- 想改速度/吞吐，优先修改 `constant::cuda_renderer`
- 想改黑洞与吸积盘视觉效果，修改 `constant::model`
