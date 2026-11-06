---
name: shuhe-ostl-app
description: |
  STM32MP1 OpenSTLinux 上层应用开发手册（type 2 客户：不改 kernel/uboot/dts，
  只开发 app；任意领域：电池/电机/传感器/数据采集/工业协议/网络/显示）。
  是 shuhe-ostl-deploy 的 companion skill。
  触发词：开发 app、上层应用、电池管理、电机、传感器、数据采集、Modbus、CAN、
  网络通讯程序、TCP/UDP/HTTP/MQTT、HDMI/LCD 显示、图像程序、开机自启动、
  第三方开发、不碰底层、app recipe、bitbake app。
  客户不需要知道交叉编译器；AI 负责生成 recipe、bitbake、打包、部署、开机自启。
---

# /shuhe-ostl-app — STM32MP1 上层应用 AI 工作手册

> 本 skill 是 `shuhe-ostl-deploy`（MP1 BSP/部署手册）的 **companion**：
> - 部署、镜像、烧录、kernel/uboot/dts、引脚、显示驱动、网络驱动 → `shuhe-ostl-deploy`
> - 只写 app、生成 app recipe、打包、部署、开机自启 → 本 skill
> - STM32MP2/257F-DK → `shuhe-ostl-mp2`

## 0. 铁律（每次会话先读）

1. **只做上层应用**：不改 kernel、u-boot、TF-A、OP-TEE、DTS、pinctrl、分区、
   bootloader、显示/网络驱动。需要改这些时，**停止**并转 `shuhe-ostl-deploy`。
2. **客户不需要懂交叉编译器**：不要要求客户手动 source 工具链、设置 `CROSS_COMPILE`、
   写 Makefile。AI 用 bitbake/recipe 完成编译、打包、部署。
3. **先定位再动手**：先找 `<ROOT>`/`<BUILD>`/`<DEPLOY>`，找不到就问客户要路径；
   禁止硬编码 `/home/xxx`。
4. **app 放独立 layer**：源码、recipe、service 放 `meta-customer-app`；
   不改 `meta-st-*` 官方层。
5. **先问清需求**：网络协议/端口/服务端客户端；显示内容/分辨率/是否全屏；
   是否开机自启；板子 IP/接口/是否可 SSH。不要猜。
6. **必须验证**：`bitbake` 通过 + 目标机服务启动 + 实际网络/显示效果；
   不能只说“应该可以”。
7. **不把 app 放 `tmp-glibc/work`**：那是临时目录；永久源码放 `meta-customer-app`。
8. **test 构建门禁**：如果 `<BUILD>` 路径含 `-test`，先向客户说明并取得逐项批准；
   优先新建/复制一个独立的 app build 目录。不得默认对 `*-test` 执行
   `bitbake-layers add-layer` 或改 `local.conf`。需要改 BSP 时转 `shuhe-ostl-deploy`。

## 1. 和现有 skill 的分工

| 需求 | 用哪个 skill |
|------|--------------|
| 新电脑部署、repo sync、envsetup、bitbake 镜像、烧录 | `shuhe-ostl-deploy` |
| 改 dts/pinctrl、加外设、改 kernel/uboot/内存/分区 | `shuhe-ostl-deploy` |
| 写一个 app，访问网络/屏幕，开机自启 | **本 skill** |
| STM32MP257F-DK / MP2 | `shuhe-ostl-mp2` |

如果客户一句话里既有 BSP 又有 app：先拆成两部分，BSP 部分交给 `shuhe-ostl-deploy`，
app 部分用本 skill。

## 2. 环境识别（AI 先做只读检查）

### 2.1 找项目/构建/产物

```bash
pwd
find . -maxdepth 5 -type d -name meta-st-stm32mp 2>/dev/null
find .. -maxdepth 5 -type d -name meta-st-stm32mp 2>/dev/null
ls -d build-*stm32mp13-* 2>/dev/null
find . -maxdepth 5 -type d -path '*/tmp-glibc/deploy/images/*' 2>/dev/null | head
```

