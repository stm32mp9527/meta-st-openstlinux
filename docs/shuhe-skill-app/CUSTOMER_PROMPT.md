# STM32MP1 应用开发入口（给 AI 的提示词）

我是 STM32MP1 的应用开发者，可能是电池、电机、传感器、数据采集、工业协议、
网络、显示等任何领域的工程师。

**我不学 Yocto，也不装交叉编译器。**
你负责所有 Yocto 构建、编译、打包、部署和开机自启；
我只描述应用需求。

---

## 你先做什么

1. 找到项目根目录 `<ROOT>`：包含 `.repo/` 或 `layers/meta-st/meta-st-stm32mp`
   的目录。找不到就问我路径；不要让我安装工具链。
2. 读完整 AI 手册（不是给我看的）：
   ```text
   <ROOT>/layers/meta-st/meta-st-openstlinux/docs/shuhe-skill-app/SKILL.md
   <ROOT>/layers/meta-st/meta-st-openstlinux/docs/shuhe-skill-app/APP_MAP.md
   ```
3. 只做上层 app；**不要改 kernel / u-boot / TF-A / OP-TEE / DTS / pinctrl / 分区**。
   这些底层需求转 `shuhe-ostl-deploy`。
4. 自动创建 `meta-customer-app`、app recipe、systemd service，编译、部署、验证。
5. 最后告诉我：**app 源码在哪、我改完以后怎么让你重建**。不要让我手写 Makefile。
6. 如果项目还没同步，按下面的流程处理；不要让我学 Yocto 命令。

如果还没有项目（**AI 执行，不要让我做**）：

```bash
mkdir -p <ROOT> && cd <ROOT>
repo init -u https://github.com/stm32mp9527/oe-manifest.git -b shuhe-dev -m default.xml
repo sync -j8
```

sync 完再读 app skill。

---

## 先问我这些（不要猜）

### 任意领域（先问，不要默认电池/网络/显示）

- 你是什么领域的工程师？这个 app 要解决什么问题？
- 输入是什么物理量/数据/协议？来自哪个接口
  （IIO / hwmon / power_supply / I2C / SPI / UART / CAN / USB / V4L2 / ALSA /
  GPIO / 网络）？
- 处理算法/协议/状态机？采样率、精度、实时性要求？
- 输出是什么（GPIO / PWM / 继电器 / 电机 / 屏幕 / 网络 / 存储）？
- 安全/急停/互锁/失败默认状态？
- 开机自启/日志/远程调试？
- 当前内核是否已暴露标准接口？没有 → BSP，不要 app 层写驱动。

### 网络应用

- TCP / UDP / HTTP / MQTT？
- 服务端还是客户端？端口、绑定地址、数据格式？
- 收到数据做什么？回复什么？
- 开机自启？断线重连？
- 板子 DHCP 还是静态 IP？接口名？

### 显示应用

- 图片 / 视频 / 文本 / UI？
- 分辨率、全屏还是窗口？
- 图片/视频文件在哪？需要触摸/键盘输入？
- 开机自启？退出方式？

### 传感器 / 数据采集 / 控制（电池只是示例）

- 传感器/设备型号与接口：I2C / SPI / ADC / IIO / hwmon / power_supply / UART / CAN？
- 物理量和单位/缩放（电压 uV？温度 0.1°C？压力/流量/电流？）？
- 采样周期、算法/协议/状态机（SOC、滤波、PID、Modbus、客户自己的算法）？
- 是否需要控制充放电/电机/继电器/阀（GPIO/PWM）？
- 告警阈值、日志格式（CSV/SQLite）、是否需要网络上传或屏幕显示？
- 开机自启？
- 实时性要求：普通监控/控制用 Linux app；µs 级电流环/电机环转 M33/BSP。

> 电池只是一个例子；温度、压力、流量、电机电流、仪表数据、工业协议都适用。
> 如果设备只有厂商内核驱动、当前内核没暴露 `power_supply`/`hwmon`/IIO，
> 这是 BSP/驱动工作，转 `shuhe-ostl-deploy`；不要在 app 层写 kernel driver。

### 工业协议

- Modbus RTU/TCP？主站/从站？串口参数/寄存器表？
- CAN / CAN FD？波特率、ID、DBC/CANopen/J1939？
- MQTT/HTTP？Broker、Topic、QoS、TLS、断线补传？
- 只读采集，还是要写寄存器/发控制命令？互锁是什么？

---

## 你负责（不要让我做）

1. 检查当前镜像能力（core / Weston / SDL2 / Python / 网络管理器）。
2. 选最接近的模板，或按“输入 → 算法 → 输出 → systemd”生成新的 app 骨架；
   **不要把所有客户都套成电池/网络/显示模板**。
3. 创建 `meta-customer-app`，编译、打包、部署、配置开机自启。
4. 用安装包或整镜像部署；有 SSH 就增量，没有就重烧。
5. **必须 reboot 后再验证**，不能只测 `systemctl enable --now`。
6. 告诉我源码路径和重建方式，用非 Yocto 术语。

### 边界

- 电机硬实时/安全过流：Linux 用户态不承诺；用 M33/硬件比较器/驱动器保护，
  Linux app 只做监控、记录、通信。安全类 app 用 `Restart=on-failure` + 故障锁存，
  不要故障后自动重启重新驱动电机。
- CAN/RS485/编码器/PWM/ADC：先确认内核已经暴露 `can0`/`/dev/ttySTM*`/
  `/sys/bus/counter`/`/sys/class/pwm`/IIO；没有就转 BSP。
- 需要新驱动、DTS、pinctrl、实时补丁：转 `shuhe-ostl-deploy`。
- 如果 `<BUILD>` 路径含 `-test`：先问我逐项批准，或新建独立 app build；
  不要默认改 protected 的 test 构建。

---

## 示例需求（照这个格式说）

### 电池管理

```text
我是电池工程师，不想学 Yocto。使用 shuhe-ostl-app 帮我开发一个电池管理程序：
读取电压/电流/温度，计算 SOC，异常告警，记录 CSV，开机自启。
```

### 电机/运动

```text
我是电机工程师。使用 shuhe-ostl-app 帮我开发一个电机监测程序：
读取电流/编码器，PWM 输出，过流保护，记录日志，开机自启。
```

> AI 要说明：普通监控/低速控制可用 Linux app；µs 级电流环/硬实时转 M33/BSP。

### 传感器/仪表

```text
我是传感器工程师。使用 shuhe-ostl-app 帮我开发一个数据采集程序：
每 1 秒采集温度/压力，写 CSV，超限告警，开机自启。
```

### 工业协议

```text
我是工业工程师。使用 shuhe-ostl-app 帮我开发一个 Modbus/CAN 采集程序：
读取设备数据，通过 MQTT 上传，断线重连，开机自启。
```

### 网络

```text
使用 shuhe-ostl-app 帮我开发一个 TCP 网络通讯程序，端口 9000，可以开机自动执行。
```

### 显示

```text
使用 shuhe-ostl-app 帮我开发一个在 HDMI 上全屏显示图片的程序，
图片在 /opt/logo.bmp，开机自动执行。
```

---

## 我的需求

```text
-
```

如果我只发了本文件、没有写需求：先问我要做什么，不要自己乱改。
