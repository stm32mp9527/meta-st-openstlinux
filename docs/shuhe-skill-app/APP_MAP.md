# APP_MAP — STM32MP1 上层应用类型/依赖/验证矩阵

> 本文件是 `shuhe-ostl-app` 的快速索引；BSP/显示驱动/网络驱动问题转 `shuhe-ostl-deploy`。

## 1. 应用类型 → 模板/依赖/验证

| 客户需求 | 模板/语言 | 关键依赖 | 自启 target | 验证 |
|----------|-----------|----------|-------------|------|
| TCP 服务端 | `templates/network-app` C | 无（libc） | `multi-user.target` | `nc <ip> <port>` |
| TCP 客户端 | 改 network 模板 | 无 | `multi-user.target` | 服务端收包 |
| UDP | 改 network 模板 | 无 | `multi-user.target` | `nc -u` |
| HTTP 客户端 | C + libcurl | `DEPENDS/RDEPENDS = "curl"` | `multi-user.target` | `curl`/日志 |
| HTTP 服务端 | C + civetweb/mongoose | 对应库 | `multi-user.target` | `curl` |
| MQTT | C + mosquitto | `mosquitto` | `multi-user.target` | `mosquitto_sub` |
| 图片全屏 | `templates/display-app` C+SDL2 | `libsdl2` | `multi-user.target` | 屏幕/`journalctl` |
| PNG/JPG | SDL2_image | `libsdl2-image` | `multi-user.target` | 屏幕 |
| 文字/UI | SDL2_ttf 或 GTK/Qt | 对应库 | `multi-user.target` | 屏幕 |
| 视频 | GStreamer/Wayland | `gstreamer1.0-*` | `multi-user.target` | 屏幕/日志 |
| 触摸/键盘输入 | SDL2/Wayland 自带 | 无额外 | `multi-user.target` | 输入设备 |
| 电池管理/传感器 | `templates/battery-monitor` C | 无（sysfs） | `multi-user.target` | CSV/告警/服务 |

> 显示 app service 用 `multi-user.target` + `After=weston-graphical-session.service`；
> 不要用 `graphical.target`（实际 default 是 multi-user，重启不会自启）。

## 2. MP1 镜像能力矩阵

| 镜像 | Weston/Wayland | GTK/Python | SDL2 | 适合 |
|------|----------------|------------|------|------|
| `shuhe-test-image-core` | 否 | 否 | 否 | 纯 C 网络 app；有 networkd+DHCP |
| `shuhe-image-core` | 否 | 否 | 否 | 纯 C 网络 app；有 networkd+DHCP |
| `st-image-weston` | 是 | 是（demo/GTK/Python） | **是**（opengl 包组） | GUI app |

- core 镜像没有 Python3/GTK/SDL2；不要假设 `python3`/`pip` 可用。
- **`st-image-weston` 通常已含 SDL2**（`packagegroup-framework-core-base-display`
  在 `opengl` 下依赖 `libsdl2`/`libsdl2-image`）；app 的 `RDEPENDS` 保留作保险。
- 客户要 GUI 但用的是 core 镜像：换/构建 `st-image-weston`；这是镜像级改动，
  不是 kernel 改动。core 另有 `lvgl-demo-fb`/`fbset`/DRM fbdev，可作为非 Weston 备选，
  但需 root+显示验证，不属于本 SDL2 模板。

## 3. 网络管理器/接口

| 环境 | 检查 | 自启网络做法 |
|------|------|--------------|
| systemd-networkd | `systemctl is-active systemd-networkd` | `/etc/systemd/network/*.network`；当前 core 已有 `80-wired.network` DHCP |
| busybox udhcpc | `command -v udhcpc` | **仅当没有 networkd** 才加 `udhcpc@end0.service` |
| connman/NetworkManager | `command -v connmanctl/NetworkManager` | 用对应工具配置 |
| 静态 IP | 客户指定 | `.network` 或 `ip addr add` service |

- MP1 自定义板接口名：`end0`/`end1`（不是 eth0/eth1）。
- **当前 `shuhe-test-image-core` 实测有 `systemd-networkd` + DHCP**；不要重复加 udhcpc，
  否则抢配置。
- 只写 `After=network.target` 不等于有 IP；客户端要能连上必须有地址。
- 演示建议静态 IP 或明确 DHCP service，避免现场等租约。

## 3.1 电池管理/传感器 app

模板：`templates/battery-monitor/`（C + sysfs + systemd）。

| 数据/功能 | 优先接口 | 典型路径/命令 |
|-----------|----------|----------------|
| 电压/电流/温度 | `power_supply` | `/sys/class/power_supply/battery/{voltage_now,current_now,temp}` |
| 电压/电流/温度 | `hwmon` | `/sys/class/hwmon/hwmonX/in*_input` |
| ADC/传感器原始值 | IIO | `/sys/bus/iio/devices/iio:deviceX/in_voltage*_raw` + `scale` |
| I2C 电量计 | `i2c-dev` / `i2c-tools` | `/dev/i2c-*`、`i2cdetect` |
| SPI 传感器 | `spidev` | `/dev/spidevX.Y` |
| 充电/放电控制 | `gpiod` | `gpioinfo`、`gpioset` |
| CAN BMS | `socketcan` | `can0`、`candump` |
| 串口 BMS | UART | `/dev/ttySTM*` |

- **先确认单位**：`voltage_now` 常见 uV、`current_now` 常见 uA、`temp` 常见 0.1°C；
  读错会得到离谱 SOC。