- `<ROOT>`：包含 `.repo/` 或 `layers/meta-st/meta-st-stm32mp` 的目录。
- `<BUILD>`：`build-*stm32mp13-*`；用 `conf/local.conf` 确认 `MACHINE`/`DISTRO`。
- `<DEPLOY>`：`<BUILD>/tmp-glibc/deploy/images/<MACHINE>`。

### 2.2 确认镜像和运行时能力

先找目标 image 的 rootfs manifest（**排除 initrd manifest**）：

```bash
# 只选 <image>-*.rootfs.manifest，不要选 st-image-resize-initrd-*.manifest
M=$(find <DEPLOY> -maxdepth 1 -name '<image>-*.rootfs.manifest' | head -1)
[ -n "$M" ] || M=$(ls -t <DEPLOY>/<image>-*.rootfs.manifest 2>/dev/null | head -1)
echo "manifest=$M"
```

检查关键包（用前缀匹配，匹配 `libsdl2-2.0-0` / `python3-core` / `gtk+3-demo`）：

```bash
for p in weston wayland libsdl2 python3 qtbase 'gtk+3' systemd dpkg dropbear; do
  printf '%-10s ' "$p"; grep -c "^$p" "$M"
done
```

没有 manifest 时：

```bash
cd <BUILD>
unset BUILDDIR
source <ROOT>/layers/openembedded-core/oe-init-build-env .
bitbake -e <image> | grep '^IMAGE_INSTALL='
```

### 2.3 镜像能力基线（MP1）

| 镜像 | 特点 | 适合 |
|------|------|------|
| `shuhe-test-image-core` | 极简；无 Weston/Wayland/Python3/SDL2；有 systemd-networkd + DHCP、dropbear | 纯 C 网络 app；无 GUI |
| `shuhe-image-core` | 同上（官方 DK） | 纯 C 网络 app；无 GUI |
| `st-image-weston` | Weston/Wayland；**已含 SDL2**（opengl 包组）和 GTK/Python/demo | GUI app、网络+GUI |

- **DISTRO 名不等于镜像内容**：`DISTRO=openstlinux-weston` 不代表镜像装了 Weston；
  以 manifest/rootfs 为准。
- 客户要 **显示 app** 但当前镜像是 `*-core`：改用 `st-image-weston`（或自建
  Weston 镜像）。这是镜像级改动，不是 kernel 改动；首次构建会久一点。
- 客户只要 **网络 app**：`*-core` 就够；不要为了 app 去改 BSP。
- `st-image-weston` 通常已含 SDL2；app 的 `RDEPENDS:${PN} = "libsdl2"` 保留作保险，
  不作为“镜像缺 SDL2”的前提。
- 需要 Qt/GTK/视频/GStreamer 时，先检查镜像是否已有；没有就加包/换镜像，不要猜。

### 2.4 网络管理器/接口

```bash
# 目标机
ip -br a
systemctl is-active systemd-networkd 2>/dev/null
networkctl 2>/dev/null
command -v udhcpc connmanctl NetworkManager 2>/dev/null
```

- MP1 自定义板接口名通常是 `end0`/`end1`（不是 eth0/eth1）。
- **当前 `shuhe-test-image-core` 实测有 `systemd-networkd` + `80-wired.network`
  （DHCP=yes）**；不要重复加 udhcpc，否则会和 networkd 抢配置。
- 先 `systemctl is-active systemd-networkd`：active 就用 networkd；只有确实没有
  networkd 时，才加 `udhcpc` service 或静态 IP。
- `st-image-weston` 一般也有 networkd。
- 不要假设 DHCP 一定成功；演示环境建议静态 IP 或明确 DHCP 服务。

### 2.5 板子访问方式

- 有 SSH：`ssh root@<board-ip>`，可增量部署 app。
- 无 SSH：只能重建镜像 + 重新烧录。
- 先问客户板子 IP/用户名/接口，不要猜。

## 3. 需求 intake（问清再写代码）

### 3.0 任意领域 app（先做这个，不要假设客户是电池/网络/显示）

客户可能是电池、电机、传感器、数据采集、工业协议、音频、视觉、PLC、仪器仪表
等任何领域的工程师。**不要把“电池”当默认**；先问领域，再映射到 Linux 接口。

必问：

