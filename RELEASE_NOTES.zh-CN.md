# Wallhaven for Haiku 1.0.0

[English](RELEASE_NOTES.md) | **简体中文**

首次公开发布，包修订号为 6，适用于 Haiku R1 beta6 x86_64。

通过 Haiku 原生界面浏览 wallhaven.cc：搜索和筛选壁纸、预览并保存原图、离线浏览已下载图片，以及将壁纸应用到所有工作区。首次启动默认英文，可在设置中切换为中文，已有语言选择会保留。

本版本包含仅用于 API 的 `X-API-Key` 请求头、兼容 chroot 的配置目录创建方式、MIT 许可证、静态链接的 Haiku JSON 组件版权声明，以及采用 MIT 许可证的原创演示素材。

已在 Haiku R1 beta6 hrev59866+79 x86_64 上完成 HaikuPorter chroot 构建和离线回归测试，验证真实搜索、缩略图与原图下载、缓存复用、分页、从包修订 5 升级、卸载重装后保留用户数据和壁纸、首次启动默认英文，以及保留已有中文设置。

安装命令：`pkgman install ./wallhaven-1.0.0-6-x86_64.hpkg`。升级前请退出应用。
发布附件包含 HPKG、固定提交的构建源码归档、recipe、验证摘要和 SHA256SUMS。

已知限制：网络/CDN 偶发错误；图片格式支持取决于 Haiku 图像转换器；壁纸变更作用于所有工作区；仅验证 x86_64。
真实账户的受限内容鉴权尚未测试。API Key 在本地以未加密形式保存。
本次发布不代表已被 HaikuPorts 收录，也不代表获得 Wallhaven 或 Haiku Project 的背书。

项目地址：https://github.com/lhfchym/wallhaven-haiku

发布页面：https://github.com/lhfchym/wallhaven-haiku/releases/tag/v1.0.0