- 默认模板用线性 SOC 占位，必须换成客户的 OCV 查表/库仑计算法。
- 传感器没有内核驱动、或内核没暴露 `power_supply`/`hwmon`/IIO → 转
  `shuhe-ostl-deploy`，不要在 app layer 写 kernel driver。
- 没有硬件时用 mock sysfs 验证逻辑（见 `SKILL.md` §5.2）。

## 4. Weston/显示环境

ST 的 Weston：

```text
service: weston-graphical-session.service
user:    weston
runtime: /run/user/<uid> 或 /home/weston（以实际 socket 为准）
display: wayland-0（若有 wayland-1 则用 1；两边都查）
log:     /home/weston/weston.log
```

app service（用 wrapper，不要用 `%U`）：

```ini
User=weston
ExecStart=/usr/bin/customer-display.sh
After=weston-graphical-session.service
Wants=weston-graphical-session.service

[Install]
WantedBy=multi-user.target
```

`customer-display.sh`：

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

- **不要写 `XDG_RUNTIME_DIR=/run/user/%U`**：system manager 的 `%U` 是 0，
  不是 weston 的 UID，Wayland 连不上。
- **`WantedBy=multi-user.target`**：实际 `default.target` 是 multi-user；
  写 `graphical.target` 会“手工能跑、重启不自启”。
- `st-image-weston` 自带 `demo-launcher`，约 5 秒后会启动 demo，可能覆盖客户画面；
  kiosk 用 `/etc/default/demo-launcher` 设 `DEFAULT_DEMO_APPLICATION=/bin/true`
  或删除 `start_up_demo_launcher.sh`。
- app 用 SDL2/Wayland 时不要以 root 直接抢 DRM；除非客户明确要 KMS/DRM。
- 屏幕无信号/花屏/分辨率不对：先确认 Weston 正常；这是 BSP/显示问题，
  转 `shuhe-ostl-deploy` 的显示章节，不要改 app 硬扛。

## 5. 常见依赖

| 需求 | 编译依赖 `DEPENDS` | 运行依赖 `RDEPENDS:${PN}` |
|------|-------------------|---------------------------|
| SDL2 | `libsdl2` | `libsdl2` |
| PNG/JPG | `libsdl2-image` | `libsdl2-image` |
| 文字 | `libsdl2-ttf` | `libsdl2-ttf` + 字体 |
| HTTP | `curl` | `curl` |
| JSON | `json-c` | `json-c` |
| MQTT | `mosquitto` | `mosquitto` |
| SQLite | `sqlite3` | `sqlite3` |
| OpenSSL | `openssl` | `openssl` |
| GStreamer | `gstreamer1.0` | `gstreamer1.0-plugins-*` |

> 依赖名以实际 recipe 为准；用 `bitbake -s | grep <name>` 确认，不要猜。

## 6. 应用 recipe 检查清单

- [ ] `SUMMARY` / `LICENSE` / `LIC_FILES_CHKSUM` 正确
- [ ] `SRC_URI` 包含所有源文件/服务/资源
- [ ] `do_compile` 使用 `${CC} ${CFLAGS} ${LDFLAGS}`
- [ ] `do_install` 安装到 `${bindir}`、`${systemd_system_unitdir}`、`${datadir}`
- [ ] `inherit systemd`
- [ ] `SYSTEMD_SERVICE:${PN}` / `SYSTEMD_AUTO_ENABLE:${PN} = "enable"`
- [ ] `FILES:${PN}` 包含 service/资源
- [ ] `DEPENDS`（编译）和 `RDEPENDS:${PN}`（运行）区分正确
- [ ] 源码在 `meta-customer-app`，不在 `tmp-glibc/work`
- [ ] 如果 `<BUILD>` 含 `-test`：先取得逐项批准或新建独立 app build；不要默认改 test 构建

## 7. 已知坑

1. **core 镜像没有 Python/Weston/SDL2**：GUI app 先换 `st-image-weston`。
2. **core 镜像已有 networkd + DHCP**：先 `systemctl is-active systemd-networkd`；
   不要重复加 udhcpc 抢配置。
3. **Weston 用户不是 root**：app service 要 `User=weston`，用 wrapper 设
   `XDG_RUNTIME_DIR=$(id -u)`；**不要写 `%U`**。
4. **重启不自启**：`WantedBy=multi-user.target`，不是 `graphical.target`；
   必须 reboot 复测。
5. **`st-image-weston` demo launcher 抢屏**：kiosk 禁用 `demo-launcher`。
6. **`/dev/fb0` 不是唯一路径**：默认推荐 Weston/Wayland；core 也有 DRM fbdev/LVGL，
   但需 root+显示验证，不属于 SDL2 模板。
7. **`/dev/spidev*` 需要 child node**：app 层不需要碰；这是 BSP/DTS 问题。
8. **app 不在镜像**：`IMAGE_INSTALL:append` 没加或 image 没重建。
9. **运行时缺库**：`DEPENDS` 只保证编译，`RDEPENDS` 才进 rootfs。
10. **`rm_work` 后找不到 work/image**：用 `deploy/deb/<arch>/<app>_*.deb` 部署。

## 8. 什么时候必须转 BSP skill

- 改分辨率、接新屏、HDMI/DSI/LVDS 不亮、背光/触摸不工作
- 加新网口/PHY、改 MAC/PHY 地址、网络驱动
- 改 kernel config、dts、pinctrl、uboot、TF-A、OP-TEE、分区
- 需要 root + DRM/KMS 直接控制显示（非 Weston app）

这些不是 app 层能解决的，转 `shuhe-ostl-deploy`，不要在本 skill 里改 BSP。