1. **领域/目标**：你是什么工程师？这个 app 要解决什么问题？给谁看/控制什么？
2. **输入**：什么物理量/数据/协议？来自哪个接口？
   （IIO / hwmon / power_supply / I2C / SPI / UART / CAN / USB / V4L2 / ALSA /
   GPIO / 网络 / 文件）
3. **处理**：客户自己的算法/协议/状态机；采样率、精度、实时性要求。
4. **输出**：GPIO / PWM / 继电器 / 电机 / 屏幕 / 网络 / 存储 / 告警。
5. **安全**：急停/互锁/默认安全状态/看门狗；失败时怎么处理。
6. **运行**：开机自启、失败重启、日志、远程调试/升级。
7. **内核接口**：当前内核是否已经暴露标准接口？没有则转 BSP，不要在 app 层写驱动。

领域 → 接口 → 模板起点：

| 领域 | 典型输入 | Linux 接口 | 典型输出 | 起点 |
|------|----------|------------|----------|------|
| 电池/BMS | 电压/电流/温度 | `power_supply`/`hwmon`/IIO | GPIO/PWM/继电器、CSV、网络 | `battery-monitor`（通用 sysfs 骨架） |
| 电机/运动 | 编码器/电流/限位 | GPIO/IIO/PWM/counter | PWM/GPIO/继电器 | `battery-monitor` + PWM/GPIO；硬实时转 M33/BSP |
| 温度/压力/流量 | 传感器 | IIO/hwmon/I2C/SPI | 日志/显示/网络 | `battery-monitor` 改路径/单位/算法 |
| 数据采集/仪表 | ADC/传感器 | IIO/spidev/i2c-dev | CSV/SQLite/网络 | `battery-monitor` 或 `network-app` |
| 工业协议 | Modbus/CAN/串口 | libmodbus/SocketCAN/tty | 网络/PLC/数据库 | `network-app` + 协议库 |
| 网络/云 | socket/MQTT/HTTP | 网络 | 网络/存储 | `network-app` |
| HMI/显示 | 图片/文本/UI | Weston/SDL2/GTK/Qt | 屏幕 | `display-app` |
| 视觉 | Camera | V4L2/libcamera/GStreamer | 屏幕/网络 | BSP camera + app |
| 音频 | Mic/Line | ALSA/PipeWire | 播放/网络 | BSP audio + app |

> 硬实时（µs 级电机/电流环）不要承诺用 Linux 用户态 app 实现；用 M33/M4 或
> BSP 的实时方案，Linux app 做监控/配置/通信。需要新驱动、DTS、pinctrl、实时补丁
> → 转 `shuhe-ostl-deploy`。

- **电机安全**：软件过流只能做监测/后备；安全过流用硬件比较器 → 定时器 break →
  驱动器关断，或 M33。安全类 service 用 `Restart=on-failure` + 故障锁存，
  不要 `Restart=always` 让故障后自动重启重新驱动电机；必须有 PWM=0/继电器断开的
  fail-safe。
- **工业协议**：Modbus 用 `libmodbus`；CAN 用 SocketCAN + 内核 `can0`（本板 DTS
  可能 disabled，转 BSP）；MQTT 用 `libmosquitto`，运行时依赖是
  `libmosquitto1` 不是 `mosquitto`。链接这些库的 app 不能只 scp 二进制，
  要整镜像或 `.deb` 带依赖部署。

### 3.1 网络 app

1. TCP / UDP / HTTP / MQTT / WebSocket？
2. 服务端还是客户端？
3. 端口、绑定地址、协议格式（文本/二进制/JSON/行协议）？
4. 收到消息后做什么？回复什么？是否需要多客户端并发？
5. 是否开机自启？失败是否自动重启？
6. 板子用 DHCP 还是静态 IP？接口名？

默认（客户没指定时）：TCP echo server，端口 9000，绑定 `0.0.0.0`，systemd 自启。

### 3.2 显示 app

1. 显示图片 / 视频 / 文本 / UI？分辨率/是否全屏？
2. 图片/视频文件谁提供？放哪里？
3. 是否需要触摸/键盘/鼠标输入？
4. 是否开机自启？是否需要退出方式？
5. 当前镜像是否有 Weston？没有则先换 `st-image-weston`。

