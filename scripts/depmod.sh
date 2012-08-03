#!/bin/sh
#
# A depmod wrapper used by the toplevel Makefile

if test $# -ne 2; then
	echo "Usage: $0 /sbin/depmod <kernelrelease>" >&2
	exit 1
fi
DEPMOD=$1
KERNELRELEASE=$2

if ! "$DEPMOD" -V 2>/dev/null | grep -q module-init-tools; then
	echo "Warning: you may need to install module-init-tools" >&2
	echo "See http://www.codemonkey.org.uk/docs/post-halloween-2.6.txt" >&2
	sleep 1
fi

if ! test -r System.map -a -x "$DEPMOD"; then
	exit 0
fi
# older versions of depmod require the version string to start with three
# numbers, so we cheat with a symlink here
depmod_hack_needed=true
# arch decided to move everything to usr/lib instead of lib
# we can test for this and handle things with symlinks, if needed
depmod_arch_hack_needed=false
tmp_dir=$(mktemp -d ${TMPDIR:-/tmp}/depmod.XXXXXX)
mkdir -p "$tmp_dir/lib/modules/$KERNELRELEASE"
mkdir -p "$tmp_dir/usr/lib/modules/$KERNELRELEASE"
if "$DEPMOD" -b "$tmp_dir" $KERNELRELEASE 2>/dev/null; then
	if test -e "$tmp_dir/usr/lib/modules/$KERNELRELEASE/modules.dep" -o \
		-e "$tmp_dir/usr/lib/modules/$KERNELRELEASE/modules.dep.bin"; then
		depmod_arch_hack_needed=true
		rm -rf "$tmp_dir/lib"
		mv "$tmp_dir/usr/lib" "$tmp_dir/lib"
	fi
	if test -e "$tmp_dir/lib/modules/$KERNELRELEASE/modules.dep" -o \
		-e "$tmp_dir/lib/modules/$KERNELRELEASE/modules.dep.bin"; then
		depmod_hack_needed=false
	fi
fi
rm -rf "$tmp_dir"
if $depmod_hack_needed; then
	symlink="$INSTALL_MOD_PATH/lib/modules/99.98.$KERNELRELEASE"
	ln -s "$KERNELRELEASE" "$symlink"
	KERNELRELEASE=99.98.$KERNELRELEASE
fi
if $depmod_arch_hack_needed; then
	mkdir -p "$INSTALL_MOD_PATH/usr/lib/modules"
	ln -s ../../../lib/modules/$KERNELRELEASE \
		"$INSTALL_MOD_PATH/usr/lib/modules/$KERNELRELEASE"
fi

set -- -ae -F System.map
if test -n "$INSTALL_MOD_PATH"; then
	set -- "$@" -b "$INSTALL_MOD_PATH"
fi
"$DEPMOD" "$@" "$KERNELRELEASE"
ret=$?

if $depmod_hack_needed; then
	rm -f "$symlink"
fi

if $depmod_arch_hack_needed; then
	rm -f "$INSTALL_MOD_PATH/usr/lib/modules/$KERNELRELEASE"
	if [ -z "$(ls "$INSTALL_MOD_PATH/usr/lib/modules")" ]; then
		rm -rf "$INSTALL_MOD_PATH/usr/lib/modules"
	fi
	if [ -z "$(ls "$INSTALL_MOD_PATH/usr/lib")" ]; then
		rm -rf "$INSTALL_MOD_PATH/usr/lib"
	fi
	if [ -z "$(ls "$INSTALL_MOD_PATH/usr")" ]; then
		rm -rf "$INSTALL_MOD_PATH/usr"
	fi
fi

exit $ret
