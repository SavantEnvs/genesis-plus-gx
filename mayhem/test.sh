#!/usr/bin/env bash
#
# mayhem/test.sh — behavioral oracle for Genesis Plus GX (the probe is built by mayhem/build.sh).
# exit 0 = pass. The probe boots a hand-assembled Mega Drive ROM through the real core and this script
# compares what it observed with known answers.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

PROBE="$SRC/mayhem/build/kat"
if [ ! -x "$PROBE" ]; then
  echo "missing $PROBE — mayhem/build.sh must build it" >&2
  emit_ctrf "genesis-plus-gx-kat" 0 1 0
  exit 1
fi

# Expected observations, derived from the test ROM's 68000 program (mayhem/harness/testrom.h):
#  - it sums 1..100 and stores the long at $FF0000 (work RAM)            -> 5050
#  - it enables the display and writes CRAM[0] = $0E00 (backdrop colour)  -> 256x224 frame, RGB565 pixel
#  - a savestate round-trip must reproduce the exact state and RAM
EXPECTED=(
  "sum=5050"
  "video=256x224 pixel=0x0004"
  "state_roundtrip=identical"
  "sum_after_restore=5050"
)

OUTPUT="$(cd "$(mktemp -d)" && "$PROBE" 2>&1)" || true
echo "$OUTPUT"

passed=0; failed=0
for want in "${EXPECTED[@]}"; do
  if printf '%s\n' "$OUTPUT" | grep -qxF -- "$want"; then
    passed=$((passed + 1))
  else
    echo "FAIL: expected line '$want'" >&2
    failed=$((failed + 1))
  fi
done

emit_ctrf "genesis-plus-gx-kat" "$passed" "$failed" 0