默认：SDL2 全屏渐变 + 可选 BMP 图片，Weston 下全屏，systemd 自启。

### 3.3 语言

- 默认 **C**：无额外运行时，体积小，启动快。
- 客户明确要 Python：仅当镜像有 `python3`；否则加包或改用 C。
- C++：仅当镜像有 `libstdc++`；否则加 `RDEPENDS` 或改用 C。
- 不要默认引入 Qt/GTK；除非客户明确要 UI 框架且镜像已有。

## 4. 生成 app layer（标准流程）

### 4.1 目录

```text
<ROOT>/meta-customer-app/
├── conf/layer.conf
├── recipes-apps/<app>/
│   ├── <app>.bb
│   └── files/
│       ├── <app>.c
│       └── <app>.service
└── README-app.md
```

### 4.2 注册 layer

> **test 构建门禁**：如果 `<BUILD>` 路径含 `-test`，先取得客户逐项批准，
> 或新建一个独立的 app build 目录；不要默认对 test 构建执行 `bitbake-layers add-layer`
> 或改 `local.conf`。MP1 的 `*-test` 资产受 BSP skill 铁律保护。

```bash
cd <BUILD>
unset BUILDDIR
source <ROOT>/layers/openembedded-core/oe-init-build-env .
bitbake-layers add-layer <ROOT>/meta-customer-app
bitbake-layers show-layers | grep customer-app
```

### 4.3 加进镜像

快速验证：在 `<BUILD>/conf/local.conf` 末尾加：

```conf
IMAGE_INSTALL:append = " <app>"
```

交付/永久：在自己的 image bbappend 里加同样的行，不要只改 `local.conf`：

```text
<ROOT>/meta-customer-app/recipes-st/images/<image>.bbappend
```

### 4.4 构建

```bash
cd <BUILD>
unset BUILDDIR
source <ROOT>/layers/openembedded-core/oe-init-build-env .
bitbake <app>
bitbake <image>
```

- 只编 app：`bitbake <app>`。
- 要进镜像/开机自启：`bitbake <image>`（或增量部署，见 §7）。
- 首次构建 `st-image-weston` 会久一点；SDL2 通常已包含。之后走 sstate 增量。

### 4.5 告诉客户源码在哪

生成后在 `README-app.md` 和应用 recipe 里写清楚：

```text
源码：<ROOT>/meta-customer-app/recipes-apps/<app>/files/
改完：bitbake <app> && bitbake <image>
部署：见 README-app.md / skill §7
```

## 5. 网络 app 模板

模板目录：`templates/network-app/`

- `tcp-server.c`：POSIX socket TCP echo server，默认端口 9000。
- `tcp-server.bb`：bitbake recipe，`inherit systemd`，自动 enable。
- `tcp-server.service`：`After=network.target`，`Restart=always`。

复制后改：

- app 名、端口、协议处理、日志。
- 需要的库：HTTP → `curl`；MQTT → `mosquitto`；JSON → `json-c`。
- recipe 里同步加 `DEPENDS`/`RDEPENDS`。

构建/验证：

```bash
bitbake tcp-server
# 目标机（有 SSH）
systemctl status tcp-server
journalctl -u tcp-server -f
# 从 PC 测试
nc <board-ip> 9000
```

### 5.1 网络自启动的前置条件

- `tcp-server.service` 用 `After=network.target` 即可 bind `0.0.0.0`；
  但没有 IP 时客户端连不上。
- **当前 `shuhe-test-image-core` 已有 `systemd-networkd` + `80-wired.network`
  （DHCP=yes）**；先用 networkd，不要再加 udhcpc。
- 只有 `systemctl is-active systemd-networkd` 不是 active 时，才加 `udhcpc` service
  或静态 IP。
- 有 networkd：加 `/etc/systemd/network/52-static.network` 或在镜像里配置 DHCP。
- 如果 app 是客户端、必须拿到 IP 才能启动：用 `After=network-online.target` +
  `Wants=network-online.target`，并确认 `systemd-networkd-wait-online.service` 存在；
  否则保持 `network.target`，让 app 先 bind/重试。

