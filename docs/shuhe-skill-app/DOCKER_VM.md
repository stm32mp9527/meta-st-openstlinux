# DOCKER_VM — STM32MP1 应用开发环境交付

> 目标：客户拿到一个“已经完整编译好 Yocto”的 Docker/虚拟机环境，
> 不需要懂交叉编译器，只需要对 AI 说需求，AI 用 `shuhe-ostl-app` 创建 app 并部署。

## 1. 推荐形态

| 形态 | 适合 | 注意 |
|------|------|------|
| 虚拟机（qcow2/OVA） | 展会/客户现场 | 体积大，但最稳；可预装桌面/终端 |
| Docker + volume | 内部/云端 | Yocto 树几十 GB，放 volume，不要打进镜像层 |
| Docker 镜像（全量） | 不推荐 | 镜像过大，构建/分发慢 |

## 2. 虚拟机建议内容

```text
/home/yocto/
├── shuhe/                    # repo sync 后的源码树
│   ├── layers/
│   ├── build-openstlinuxweston-stm32mp13-disco-test/
│   │   └── tmp-glibc/deploy/images/stm32mp13-disco-test/
│   └── meta-customer-app/    # app 模板/layer（可预置）
└── .config/opencode/skills/shuhe-ostl-app/   # stub（可选）

/opt/board/                   # 烧录工具/镜像/说明
```

- 预构建镜像：`st-image-weston`（GUI，已含 SDL2）或 `shuhe-test-image-core`
  （网络，已有 networkd+DHCP）。
- 预烧板子；现场只改 app，不重烧整包。
- 预置 `meta-customer-app` 模板和 demo app，客户可直接改源码。
- 如果预置的是 `*-test` 构建树，先确认允许加 app layer；否则单独准备 app build 目录。
- GUI kiosk 预置时禁用 `st-image-weston` 的 demo launcher，避免覆盖客户画面。

## 3. Docker 建议

### 3.1 基础镜像

```dockerfile
FROM ubuntu:22.04

# Yocto 依赖（按实际发行版调整）
RUN apt-get update && apt-get install -y \
    chrpath diffstat lz4 gawk git-core texinfo gcc-multilib build-essential \
    socat cpio python3 python3-pip python3-git python3-jinja2 xz-utils \
    debianutils iputils-ping libegl1-mesa libsdl1.2-dev zstd liblz4-tool \
    file repo bsdmainutils git-lfs rsync e2fsprogs picocom minicom \
    device-tree-compiler openssh-client && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash yocto
USER yocto
WORKDIR /home/yocto
```

### 3.2 运行（推荐 volume）

```bash
docker run --rm -it \
  -v /path/to/shuhe:/home/yocto/shuhe \
  -v /path/to/skill-app:/home/yocto/.config/opencode/skills/shuhe-ostl-app \
  --name shuhe-app \
  shuhe-app-base:latest
```

- Yocto 树放 volume，镜像只装依赖。
- 不要以 root 跑 bitbake；用 `yocto` 用户。
- 如果 BitBake 需要 sudo（安装依赖/改 host），在镜像构建阶段完成，不要运行时做。

## 4. 首次准备（交付前）

1. 在 VM/容器里确认构建树可用：
   ```bash
   cd /home/yocto/shuhe/build-*stm32mp13-*
   unset BUILDDIR
   source ../layers/openembedded-core/oe-init-build-env .
   bitbake -p
   ```
2. 预构建目标镜像：
   ```bash
   # 纯网络
   bitbake shuhe-test-image-core
   # GUI
   bitbake st-image-weston
   ```
3. 烧录到板子，确认串口/网络/显示正常（按 `shuhe-ostl-deploy`）。
4. 安装 app skill stub：
   ```bash
   git clone -b shuhe-dev --depth 1 https://github.com/stm32mp9527/oe-manifest.git /tmp/om
   mkdir -p ~/.config/opencode/skills
   cp -r /tmp/om/skill-app ~/.config/opencode/skills/shuhe-ostl-app
   ```
5. 预置 `meta-customer-app` 模板（可选），并做一个 demo app 验证全流程。

## 5. 客户现场对话流程

客户只需要说：

```text
当前环境的 yocto 在 /home/yocto/shuhe，使用 shuhe-ostl-app 帮我开发一个
TCP 网络通讯程序，端口 9000，可以开机自动执行。
```

或：

```text
当前环境的 yocto 在 /home/yocto/shuhe，使用 shuhe-ostl-app 帮我开发一个
在 HDMI 上全屏显示图片的程序，图片在 /home/yocto/logo.bmp，开机自动执行。
```

AI 应：

1. 找到 `<ROOT>`/`<BUILD>`/`<DEPLOY>`，确认镜像能力。
2. 问清协议/分辨率/自启等细节。
3. 生成 `meta-customer-app` recipe/service。
4. `bitbake <app>`，部署/重启，验证。
5. 告诉客户源码路径和修改后的重建命令。

## 6. 展会演示建议

- 预烧一个带 demo app 的镜像；现场只改源码。
- 演示“改一行代码 → `bitbake app` → scp/restart → 屏幕/网络变化”。
- 保留恢复镜像/U 盘，防止现场改坏。
- 不要让客户在展会上等全量 `bitbake st-image-weston`；预构建好。

## 7. 安全/纪律

- 不以 root 跑 bitbake。
- 不把客户 app 放进 `tmp-glibc/work`。
- 不在 app skill 里改 kernel/uboot/dts；需要时转 `shuhe-ostl-deploy`。
- 容器/VM 里不要把 `DL_DIR`/`SSTATE_DIR` 指到宿主机不存在的路径。
