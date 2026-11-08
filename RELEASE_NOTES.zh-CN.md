# Wallhaven for Haiku 1.1.0

[English](RELEASE_NOTES.md) | **简体中文**

适用于 Haiku R1 beta6 x86_64，安装包修订号为 1。

- 接入 Haiku catkeys，内嵌简体中文翻译，缺少翻译时回退英文。新安装仍默认英文，保留原有中文偏好。设置可选择 English、简体中文、跟随系统语言，重启后生效。
- 支持将壁纸应用到当前、指定或全部工作区，保留其他工作区的图片和显示方式。当前工作区在点击时确定，下载期间切换工作区不会改变目标。
- 整理 recipe：测试统一由 TEST() 执行，去掉 BUILD 中的重复测试、清理步骤、独立调试包及多余的 objcopy 声明。

升级前请退出 Wallhaven，然后安装：

```sh
pkgman install ./wallhaven-1.1.0-1-x86_64.hpkg
```

附件包括 HPKG、实际用于测试构建的源码归档、固定源码地址的 recipe、验证报告和 SHA256SUMS。安装前可核对校验值。Git 标签在经过测试的应用源码上补充了本次发布文档与正式下载地址；源码附件保留精确的构建输入。

已在 Haiku R1 beta6 hrev59866+79 x86_64 验证：严格 HaikuPorter chroot 构建、打包和测试；内嵌翻译（含已安装 HPKG）；真实搜索、原图下载、缓存和分页；工作区桌面操作；中英文界面；1.0.0-6 → 1.1.0-1 升级、卸载和重装，保留用户数据及背景。

限制：仅验证 x86_64；不支持按显示器分别设置；图片格式支持取决于 Haiku translators。网络故障和限流可能影响下载。未使用真实账户 API Key 验证受限内容；密钥在本机未加密保存，仅通过 API 请求头发送。

HaikuPorts 收录继续暂停，本次发布不代表已被收录或获得官方认可。

[版本发布](https://github.com/lhfchym/wallhaven-haiku/releases/tag/v1.1.0)