### 5.2 传感器/数据采集/控制 app（电池只是示例）

> `battery-monitor` 是一个**通用 sysfs 轮询/告警/CSV 骨架**：读 1~N 个 sysfs 值，
> 做客户算法，输出日志/告警/控制。电池只是示例；温度、压力、流量、电机电流、
> 仪表数据等，改路径/单位/算法即可。
>
> - 电机/运动：读电流/编码器/限位，PWM/GPIO 输出；**硬实时（µs 级电流环）转
>   M33/BSP**，Linux app 做监控/配置/通信。
> - 数据采集/仪表：IIO/SPI/I2C 读多路，写 CSV/SQLite，必要时 MQTT/HTTP 上传。
> - 工业协议：Modbus/CAN/串口，用 `libmodbus`/SocketCAN/termios，套 `network-app`
>   的 systemd/部署骨架。

模板目录：`templates/battery-monitor/`

- `battery-monitor.c`：读 sysfs 的电压/电流/温度，算简单 SOC，超温/欠压告警，
  可写 CSV，systemd 自启。
- `battery-monitor.bb` / `.service`：C + systemd，无额外运行时依赖。
- 默认路径是 `power_supply` 风格：
  ```text
  /sys/class/power_supply/battery/voltage_now   (uV)
  /sys/class/power_supply/battery/current_now   (uA)
  /sys/class/power_supply/battery/temp          (0.1 C)
  ```
  实际路径/单位按客户的 fuel gauge / IIO / hwmon 设备改。

**先问清**：

1. 电量计/传感器型号与接口：I2C / SPI / ADC / IIO / hwmon / power_supply / UART / CAN？
2. 电压/电流/温度的单位和缩放（uV? mV? 0.1°C?）。
3. 采样周期、SOC 算法（OCV 查表 / 库仑计 / 客户自己的算法）。
4. 是否需要控制充电/放电/继电器（GPIO/PWM）？
5. 告警阈值、日志格式（CSV/SQLite）、是否需要 TCP/MQTT 上传或屏幕显示。
6. 开机自启？失败重启？

**Linux 接口映射**：

| 数据/功能 | 优先接口 | 说明 |
|-----------|----------|------|
| 电压/电流/温度 | `power_supply` / `hwmon` / IIO sysfs | 内核已有驱动时直接读 |
| I2C 电量计 | `/dev/i2c-*` + `i2c-dev` | 设备已有内核驱动时优先用 power_supply |
| SPI 传感器 | `/dev/spidev*` | 需要 BSP 已启用 spidev/CS |
| ADC | IIO sysfs | `in_voltageX_raw` + `scale` |
| 充电/放电控制 | `gpiod` / `libgpiod` | 先确认引脚未被别的功能占用 |
| CAN 电池数据 | `socketcan` (`can0`) | 需要 BSP 配好 CAN |
| 串口 BMS | `/dev/ttySTM*` | 需要 BSP 配好 UART |

> 如果传感器只有厂商内核驱动、当前内核没暴露 `power_supply`/`hwmon`/IIO，
> 这是 BSP/驱动工作，转 `shuhe-ostl-deploy`；不要在 app layer 里写 kernel driver。

**构建/验证**：

```bash
bitbake battery-monitor
# 目标机
systemctl status battery-monitor
journalctl -u battery-monitor -b
cat /var/log/battery.csv
```

没有硬件时，先在 PC 上用 mock sysfs 验证逻辑：

```bash
mkdir -p /tmp/bat
echo 4000000 > /tmp/bat/voltage_now
echo 500000  > /tmp/bat/current_now
echo 250     > /tmp/bat/temp
BAT_VOLTAGE=/tmp/bat/voltage_now \
BAT_CURRENT=/tmp/bat/current_now \
BAT_TEMP=/tmp/bat/temp \
BAT_INTERVAL=1 BAT_LOG=/tmp/battery.csv \
./battery-monitor
```

不要假设单位；`voltage_now` 常见是 uV，`temp` 常见是 0.1°C；读错会得到离谱 SOC。

生产采集还要注意：

