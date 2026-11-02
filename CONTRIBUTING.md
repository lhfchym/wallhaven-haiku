# Contributing

Report bugs at https://github.com/lhfchym/wallhaven-haiku/issues.
Include the Haiku revision, architecture, Wallhaven package revision, reproduction
steps, expected result and actual error text. Explain whether it also happens
with a local image. Remove API keys, private paths and unrelated personal content
from logs or screenshots. Do not upload copyrighted wallpapers without permission.

For code changes, explain the behavior being changed and run `make test` on Haiku.
For networking changes, also run `make test-live` and document failures rather
than suppressing them. Preserve English and Chinese UI strings. Do not introduce
real credentials into fixtures: the existing test keys are synthetic.

Before an HPKG release, run a clean HaikuPorter build with its offline tests and
validate install, upgrade, uninstall and reinstall on a test VM. Verify that
settings, downloads and the current background survive package removal. Review
third-party notices when changing static dependencies or build versions.

Project contributions are intended to use the MIT license. Keep upstream notices
with copied code and document the provenance of new assets.
