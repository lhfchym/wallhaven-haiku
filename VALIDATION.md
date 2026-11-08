# Wallhaven 1.1.0 validation

Date: 2026-11-08. Haiku R1 beta6 hrev59866+79, x86_64, dedicated test VM.

Passed: strict HaikuPorter chroot build, package policy and TEST(); offline backend,
request-header, workspace-merge and catalog tests; catalog checks against the
installed package; live search, thumbnail/original download, cache reuse and
seeded pagination; native desktop application and rollback of test changes;
English default, legacy Chinese, language-menu restart and system-language UI;
current/selected/all workspace targeting; upgrade from 1.0.0-6, uninstall and
reinstall with preserved preferences/history/downloads/background.

The target workspace is captured before downloading. Non-target workspace images,
placement, origin and text outline are preserved. Tests include workspace bit 31,
repeated updates, missing optional fields and invalid input rejection.

The source archive is the exact input to the tested HaikuPorter build. The release
recipe changes its local test URI to the immutable release-asset URL, retaining
the checksum and build/install/test commands. The Git tag additionally updates
release documentation; application code, resources, catalogs and tests match the
build archive byte-for-byte. The public-source allowlist/private-data scan passed.
No real-account restricted-content API-key test was performed. No HaikuPorts
submission is part of this release.

SHA-256:

- `wallhaven-1.1.0-1-x86_64.hpkg`: `3395d367e7ab49d6bafd7c37aee30d8c90167c1b82143318ca00ca25a87a7dd4`

- `wallhaven-1.1.0.tar.gz`: `d56f833625e4b038adfc8b02bdac37b904b1afcbd1b19cbe400b7343e381bfdd`