- CSV 首次创建写表头；按天/大小轮转，避免 1 Hz 长期写满 eMMC。
- 读失败写 `invalid`/`nan` 并置质量码，**不要写 0.0 冒充有效数据**。
- 用 `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME)` 做绝对周期采样，避免漂移。
- 确认 RTC/NTP；断电后时间回退会让 CSV 时间错乱。
- 告警要落日志/GPIO/继电器/MQTT，不能只 printf 到 journal。

## 6. 显示 app 模板

模板目录：`templates/display-app/`

- `customer-display.c`：SDL2 全屏渐变 + 可选 BMP 图片，ESC 退出；图片按输出尺寸铺满。
- `customer-display.sh`：Wayland 环境 wrapper（`XDG_RUNTIME_DIR=$(id -u)`，
  `wayland-0/1`，传 `/opt/logo.bmp`）。
- `customer-display.bb`：`inherit pkgconfig systemd`，`DEPENDS = "libsdl2"`。
- `customer-display.service`：`User=weston`，`ExecStart=/usr/bin/customer-display.sh`，
  `WantedBy=multi-user.target`，`After=weston-graphical-session.service`。

### 6.1 镜像要求

- 需要 Weston/Wayland：用 `st-image-weston`，或在镜像里加 `weston`/`wayland`。
- **`st-image-weston` 通常已含 SDL2**（opengl 包组）；app 的
  `RDEPENDS:${PN} = "libsdl2"` 保留作保险，不作为“镜像缺 SDL2”的前提。
- 如果客户要 PNG/JPG：加 `libsdl2-image`，代码用 `IMG_Load`。
- 如果客户要文字：加 `libsdl2-ttf` + 字体；或改用 GTK/Qt（先确认镜像已有）。
- 如果客户要视频：GStreamer/Wayland；这是更重的依赖，先评估。

### 6.2 Weston 用户/环境（必须用 wrapper，不要用 `%U`）

ST 的 Weston 由 `weston-graphical-session.service` 以 `weston` 用户启动：

```text
XDG_RUNTIME_DIR=/run/user/$(id -u weston) 或 /home/weston（以实际 socket 为准）
WAYLAND_DISPLAY=wayland-0（若有 wayland-1 则用 wayland-1）
```

**不要在 system service 里写 `XDG_RUNTIME_DIR=/run/user/%U`**：
systemd system manager 的 `%U` 是 0，会变成 `/run/user/0`，不是 weston 的 UID，
SDL/Wayland 会连不上。

模板用 wrapper `customer-display.sh`（同时兼容 `/run/user/<uid>` 和 `/home/weston`）：

```sh
#!/bin/sh
UID_W="$(id -u)"
for dir in "/run/user/$UID_W" "/home/weston"; do
    if [ -e "$dir/wayland-0" ] || [ -e "$dir/wayland-1" ]; then
        export XDG_RUNTIME_DIR="$dir"
        break
    fi
done
[ -n "$XDG_RUNTIME_DIR" ] || export XDG_RUNTIME_DIR="/run/user/$UID_W"
if [ -e "$XDG_RUNTIME_DIR/wayland-1" ]; then
    export WAYLAND_DISPLAY="wayland-1"
else
    export WAYLAND_DISPLAY="wayland-0"
fi
exec /usr/bin/customer-display /opt/logo.bmp
```

上板先确认实际 socket 路径：

```bash
ls -l /run/user/$(id -u weston)/wayland-* 2>/dev/null
ls -l /home/weston/wayland-* 2>/dev/null
```

service：

```ini
[Unit]
Description=Customer fullscreen display
After=weston-graphical-session.service
Wants=weston-graphical-session.service

[Service]
Type=simple
User=weston
ExecStart=/usr/bin/customer-display.sh
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target
```

- **`WantedBy=multi-user.target`**：实际 rootfs 的 `default.target` 是
  `multi-user.target`；写 `graphical.target` 会导致“手工 enable 能跑、重启不自启”。
- 如果 Weston 实际用 `wayland-1`，wrapper 已自动检测；也可手工改。
- 如果 app 需要 root（例如直接 DRM/KMS），不要用 Weston 模板；先问客户/转 BSP。

