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
