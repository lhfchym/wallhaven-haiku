#!/bin/sh
# Opt-in integration test on a disposable/test Haiku VM. Requires no installed wallhaven HPKG.
# Temporarily hides the developer install; restores it on exit. Does not delete user data.
set -eu
cd "$(dirname "$0")/.."
old=${1:-dist/wallhaven-1.0.0-1-x86_64.hpkg}
new=${2:-dist/wallhaven-1.0.0-2-x86_64.hpkg}
test -f "$old" && test -f "$new"
old_name=$(package info -f "%fileName%" "$old")
new_name=$(package info -f "%fileName%" "$new")
test "$old_name" != "$new_name"
test ! -e /boot/system/apps/Wallhaven/Wallhaven
hey application/x-vnd.lhfchym-Wallhaven QUIT >/dev/null 2>&1 || true
sleep 2
work=$(mktemp -d /boot/home/wallhaven-package-test.XXXXXX)
dev=/boot/home/config/non-packaged/apps/Wallhaven
menu=/boot/home/config/settings/deskbar/menu/Applications/Wallhaven
restore() {
    if [ -e /boot/system/apps/Wallhaven/Wallhaven ]; then pkgman uninstall -y wallhaven; fi
    if [ -d "$work/dev" ]; then mv "$work/dev" "$dev"; fi
    if [ -L "$work/menu" ]; then mv "$work/menu" "$menu"; fi
    echo "Evidence retained at $work"
}
trap restore EXIT
if [ -d "$dev" ]; then mv "$dev" "$work/dev"; fi
if [ -L "$menu" ]; then mv "$menu" "$work/menu"; fi
fingerprint() {
    find /boot/home/config/settings/Wallhaven /boot/home/Pictures/Wallhaven -type f -exec sha256sum '{}' \; | sort
}
fingerprint > "$work/before.sha256"
catattr -r be:bgndimginfo /boot/home/Desktop > "$work/background.before"
pkgman install -y -R "$old"
test -x /boot/system/apps/Wallhaven/Wallhaven
echo 'PASS: baseline package install'
pkgman install -y -R "$new"
test -f "/boot/system/packages/$new_name"
test ! -f "/boot/system/packages/$old_name"
test -f /boot/system/documentation/packages/wallhaven/README.en.md
test -f /boot/system/data/licenses/Wallhaven-MIT || test -f /boot/system/data/licenses/MIT
test -x /boot/system/data/deskbar/menu/Applications/Wallhaven
fingerprint > "$work/upgraded.sha256"
cmp "$work/before.sha256" "$work/upgraded.sha256"
echo 'PASS: upgrade, standard layout, Deskbar target, preserved data'
pkgman uninstall -y wallhaven
test ! -e /boot/system/apps/Wallhaven/Wallhaven
test ! -e /boot/system/data/deskbar/menu/Applications/Wallhaven
fingerprint > "$work/uninstalled.sha256"
cmp "$work/before.sha256" "$work/uninstalled.sha256"
echo 'PASS: uninstall removes application and menu, preserves user data'
pkgman install -y -R "$new"
fingerprint > "$work/reinstalled.sha256"
cmp "$work/before.sha256" "$work/reinstalled.sha256"
catattr -r be:bgndimginfo /boot/home/Desktop > "$work/background.after"
cmp "$work/background.before" "$work/background.after"
/boot/system/apps/Wallhaven/Wallhaven > "$work/app.log" 2>&1 &
sleep 8
hey application/x-vnd.lhfchym-Wallhaven GET Text of View status of View center of Window 0
hey application/x-vnd.lhfchym-Wallhaven QUIT
sleep 2
echo 'PASS: reinstall preserves data/background and packaged application responds'
