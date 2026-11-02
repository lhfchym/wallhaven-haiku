# Wallhaven for Haiku 1.0.0 — public release draft

Browse wallhaven.cc from a native Haiku application. Search and filter images,
preview and save originals, browse your local collection offline, and set a
wallpaper across all workspaces. The interface starts in English and can be
switched to Chinese.

The prepared implementation includes API-key request headers scoped to the API,
settings persistence that works in HaikuPorter chroots, and package lifecycle
tests that preserve user data. The next package revision adds third-party
attribution to the installed documentation.

Test baseline: Haiku R1 beta6 hrev59866+79, x86_64. Earlier internal builds passed
isolated compilation, offline regression tests, live original downloads, preview
and background application, plus installation/upgrade/removal/reinstallation.
The final public artifact must be rebuilt and checked after its URLs and source
tag are fixed; those results are not implied by earlier internal tests.

Known limitations include intermittent network/CDN errors, dependence on Haiku
image translators, all-workspace background changes, and no real-account test of
restricted-content authorization. API keys are stored locally without encryption.

Project: https://github.com/lhfchym/wallhaven-haiku

Download filenames and SHA-256 checksums will be filled from
the actual release assets. No public release has been made from this draft.
