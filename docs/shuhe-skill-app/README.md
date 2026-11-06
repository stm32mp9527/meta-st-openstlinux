# shuhe-ostl-app — STM32MP1 上层应用开发手册

本目录是 `shuhe-ostl-deploy`（MP1 BSP/部署手册）的 **companion skill**：
只做上层 app，不改 kernel/uboot/dts；客户不需要知道交叉编译器。

## 文件清单

| 文件 | 内容 |
|------|------|
| `SKILL.md` | 主手册：环境识别、需求 intake、app layer、网络/显示模板、部署、排错 |
| `APP_MAP.md` | app 类型/镜像能力/依赖/验证矩阵 |
| `README.md` | 本文件：使用方式、stub 安装、维护纪律 |
| `DOCKER_VM.md` | Docker/虚拟机交付、预编译镜像、展会演示流程 |
| `TEST_RESULTS.md` | 已做/未做的测试范围（编译、.deb、MP1/MP2 边界） |
| `templates/layer.conf` | `meta-customer-app` 的 layer.conf |
| `templates/network-app/` | TCP server C + recipe + systemd service |
| `templates/battery-monitor/` | 电池/传感器 C + sysfs + recipe + systemd service |
| `templates/display-app/` | SDL2 全屏 C + Wayland wrapper + recipe + systemd service |

## 与现有 skill 的关系

- `shuhe-ostl-deploy`：部署、镜像、烧录、kernel/uboot/dts、引脚、显示驱动、网络驱动。
- `shuhe-ostl-app`（本目录）：只写 app、生成 recipe、打包、部署、开机自启。
- `shuhe-ostl-mp2`：STM32MP257F-DK/MP2；本 skill 面向 MP1。

客户需求里只要出现“改 dts/驱动/分辨率/网口 PHY/分区”，就转 `shuhe-ostl-deploy`。

## AI 使用方式

> **test 构建门禁**：如果 `<BUILD>` 路径含 `-test`，先取得客户逐项批准，或新建
> 独立 app build；不要默认对 protected 的 test 构建执行 `bitbake-layers add-layer`
> 或改 `local.conf`。

1. 找到 `<ROOT>`/`<BUILD>`/`<DEPLOY>`，确认镜像能力（core / weston）。
2. 问清 app 需求（网络协议/显示内容/自启/板子 IP）。
3. 复制 `templates/` 到 `<ROOT>/meta-customer-app/`，按需求改。
4. `bitbake-layers add-layer`，`IMAGE_INSTALL:append`，`bitbake <app>`/`bitbake <image>`。
5. 部署/重启/验证；告诉客户源码路径和重建命令。

## Stub 安装

完整手册随 `meta-st-openstlinux` 层分发；agent 入口是 `oe-manifest` 的
`skill-app/` stub。

```bash
git clone -b shuhe-dev --depth 1 https://github.com/stm32mp9527/oe-manifest.git /tmp/om
mkdir -p ~/.config/opencode/skills
cp -r /tmp/om/skill-app ~/.config/opencode/skills/shuhe-ostl-app

# OpenClaw:
# mkdir -p ~/.openclaw/skills
# cp -r /tmp/om/skill-app ~/.openclaw/skills/shuhe-ostl-app
```

## 维护纪律

- 主手册只改本目录（唯一真相源）；stub 只维护入口。
- 模板改动必须在 Yocto 环境实际 build 过（至少 C 编译 + recipe parse）。
- 不要在手册里写死 `/home/xxx`；用 `<ROOT>`/`<BUILD>`/`<DEPLOY>`。
- 不把 app 模板塞进 BSP skill；BSP 与 app 分开，避免客户误改底层。
