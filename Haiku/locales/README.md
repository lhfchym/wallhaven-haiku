# Translating Wallhaven

Wallhaven uses Haiku's `B_TRANSLATE` macros and catkeys catalogs. English is the
source language and the default for new installations. Settings offers English,
Simplified Chinese, and System language; changing it requires restarting the app.
The existing `zh-Hans` preference is mapped to Haiku's `zh_Hans` catalog identifier.
System language uses Haiku's preferred languages; missing translations fall back
to the English source strings.

On Haiku, run `make catkeys` after changing translatable strings. This regenerates
`en.catkeys` with `collectcatkeys`. Use Haiku's CatKeys editor or a tab-aware text
editor to update `zh_Hans.catkeys` or add another `<language>.catkeys` file. Keep
all source/context/comment keys and the generated fingerprint; translate only the
fourth column, and set the language in the header. Preserve placeholders such as
`%number%`, `%page%`, and ICU plural expressions. Do not translate identifiers,
URLs, MIME signatures, API values, or language self-names.

`make` embeds every non-English catalog using `linkcatkeys`; there is no runtime
dependency on this directory. `make test` checks the embedded Chinese catalog and
English fallback. Select System language to use additional catalogs. If a key is
added or removed, regenerate the English template and synchronize the translated
catalogs before building. `make clean` removes build products, not translations.

See [Haiku's translation guide](https://www.haiku-os.org/community/getting-involved/translating/3rdparty/).
