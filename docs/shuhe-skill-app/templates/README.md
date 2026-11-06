# templates — 起点，不是所有领域的成品

这些模板是 **AI 生成 app 的起点**，不是每个领域的生产级成品。
客户可能是电池、电机、传感器、数据采集、工业协议、网络、显示、视觉、音频等
任何领域的工程师；先按 `SKILL.md` §3.0 做领域 intake，再选/改模板。

| 模板 | 适合 | 备注 |
|------|------|------|
| `network-app/` | TCP/UDP/HTTP/MQTT 客户端/服务端 | TCP echo 起点；协议自行替换 |
| `battery-monitor/` | sysfs/IIO/hwmon/power_supply 轮询、告警、CSV | **通用采集骨架**；电池只是示例 |
| `display-app/` | Weston/SDL2 全屏图片/UI | 需要 `st-image-weston` |
| 无匹配模板 | 电机、工业协议、视觉、音频、PLC 等 | **生成新骨架**；不要硬套电池/网络/显示 |

## 改模板时必须做的

1. **去掉不适用的领域逻辑**：非电池场景删除 SOC、电压欠压、电池温度阈值。
2. **改环境变量/路径/单位**：`BAT_*` → 你的 `SENSOR_*`；uV/uA/0.1°C 按实际换算。
3. **CSV**：首次写表头；按天/大小轮转；读失败写 `invalid`/`nan`，不要写 0.0。
4. **采样**：用 `CLOCK_MONOTONIC` 绝对周期；确认 RTC/NTP。
5. **service**：网络类 `After=network-online.target`；安全类用 `Restart=on-failure`
   + 故障锁存，不要 `Restart=always` 让故障后自动重启重新驱动电机。
6. **依赖**：`DEPENDS`（编译）和 `RDEPENDS:${PN}`（运行）分开；MQTT 运行时是
   `libmosquitto1`，不是 `mosquitto`。
7. **部署**：链接 `libmodbus`/`libmosquitto1` 的 app 不能只 scp 二进制；
   整镜像或 `.deb` 带依赖安装。
8. **安全**：软件过流不是安全保护；硬件比较器/定时器 break/M33 负责安全关断。

## 没有模板的领域（电机/工业/视觉/音频）

不要硬套 `battery-monitor` 或 `network-app` 的领域逻辑。按下面的通用骨架生成：

```text
输入（IIO/sysfs/socket/CAN/tty/GPIO/V4L2/ALSA）
  → 处理（客户算法/协议/状态机）
  → 输出（GPIO/PWM/继电器/屏幕/网络/存储）
  → systemd service（自启/重启/日志）
```

先查内核是否暴露标准接口；没有就转 `shuhe-ostl-deploy`（驱动/DTS/pinctrl/实时）。
生产级电机/工业/安全应用必须做硬件/BSP 评审和台架验证，不能只靠编译通过。
