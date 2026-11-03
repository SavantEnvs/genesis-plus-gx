#!/usr/bin/env bash
#
# mayhem/build.sh — builds Genesis Plus GX (libretro core, static) twice — sanitized for the fuzz
# harnesses, normal flags for the oracle probe — then the three harnesses and their -standalone
# reproducers. Offline, idempotent, from a clean tree.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

: "${SRC:=/mayhem}"
cd "$SRC"

OUT=/mayhem
B="$SRC/mayhem/build"
INC="-I$SRC/libretro/libretro-common/include -I$SRC/core/cd_hw/libchdr/include"
mkdir -p "$B"

# Makefile.libretro appends its own flags to the *environment* CFLAGS (a command-line CFLAGS would
# discard them), so pass ours that way. GIT_VERSION is pinned so the build never shells out to git.
build_core() {
  local archive="$1" cflags="$2" objs
  make -f Makefile.libretro clean-objs >/dev/null
  # STATIC_LINKING=1 would leave out libretro-common (VFS) and the bundled zlib, which a frontend
  # normally supplies; build every object the shared-library target would link and archive them.
  objs=$(CFLAGS="$cflags" make -s -f Makefile.libretro GIT_VERSION=" mayhem" print-OBJECTS | sed 's/^OBJECTS=//')
  # shellcheck disable=SC2086
  CFLAGS="$cflags" make -f Makefile.libretro -j"$MAYHEM_JOBS" GIT_VERSION=" mayhem" $objs
  rm -f "$archive"
  # shellcheck disable=SC2086
  ar rcs "$archive" $objs
  make -f Makefile.libretro clean-objs >/dev/null
}

# 1) Oracle probe: the core with its NORMAL flags (no sanitizers), plus a plain dynamically linked
#    driver — dynamic on purpose, so the sabotage check's LD_PRELOAD _exit(0) shim can intercept it.
build_core "$B/libgpgx_normal.a" "$COVERAGE_FLAGS"
$CC -O1 $COVERAGE_FLAGS $INC "$SRC/mayhem/kat/kat.c" "$B/libgpgx_normal.a" -lm -o "$B/kat"

# 2) Fuzzed code: the core with sanitizers + DWARF 3. fuzzer-no-link adds libFuzzer's coverage
#    instrumentation to the core itself without linking the engine into the archive.
# UBSan checks relaxed on the emulator core only (harnesses keep everything on): see RELAX.
# `function`: libchdr hands LzmaEnc/LzmaDec an ISzAlloc whose Alloc has a different signature
# (lzma_fast_alloc(void*,size_t)), which trips on every LZMA/CDLZ CHD at open, before any hunk is read.
RELAX="-fno-sanitize=shift,alignment,signed-integer-overflow,function"
build_core "$B/libgpgx_san.a" "$SANITIZER_FLAGS $DEBUG_FLAGS $RELAX -fsanitize=fuzzer-no-link"

# 3) Harnesses: libFuzzer binaries + run-once standalone reproducers.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$B/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"

harness() {
  local name="$1" src="$2"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE $INC "$src" "$B/libgpgx_san.a" "$B/lsan_off.o" -lm -o "$OUT/$name"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $INC "$B/standalone_main.o" "$src" "$B/libgpgx_san.a" "$B/lsan_off.o" -lm -o "$OUT/$name-standalone"
}

harness fuzz_chd "$SRC/mayhem/harness/fuzz_chd.c"
harness fuzz_rom "$SRC/mayhem/harness/fuzz_rom.c"
harness fuzz_state "$SRC/mayhem/harness/fuzz_state.c"
