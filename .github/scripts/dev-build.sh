#!/usr/bin/env bash
set -euo pipefail

DS_SRC="${GITHUB_WORKSPACE:?}"
KOS_ROOT="/usr/local/dc/kos"
KOS_BASE="${KOS_ROOT}/kos"
KOS_PORTS="${KOS_ROOT}/kos-ports"
TOOLCHAIN_ROOT="/opt/toolchains/dc"

log() {
	printf '\n==> %s\n' "$*"
}

load_kos_env() {
	set +u
	# shellcheck disable=SC1091
	source "${KOS_BASE}/environ.sh"
	set -u
}

cmd="${1:-}"

case "${cmd}" in
prepare)
	sudo mkdir -p "${KOS_ROOT}" "${TOOLCHAIN_ROOT}"
	sudo chown -R "$(id -u):$(id -g)" "${KOS_ROOT}" "${TOOLCHAIN_ROOT}"

	KOS_SHA="$(tr -d '[:space:]' < "${DS_SRC}/sdk/doc/KallistiOS.txt")"
	if [[ -z "${KOS_SHA}" ]]; then
		echo "Pinned KallistiOS commit is empty (sdk/doc/KallistiOS.txt)" >&2
		exit 1
	fi

	log "Clone KallistiOS ${KOS_SHA}"
	rm -rf "${KOS_BASE}"
	git clone --filter=blob:none https://github.com/DC-SWAT/KallistiOS.git "${KOS_BASE}"
	if ! git -C "${KOS_BASE}" cat-file -e "${KOS_SHA}^{commit}"; then
		git -C "${KOS_BASE}" fetch --depth=1 origin "${KOS_SHA}"
	fi
	git -C "${KOS_BASE}" checkout --detach "${KOS_SHA}"

	ln -sfn "${DS_SRC}" "${KOS_BASE}/ds"
	cp "${DS_SRC}/sdk/toolchain/environ.sh" "${KOS_BASE}/environ.sh"
	cp "${DS_SRC}/sdk/toolchain/Makefile.cfg" "${KOS_BASE}/utils/kos-chain/Makefile.cfg"
	cp "${DS_SRC}/sdk/toolchain/patches/"*.diff "${KOS_BASE}/utils/kos-chain/patches/"
	;;

toolchain)
	log "Build SH4 toolchain"
	make -C "${KOS_BASE}/utils/kos-chain" build
	test -x "${TOOLCHAIN_ROOT}/sh-elf/bin/sh-elf-gcc"
	;;

kos)
	load_kos_env
	log "Build KallistiOS"
	make -C "${KOS_BASE}" -j"$(nproc)"
	;;

ports)
	load_kos_env
	log "Build kos-ports"
	(cd "${KOS_PORTS}" && ./utils/build-all.sh) || true
	test -f "${KOS_PORTS}/lib/libz.a"
	test -f "${KOS_PORTS}/lib/libpng.a"
	test -f "${KOS_PORTS}/lib/libjpeg.a"
	;;

release)
	load_kos_env
	log "Build DreamShell SDK host tools"
	make -C "${DS_SRC}/sdk/bin/src"
	if ! make -C "${DS_SRC}/sdk/bin/src" install; then
		log "SDK install failed, copying cdi4dc only"
		install -m 755 "${DS_SRC}/sdk/bin/src/img4dc/build/cdi4dc/cdi4dc" "${DS_SRC}/sdk/bin/cdi4dc"
	fi

	ln -nsf "$(command -v lua5.2 || command -v lua)" "${DS_SRC}/sdk/bin/lua"
	ln -nsf "$(command -v tolua)" "${DS_SRC}/sdk/bin/tolua"
	ln -nsf "$(command -v mkisofs || command -v genisoimage)" "${DS_SRC}/sdk/bin/mkisofs"
	ln -nsf "$(command -v mksquashfs)" "${DS_SRC}/sdk/bin/mksquashfs"
	ln -nsf "$(command -v zip)" "${DS_SRC}/sdk/bin/zip"

	log "Build DreamShell release"
	make -C "${DS_SRC}" release

	zip_path="$(find "${DS_SRC}/release" -maxdepth 1 -name '*.zip' -print | head -n 1)"
	if [[ -z "${zip_path}" ]]; then
		echo "Release zip was not created" >&2
		ls -la "${DS_SRC}/release" >&2 || true
		exit 1
	fi

	cp -f "${zip_path}" "${DS_SRC}/DreamShell-dev.zip"
	log "Packaged ${zip_path} -> DreamShell-dev.zip"
	ls -lh "${DS_SRC}/DreamShell-dev.zip"
	;;

*)
	echo "Usage: $0 {prepare|toolchain|kos|ports|release}" >&2
	exit 2
	;;
esac
