# 1.1.0 — 2026-11-08

- 使用 Haiku catkeys：内嵌英文源文案及简体中文翻译，支持跟随系统语言与英文回退。
- 增加当前、指定、全部工作区壁纸目标，保留其他工作区设置。
- 整理 recipe：BUILD 中只构建，测试集中在 TEST，移除独立调试包和多余的 objcopy 声明。
- 补充工作区合并、目录资源和桌面集成回归测试。

# Changes

## Package revision 6 — unreleased preparation

- Include MIT notices for statically linked Haiku JSON components.
- Prepare an allowlisted Haiku-only public source snapshot and English release materials.


## 1.0.0, package revision 5 — 2026-11-02

- Default to English on fresh installations or when no language is saved.
- Preserve existing explicit Chinese/English preferences.


## 1.0.0, package revision 4 — 2026-11-02

- Fix settings/cache/download directory creation inside HaikuPorter chroots.
- Restore full offline regression tests during isolated builds.
- Send API keys using X-API-Key headers, restricted to the Wallhaven API.
- Reject control characters in keys and test anonymous/CDN request isolation.
- Revalidate live original downloads and package installation lifecycle.

## 1.0.0, package revision 2 — 2026-11-02

- Adopt MIT licensing for project code and original icons.
- Add English installation, privacy and support documentation.
- Install documentation under documentation/packages/wallhaven.
- Declare the application-created user settings directory in package metadata.
- Add DESTDIR-based staged installation; remove debug information from the default build.
- Add repeatable HPKG upgrade, uninstall, reinstall and data-preservation checks.
- Keep revision 1 and its internal release unchanged.