### 6.3 构建/验证（必须 reboot 后复测）

```bash
bitbake customer-display
bitbake st-image-weston
```

目标机：

```bash
systemctl get-default                 # 应为 multi-user.target
id -u weston
ls -l /run/user/$(id -u weston)/wayland-* 2>/dev/null
ls -l /home/weston/wayland-* 2>/dev/null
systemctl status customer-display
journalctl -u customer-display -b
cat /home/weston/weston.log | tail -50
file /opt/logo.bmp
```

然后 **`systemctl reboot`**，起来后再查一次 `status`/`journalctl -b`/实际屏幕。
不能只测 `enable --now`；很多“现场重启失效”都是 target 写错。

- `/opt/logo.bmp`：service 已传这个路径；图片由 recipe 安装或部署时 `scp` 到 `/opt/logo.bmp`。
  文件不存在时 app 会显示渐变并打印 warning。
- 图片缩放：模板按输出尺寸铺满（可能拉伸）；要保持比例就改 C 里的 letterbox 计算。
- **demo launcher 抢屏**：`st-image-weston` 自带 `demo-launcher`，
  `/usr/local/weston-start-at-startup/start_up_demo_launcher.sh` 约 5 秒后会启动 demo，
  可能覆盖客户画面。做 kiosk 时禁用它：
  ```bash
  # 目标机临时验证
  echo 'DEFAULT_DEMO_APPLICATION=/bin/true' > /etc/default/demo-launcher
  # 或镜像里删除/屏蔽 start_up_demo_launcher.sh
  ```
  永久化用 image bbappend/自定义 image，不要改 BSP。
- 屏幕无显示/花屏/分辨率不对：先确认 Weston 本身正常；这是 BSP/显示问题，
  转 `shuhe-ostl-deploy`。core 镜像没有 Weston，不能直接跑 SDL2 模板。

## 7. 部署：增量 vs 整包

### 7.1 有 SSH（推荐给客户开发）

镜像配置了 `INHERIT += "rm_work"`，`tmp-glibc/work/.../image/usr/bin` 可能已被清理。
**用 `.deb` 部署，不要依赖 work 目录**：

```bash
# 1) 编 app
bitbake <app>

# 2) 找 .deb（路径含架构，cortexa7t2hf-neon-vfpv4 / cortexa35 等）
ARCH=<arch>   # MP1: cortexa7t2hf-neon-vfpv4；MP2: cortexa35；以实际目录为准
DEB=$(ls -t <BUILD>/tmp-glibc/deploy/deb/"$ARCH"/<app>_*.deb | head -1)
echo "$DEB"

# 3) 解包
rm -rf /tmp/app-root && mkdir -p /tmp/app-root
dpkg-deb -x "$DEB" /tmp/app-root

# 4) 拷二进制/service（显示 app 还有 customer-display.sh）
scp /tmp/app-root/usr/bin/<app>* root@<board-ip>:/usr/bin/
scp /tmp/app-root/usr/lib/systemd/system/<app>.service root@<board-ip>:/etc/systemd/system/

# 5) 启服务
ssh root@<board-ip> 'systemctl daemon-reload && systemctl enable --now <app>'
ssh root@<board-ip> 'systemctl status <app>'
```

- 需要 `dropbear`/`openssh`；MP1 镜像默认有 dropbear。
- 拷 service 只在第一次或 service 改动时；之后只拷二进制 + restart。
- **必须 `systemctl reboot` 后再测一次**，确认是开机自启，不是 `enable --now` 的假象。
- 如果 app 有资源文件（图片/配置），从 `/tmp/app-root` 对应路径一起拷。

### 7.2 无 SSH

- `bitbake <image>` → 按 `shuhe-ostl-deploy` 的烧录流程重烧。
- 这是最稳的演示方式；适合展会前预置。

### 7.3 展会/演示建议

- 预先构建一个包含 demo app 的镜像，烧好板子。
- 现场只改 app 源码 → `bitbake <app>` → scp/restart，不重烧整包。
- 保留一个“恢复镜像”，防止现场改坏。

## 8. 客户自己改代码

