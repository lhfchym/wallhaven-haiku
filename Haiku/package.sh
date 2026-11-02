#!/bin/sh
# Build a native Haiku package without installing it or modifying user settings.
set -eu

if [ "$(uname -s)" != Haiku ]; then
    echo "Run this script inside Haiku." >&2
    exit 1
fi

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
architecture=$(uname -m)
if [ "$architecture" != x86_64 ]; then
    echo "This package definition currently targets Haiku x86_64." >&2
    exit 1
fi

cd "$script_dir"
make
dist_dir="$script_dir/dist"
mkdir -p "$dist_dir"
staging_dir=$(mktemp -d "$dist_dir/.wallhaven-package.XXXXXX")
trap 'rm -rf "$staging_dir"' EXIT HUP INT TERM

make install-staged DESTDIR="$staging_dir" PREFIX=
mkdir -p "$staging_dir/data/deskbar/menu/Applications" "$staging_dir/data/licenses"
mimeset -f "$staging_dir/apps/Wallhaven/Wallhaven"
ln -s ../../../../apps/Wallhaven/Wallhaven \
    "$staging_dir/data/deskbar/menu/Applications/Wallhaven"
copyattr -n BEOS:M:STD_ICON "$staging_dir/apps/Wallhaven/Wallhaven" \
    "$staging_dir/data/deskbar/menu/Applications/Wallhaven"
copyattr -n BEOS:L:STD_ICON "$staging_dir/apps/Wallhaven/Wallhaven" \
    "$staging_dir/data/deskbar/menu/Applications/Wallhaven"

cp LICENSE "$staging_dir/data/licenses/Wallhaven-MIT"

cat > "$staging_dir/.PackageInfo" <<'EOF'
name wallhaven
version 1.0.0-6
architecture x86_64
summary "Browse and apply wallpapers from wallhaven.cc"
description "Browse, preview and download Wallhaven images with a native Haiku interface. Set the desktop background on all workspaces, browse downloaded images offline, and configure filters in Chinese or English."
packager "lhfchym"
vendor "lhfchym"
copyrights {
    "2026 lhfchym"
}
licenses {
    "Wallhaven-MIT"
}
urls {
    "https://github.com/lhfchym/wallhaven-haiku"
}
user-settings-files {
    "settings/Wallhaven" directory
}
provides {
    wallhaven = 1.0.0
    app:Wallhaven = 1.0.0
}
requires {
    haiku >= r1~beta6
    lib:libcurl >= 4.8.0
    lib:libstdc++ >= 6.0.32
    lib:libgcc_s >= 1
}
EOF

package_path="$dist_dir/wallhaven-1.0.0-6-$architecture.hpkg"
package create -C "$staging_dir" "$package_path"
package list -i "$package_path"
echo "Created $package_path"
