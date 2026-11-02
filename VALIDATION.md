# Release validation — 1.0.0, package revision 6

Validation completed 2026-11-02 on Haiku R1 beta6 hrev59866+79, x86_64,
in a separate VM installed from the official beta6 ISO. Development dependencies
were present for these tests; this was not a second newly provisioned OS for revision 6.

## Exact build inputs and output

- Build source commit: `c93b11b5c6783e0b3fdb97fc797018f1bc9ac963`.
- Source URL: https://github.com/lhfchym/wallhaven-haiku/archive/c93b11b5c6783e0b3fdb97fc797018f1bc9ac963.tar.gz
- Source SHA-256: `040c50feccaadd2d83e13737dc1d8c87ad830a09c675e498c66ed5d3549337d0`.
- HPKG SHA-256: `3573b0b9c96b77e1517ba262e39bde69d90291ebac30e88d551bf50c13435db8`.
- Recipe SHA-256: `674b300440d05fca98fb9fadd7d5230a2a896e5601add2cabc0da3808487a372`.
- HaikuPorter revision: `601d31b506386466f6155305fdfe86a1c470fda5`.
- Build: `haikuporter wallhaven --clean`, then `haikuporter wallhaven -j4`;
  normal chroot enabled, `make test` executed inside BUILD().

The attached source archive contains the exact build input. The release tag also
includes this validation record and the pinned recipe, added after the build.
No application source changes were made between the build commit and release tag.

## Results

- Public source manifest hashes and private-address/key-pattern scan: PASS.
- Normal chroot build, dependency resolution and package creation: PASS.
- JSON parsing, item isolation, URL validation, fit scoring and pagination: PASS.
- Isolated settings, Unicode paths, permissions and history deduplication: PASS.
- Actual CURL request configuration: API-only X-API-Key, anonymous requests,
  CDN isolation and injection rejection: PASS (synthetic key, no account login).
- Live search (24 results), thumbnail and original download, cache reuse,
  offline enumeration and seeded second page: PASS on final build source.
- Revision 5 install, upgrade to the final revision 6 HPKG, uninstall and reinstall:
  PASS; settings and downloaded file hashes preserved, background attribute unchanged.
- Packaged application launch and IPC response: PASS.
- Fresh profile: English Settings label; restored saved profile: Chinese label: PASS.
- Installed THIRD_PARTY_NOTICES.txt and README.en.md match build source: PASS.
- Standard app/documentation paths and Deskbar launch target: PASS.

Checksums describe this tested artifact, not a promise of bit-for-bit reproducibility
on every toolchain. No real-account restricted-content authentication or 32-bit
platform validation is claimed. The project has not been accepted into HaikuPorts.