告诉客户（用非技术语言）：

```text
你的 app 源码在：
  <ROOT>/meta-customer-app/recipes-apps/<app>/files/

改完后让 AI 执行：
  bitbake <app>
  # 有 SSH：scp/restart；没有：bitbake <image> 后重烧
```

- 新增源文件：改 recipe 的 `SRC_URI` 和 `do_install`。
- 新增库：改 `DEPENDS`（编译时）和 `RDEPENDS`（运行时）。
- 不要把源码放 `tmp-glibc/work`；不要手写交叉编译器命令。

## 9. 排错

| 症状 | 优先检查 |
|------|----------|
| `bitbake <app>` 找不到头文件 | recipe `DEPENDS` 是否包含库 |
| 运行时报 `libxxx.so not found` | `RDEPENDS:${PN}` 是否包含运行时库；`ldd` |
| app 不在镜像里 | `IMAGE_INSTALL:append` 是否加了 `<app>`；`bitbake -e <image>` |
| 服务起不来 | `systemctl status`、`journalctl -u`、ExecStart 路径/权限 |
| 网络 app 无 IP | 先 `systemctl is-active systemd-networkd`；core 镜像已有 networkd+DHCP，不要重复加 udhcpc；`ip -br a`、`networkctl` |
| 显示 app 无窗口 | Weston 是否运行、`User=weston`、`WAYLAND_DISPLAY`、`XDG_RUNTIME_DIR`（必须 `$(id -u)`，不能 `%U`） |
| 重启后 app 不自启 | 检查 `WantedBy=multi-user.target`（不是 `graphical.target`）；`systemctl get-default`；reboot 复测 |
| demo launcher 覆盖客户画面 | `/etc/default/demo-launcher` 设 `DEFAULT_DEMO_APPLICATION=/bin/true`，或删除 `/usr/local/weston-start-at-startup/start_up_demo_launcher.sh` |
| 找不到编出来的二进制 | 镜像有 `rm_work`；用 `tmp-glibc/deploy/deb/<arch>/<app>_*.deb`，不要找 work/image |
| 显示花屏/无信号 | 这是 BSP/显示驱动问题，转 `shuhe-ostl-deploy`，不要改 app 硬扛 |
| 构建很慢 | 首次构建 Weston/依赖；后续 sstate；只 `bitbake <app>` 增量 |
| 客户要改分辨率/接新屏 | 转 `shuhe-ostl-deploy`（DTS/显示驱动），app 只做适配 |

## 10. Docker / 虚拟机交付

见 `DOCKER_VM.md`。要点：

- 虚拟机/容器里已经有完整 Yocto 构建树和预编译镜像；**不要重新 envsetup/全量构建**。
- 客户只需要说需求；AI 用本 skill 创建 app layer，`bitbake <app>`，部署。
- 首次演示前预构建 `st-image-weston`（GUI，已含 SDL2）或 `shuhe-test-image-core`
  （网络，已有 networkd+DHCP），并烧好板子；现场只迭代 app。
- GUI 演示要禁用 `st-image-weston` 的 demo launcher，避免覆盖客户画面。
- 容器建议用 volume 挂载 Yocto 树（几十 GB，不适合打进镜像层）。
- 如果只有 `*-test` 构建树，先确认客户允许在该构建上加 app layer；否则新建独立
  app build 目录，不要动 test 资产。

## 11. 模板索引

- `templates/README.md` —— 模板选择/泛化；无匹配领域时生成新骨架
- `templates/layer.conf` —— `meta-customer-app` 的 layer.conf
- `templates/network-app/` —— TCP server C + recipe + service
- `templates/battery-monitor/` —— 电池/传感器 C + recipe + service（sysfs/IIO/hwmon）
- `templates/display-app/` —— SDL2 全屏 C + Wayland wrapper + recipe + service
- `APP_MAP.md` —— app 类型/依赖/验证/镜像能力矩阵
- `README.md` —— 文件清单与 stub 安装
- `DOCKER_VM.md` —— Docker/VM 交付与演示流程
- `TEST_RESULTS.md` —— 已做/未做的测试范围（编译、.deb、MP1/MP2 边界）
