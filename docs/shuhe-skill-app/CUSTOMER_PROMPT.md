# STM32MP1 应用开发入口（给 AI 的提示词）

我是 STM32MP1 的应用开发者，可能做电池管理、网络通讯、显示等应用。
**我不想学 Yocto，也不想装交叉编译器。** 你负责所有 Yocto、recipe、bitbake、
编译、打包、部署和开机自启；我只描述应用需求。

---

## 你先做什么

1. 找到项目根目录 `<ROOT>`：包含 `.repo/` 或 `layers/meta-st/meta-st-stm32mp`
   的目录。找不到就问我路径，不要让我装工具链。
2. 读完整 AI 手册（不是给我看的）：
   ```text
   <ROOT>/layers/meta-st/meta-st-openstlinux/docs/shuhe-skill-app/SKILL.md
   <ROOT>/layers/meta-st/meta-st-openstlinux/docs/shuhe-skill-app/APP_MAP.md
   ```
3. 只做上层 app；**不要改 kernel / u-boot / TF-A / OP-TEE / DTS / pinctrl / 分区**。
   这些底层需求转 `shuhe-ostl-deploy`。
4. 自动创建 `meta-customer-app`、app recipe、systemd service，编译、部署、验证。
5. 最后告诉我：**app 源码在哪、我改完以后怎么让你重建**。不要让我手写 Makefile。

如果项目还没同步，且我确认使用 `stm32mp9527/oe-manifest`：

```bash
mkdir -p <ROOT> && cd <ROOT>
repo init -u https://github.com/stm32mp9527/oe-manifest.git -b shuhe-dev -m default.xml
repo sync -j8
```

sync 完再读上面的手册。

---

## 先问我这些（不要猜）

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

### 电池管理 / 传感器应用

- 电量计/传感器型号与接口：I2C / SPI / ADC / IIO / hwmon / power_supply / UART / CAN？
- 电压/电流/温度的单位和缩放（uV? mV? 0.1°C?）？
- 采样周期、SOC 算法（OCV 查表 / 库仑计 / 客户自己的算法）？
- 是否需要控制充电/放电/继电器（GPIO/PWM）？
- 告警阈值、日志格式（CSV/SQLite）、是否需要网络上传或屏幕显示？
- 开机自启？

> 如果传感器只有厂商内核驱动、当前内核没暴露 `power_supply`/`hwmon`/IIO，
> 这是 BSP/驱动工作，转 `shuhe-ostl-deploy`；不要在 app 层写 kernel driver。

---

## 标准做法（你执行，不要让我做）

1. 识别当前镜像：`shuhe-test-image-core`（纯 C 网络 app）或 `st-image-weston`
   （GUI，已含 SDL2）。core 镜像已有 `systemd-networkd` + DHCP，不要重复加 udhcpc。
2. 复制 `docs/shuhe-skill-app/templates/` 里对应模板到 `meta-customer-app`：
   - 网络：`network-app/`
   - 电池/传感器：`battery-monitor/`
   - 显示：`display-app/`
3. `bitbake-layers add-layer`，`IMAGE_INSTALL:append`，`bitbake <app>`/`bitbake <image>`。
4. 部署用 `tmp-glibc/deploy/deb/<arch>/<app>_*.deb`（镜像有 `rm_work`，
   不要找 work/image）；有 SSH 就增量部署，没有就重烧。
5. **必须 reboot 后再验证**，不能只测 `systemctl enable --now`。
6. 告诉我源码路径和重建命令。

> 如果 `<BUILD>` 路径含 `-test`：先取得我的逐项批准，或新建独立 app build；
> 不要默认改 protected 的 test 构建。

---

## 不要做

1. 不要让我学 Yocto/bitbake/交叉编译器/Makefile。
2. 不要改 kernel/uboot/dts/pinctrl/分区/bootloader。
3. 不要把 app 源码放 `tmp-glibc/work`。
4. 不要硬编码本机路径。
5. 不要在没问清协议/分辨率/传感器接口的情况下猜代码。
6. 不要把“bitbake 成功”当成“功能验证通过”。

---

## 示例需求（照这个格式说）

### 电池管理

```text
我是电池工程师，不想学 Yocto。使用 shuhe-ostl-app 帮我开发一个电池管理程序：
读取电压/电流/温度，计算 SOC，异常告警，记录 CSV，开机自启。
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
