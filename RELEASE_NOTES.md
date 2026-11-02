# Wallhaven for Haiku 1.0.0

**English** | [简体中文](RELEASE_NOTES.zh-CN.md)

First public release, package revision 6, for Haiku R1 beta6 x86_64.

Browse wallhaven.cc with a native Haiku interface: search and filter images,
preview and save originals, browse downloaded images offline, and apply a
wallpaper across all workspaces. English is the default; Chinese remains
available in Settings, and existing language preferences are preserved.

This release includes API-only X-API-Key request headers, chroot-compatible
settings directory creation, MIT licensing, attribution for linked Haiku JSON
components, and original MIT-licensed demonstration artwork.

Validation on Haiku R1 beta6 hrev59866+79 x86_64 covered HaikuPorter chroot
builds and offline regression tests, live search/thumbnail/original download,
cache reuse and pagination, upgrade from package revision 5, uninstall and
reinstall with user data and background preservation, English first launch,
and preservation of an existing Chinese preference.

Install: `pkgman install ./wallhaven-1.0.0-6-x86_64.hpkg`. Quit the application
before upgrading. Release assets include the HPKG, pinned source archive,
recipe, validation summary and SHA256SUMS.

Known limitations: intermittent network/CDN errors; image support depends on
Haiku translators; background changes apply to all workspaces; x86_64 only.
Real-account restricted-content authentication has not been tested. API keys
are stored locally without encryption. This is not a HaikuPorts inclusion
or an endorsement by Wallhaven or the Haiku Project.

Project: https://github.com/lhfchym/wallhaven-haiku
Release: https://github.com/lhfchym/wallhaven-haiku/releases/tag/v1.0.0
