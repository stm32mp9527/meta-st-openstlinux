# TEST_RESULTS — shuhe-ostl-app

> 记录本 skill 的模板/recipe 实际测试范围，避免把“编译通过”说成“MP1 上板验证通过”。

## 已做

| 项目 | 环境 | 结果 |
|------|------|------|
| `tcp-server.c` 本机编译/运行 | x86_64 gcc `-Wall -Wextra -Werror` | 编译通过；TCP 回环 `hello`→`OK`；SIGTERM 干净退出 |
| `tcp-server` Yocto 交叉编译 | `MACHINE=stm32mp2-m33td`，scarthgap，aarch64 | `tcp-server_1.0-r0.0_arm64.deb`；`/usr/bin/tcp-server` + service + preset；`Depends: libc6` |
| `battery-monitor.c` 本机 mock 测试 | x86_64 gcc + mock sysfs | 4.000V/0.500A/25.0C/SOC 83.3%；CSV 写入；SIGTERM 干净退出 |
| `battery-monitor` Yocto 交叉编译 | 同上 | `battery-monitor_1.0-r0.0_arm64.deb`；`/usr/bin/battery-monitor` + service/preset；`Depends: libc6` |
| `customer-display` Yocto 交叉编译 | 同上 | `customer-display_1.0-r0.2_arm64.deb`；`/usr/bin/customer-display` + `customer-display.sh` + service；`Depends: libsdl2-2.0-0`；`NEEDED libSDL2-2.0.so.0` |
| `.deb` service/wrapper 核对 | 解包实际 `.deb` | service = `User=weston`、`ExecStart=/usr/bin/customer-display.sh`、`WantedBy=multi-user.target`；wrapper = `UID_W=$(id -u)`，查 `/run/user/$UID_W` 和 `/home/weston`，wayland-0/1，`/opt/logo.bmp` |
| MP1 core 镜像能力 | `build-openstlinuxweston-stm32mp13-disco-test` manifest/rootfs | 无 Weston/Python/SDL2；有 `systemd-networkd` + `80-wired.network`（DHCP=yes） |
| MP1 `st-image-weston` SDL2 | packagegroup 检查 | `packagegroup-framework-core-base-display` 在 `opengl` 下依赖 `libsdl2`/`libsdl2-image`；MP2 Weston manifest 已见 SDL2 |
| AI 导航测试 | 全新 subagent | 第一轮发现 Wayland/重启/图片/网络误判等问题，已修；电池工程师场景最终审查：功能内容 PASS |

## 未做（交付前需补）

- **未在 MP1 板子上实际烧录/运行**：没有可用板子/显示/网络现场。
- **未构建 MP1 的 `st-image-weston`**：SDL2 结论来自 packagegroup + MP2 manifest；
  客户现场首次构建 MP1 Weston 镜像后，需按 `SKILL.md` §6.3 做 reboot + 屏幕验证。
- **测试 `.deb` 是 MP2 `cortexa35`/arm64**：MP1 应构建
  `cortexa7t2hf-neon-vfpv4`/armhf 的 `.deb`；service/wrapper 与架构无关，但二进制不能混用。
- **未验证真实 Wayland socket 路径**：wrapper 同时兼容 `/run/user/<uid>` 和
  `/home/weston`；上板需 `ls -l` 确认实际路径。

## 结论

- recipe/模板/C 代码已通过真实 Yocto（MP2/aarch64 测试树）交叉编译和 `.deb` 拆包检查。
- 显示 app 的“重启自启/图片显示/全屏缩放/Wayland 环境”修复已在模板和 `.deb` 中生效。
- **上板前**必须按 `SKILL.md` §6.3 做一次 reboot + 实际屏幕验证；MP1 首次构建
  `st-image-weston` 后补一次 MP1 `.deb`/rootfs 检查。
