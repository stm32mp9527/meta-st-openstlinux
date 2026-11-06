# <app-name> — customer application

## 源码

```text
<ROOT>/meta-customer-app/recipes-apps/<app-name>/files/
```

直接修改这里的 `.c` / `.py` / 资源文件即可，不需要手写交叉编译器命令。

## 构建

```bash
cd <BUILD>
unset BUILDDIR
source <ROOT>/layers/openembedded-core/oe-init-build-env .
bitbake <app-name>
```

> 镜像配置了 `INHERIT += "rm_work"`，`tmp-glibc/work/.../image/usr/bin` 可能已被清理。
> 部署请用 `tmp-glibc/deploy/deb/<arch>/<app-name>_*.deb`，不要依赖 work 目录。
>
> 如果 `<BUILD>` 路径含 `-test`：先取得逐项批准，或新建独立 app build；
> 不要默认对 protected 的 test 构建执行 `bitbake-layers add-layer` / 改 `local.conf`。

## 进镜像 / 开机自启

在 `local.conf` 或 image bbappend 里：

```conf
IMAGE_INSTALL:append = " <app-name>"
```

然后：

```bash
bitbake <image>
```

## 增量部署（有 SSH）

```bash
ARCH=<arch>   # MP1: cortexa7t2hf-neon-vfpv4；MP2: cortexa35；以实际目录为准
DEB=$(ls -t <BUILD>/tmp-glibc/deploy/deb/"$ARCH"/<app-name>_*.deb | head -1)
rm -rf /tmp/app-root && mkdir -p /tmp/app-root
dpkg-deb -x "$DEB" /tmp/app-root

# 显示 app 还有 customer-display.sh，用 * 一起拷；网络 app 只有一个二进制
scp /tmp/app-root/usr/bin/<app-name>* root@<board-ip>:/usr/bin/
scp /tmp/app-root/usr/lib/systemd/system/<app-name>.service root@<board-ip>:/etc/systemd/system/
ssh root@<board-ip> 'systemctl daemon-reload && systemctl enable --now <app-name>'
ssh root@<board-ip> 'systemctl status <app-name>'
```

## 验证（必须 reboot 后再测一次）

```bash
ssh root@<board-ip> 'systemctl reboot'
# 等板子起来
ssh root@<board-ip> 'systemctl status <app-name>'
ssh root@<board-ip> 'journalctl -u <app-name> -b'
```

网络 app：从 PC `nc <board-ip> <port>`。
显示 app：确认 Weston 正常，屏幕出现 app 窗口。

## 说明

- 这是上层 app，不修改 kernel/uboot/dts。
- 需要改分辨率/驱动/引脚/分区时，找 BSP skill `shuhe-ostl-deploy`。
