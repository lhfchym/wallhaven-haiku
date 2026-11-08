# Wallhaven for Haiku 1.1.0

**English** | [简体中文](RELEASE_NOTES.zh-CN.md)

Package revision 1 for Haiku R1 beta6 x86_64.

- Haiku catkeys localization, with embedded Simplified Chinese and English fallback.
  English remains the default; existing Chinese preferences are preserved.
  Settings offers English, 简体中文 and System language; restart to apply a change.
- Apply a wallpaper to the current workspace, a selected workspace, or all workspaces.
  Other backgrounds and placement settings are preserved. The current workspace is
  captured when you click, even if you switch workspaces during the download.
- Cleaned recipe: tests run in TEST(), with no duplicate BUILD test, clean step,
  separate debug package, or redundant objcopy prerequisite.

Quit Wallhaven before upgrading, then install:

```sh
pkgman install ./wallhaven-1.1.0-1-x86_64.hpkg
```

Assets: HPKG, exact tested build-source archive, pinned recipe, validation report,
and SHA256SUMS. Verify the checksums before installation. The Git tag adds current
release documentation and the published-source recipe to the tested application
sources; the attached source archive preserves the exact build input.

Validation passed on Haiku R1 beta6 hrev59866+79 x86_64: strict HaikuPorter chroot
build/package/tests, embedded catalog checks (including the installed HPKG), live
search/download/cache/pagination, workspace desktop integration, English/Chinese
UI checks, and 1.0.0-6 → 1.1.0-1 upgrade/uninstall/reinstall with preserved data.

Limitations: x86_64 only; no per-monitor background selection; image formats depend
on Haiku translators. Network failures and rate limits can interrupt downloads.
Real-account restricted-content authentication has not been tested. API keys are
stored locally without encryption and sent only in API request headers.
HaikuPorts inclusion is paused; this release does not imply inclusion or endorsement.

[Release](https://github.com/lhfchym/wallhaven-haiku/releases/tag/v1.1.0)
