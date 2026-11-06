# shuhe-ostl-deploy 手册指针

本层(meta-st-openstlinux)**不存放手册正文**,唯一真相源在 **meta-st-stm32mp** 层。

完整手册位置(同构建树):

```
~/shuhe/layers/meta-st/meta-st-stm32mp/docs/shuhe-skill/SKILL.md
```

同目录还含 `CUSTOM_MAP.md` / `HARDWARE_MAP.md` / `ROLLBACK.md` / `templates/`。

- 部署、构建、板卡适配、改 dts 前,**先读该手册**,并遵守其第 0 节铁律
- 本层相关入口:镜像 recipe `recipes-st/images/shuhe-test-image-core.bb`(test 板)/
  `shuhe-image-core.bb`(DK 板),bootfs 修复行等见手册 §7
- 维护纪律:手册改动只改 meta-st-stm32mp 层对应目录,本文件只是指针

## MP2 / STM32MP257F-DK

MP2 手册不在本目录：见 `meta-st-stm32mp` 层的 `docs/shuhe-skill-mp2/SKILL.md`
（同目录 `HARDWARE_MAP.md` / `CUSTOM_MAP.md`）。257F-DK 不要套用本 MP1 手册的引脚。

## 上层应用开发（type 2 客户）

只做 app（网络/显示/开机自启），不改 kernel/uboot/dts → 读
`../shuhe-skill-app/SKILL.md`（skill 名 `shuhe-ostl-app`）。
