#!/bin/bash
#
# Verifies which crypto backend a build actually linked, from the artifacts alone.
#
#   tst/scripts/check_crypto_backend.sh <openssl|aws-lc> <build-dir> <crypto-lib-dir>
#
# <crypto-lib-dir> is required: the directory holding the libcrypto the SDK was
# built against (for builds that compile their dependencies, <repo>/open-source/local/lib).
# The script does not guess it; a missing argument is a usage error and a
# directory without a libcrypto is a FAIL.
#
# The C++ SDK has no crypto code of its own; the TLS path (including the AWS-LC
# post-quantum key exchange) comes from producer-c, which this build compiles as
# a sub-project. The producer-c libraries land under <build-dir>/kvscproducer-build
# (LOCAL_KVSC_PATH builds) or <build-dir>/dependency/libkvscproducer/kvscproducer-src
# (fetched builds); this script finds them in either place.
#
# Checks, all of which must agree with the requested backend:
#   1. The AWS-LC-only TLS code (kvsCurlSslCtxCallback, which installs the PQC
#      groups list) is present in the producer-c libraries for aws-lc and ABSENT
#      for openssl. This is the "default build is inert" guarantee: adding the
#      USE_AWS_LC option must not change builds that did not ask for it.
#   2. Same check on the C++ artifacts that re-package that code, when they are
#      shared libraries (libKinesisVideoProducer.so/.dylib, libgstkvssink.*):
#      a shared C++ library must carry the callback for aws-lc and not for openssl.
#      Static libKinesisVideoProducer.a holds only C++ objects, so it is skipped.
#   3. The crypto library the SDK was built against identifies itself as AWS-LC
#      (version string "AWS-LC x.y.z") or as OpenSSL. The libcrypto is taken
#      from <crypto-lib-dir>.
#   4. The headers producer-c compiled against: producer-c embeds
#      OPENSSL_VERSION_TEXT in libkvsCommonCurl (TLS evidence log line), which
#      reads "... AWS-LC x.y.z ..." when built against AWS-LC headers. aws-lc
#      requires that string; openssl requires its absence.
#   5. openssl only, Linux only: a shared default build must link OpenSSL
#      (system or bundled) dynamically, so ldd on libgstkvssink.so and
#      libKinesisVideoProducer.so must resolve both libssl and libcrypto with no
#      "not found" entries. Catches a default build that lost its TLS stack.
#      Skipped for static-only builds.
#
# Works on Linux and macOS, for shared and static builds (nm handles both). For a
# cross-compiled build set NM and STRINGS to the target toolchain's binutils.
# Exit code 0 when every check agrees, 1 otherwise.

set -u

EXPECTED="${1:-}"
BUILD_DIR="${2:-}"
CRYPTO_LIB_DIR="${3:-}"
NM="${NM:-nm}"
STRINGS="${STRINGS:-strings}"

if [[ "$EXPECTED" != "openssl" && "$EXPECTED" != "aws-lc" ]] || [[ -z "$BUILD_DIR" || -z "$CRYPTO_LIB_DIR" ]]; then
  echo "usage: $0 <openssl|aws-lc> <build-dir> <crypto-lib-dir>" >&2
  echo "  <crypto-lib-dir>: directory with the libcrypto the build linked (e.g. ./open-source/local/lib)" >&2
  exit 2
fi

FAILED=0
pass() { echo "PASS: $*"; }
fail() { echo "FAIL: $*"; FAILED=1; }

has_callback() {
  # Plain nm (symtab, not just exported/dynamic symbols): AWS-LC builds link the
  # plugin with --exclude-libs,ALL, which demotes the callback to a local symbol.
  "$NM" "$1" 2>/dev/null | grep -q 'kvsCurlSslCtxCallback'
}

readable_symbols() {
  # Positive control: nm must be able to list symbols at all, otherwise the
  # absence checks below would pass vacuously (broken NM override, opaque LTO).
  # grep -q .: any non-empty output line (nm prints a blank first line for
  # archives on macOS, so head -1 would be wrong here)
  "$NM" "$1" 2>/dev/null | grep -q .
}

# --- 1. producer-c library symbols ---------------------------------------------
KVSC_LIBS=$(find "$BUILD_DIR" -maxdepth 4 \
  \( -name 'libkvsCommonCurl.*' -o -name 'libcproducer.*' \) 2>/dev/null \
  | grep -E '\.(a|so|dylib)$' || true)
if [[ -z "$KVSC_LIBS" ]]; then
  fail "no producer-c libraries (libkvsCommonCurl, libcproducer) found under $BUILD_DIR"
else
  HITS=0
  while IFS= read -r lib; do
    [[ -n "$lib" ]] || continue
    if ! readable_symbols "$lib"; then
      fail "cannot read any symbols from $lib; the symbol checks would be vacuous"
      continue
    fi
    if has_callback "$lib"; then HITS=$((HITS + 1)); fi
  done <<< "$KVSC_LIBS"
  if [[ "$EXPECTED" == "aws-lc" ]]; then
    if [[ "$HITS" -gt 0 ]]; then
      pass "kvsCurlSslCtxCallback (PQC groups list) present in the producer-c libraries"
    else
      fail "kvsCurlSslCtxCallback missing: the AWS-LC build did not compile the PQC TLS path"
    fi
  else
    if [[ "$HITS" -eq 0 ]]; then
      pass "no AWS-LC-only symbols in the producer-c libraries (default build is unaffected by USE_AWS_LC)"
    else
      fail "kvsCurlSslCtxCallback found in an OpenSSL build: AWS-LC code leaked into the default build"
    fi
  fi
fi

# --- 2. C++ shared artifacts -----------------------------------------------------
CPP_SHARED=$(ls "$BUILD_DIR"/libKinesisVideoProducer.so "$BUILD_DIR"/libKinesisVideoProducer.dylib \
  "$BUILD_DIR"/libgstkvssink.so "$BUILD_DIR"/libgstkvssink.dylib 2>/dev/null || true)
if [[ -z "$CPP_SHARED" ]]; then
  echo "SKIP: no shared C++ artifacts in $BUILD_DIR (static-only build)"
else
  while IFS= read -r lib; do
    [[ -n "$lib" ]] || continue
    if ! readable_symbols "$lib"; then
      fail "cannot read any symbols from $lib; the symbol checks would be vacuous"
      continue
    fi
    if has_callback "$lib"; then
      if [[ "$EXPECTED" == "aws-lc" ]]; then
        pass "$(basename "$lib") carries the PQC TLS path"
      else
        fail "$(basename "$lib") carries AWS-LC-only code in an OpenSSL build"
      fi
    else
      if [[ "$EXPECTED" == "aws-lc" ]]; then
        # Only shared artifacts that package producer-c objects must carry it;
        # a shared lib built against shared producer-c libs legitimately may not.
        echo "NOTE: $(basename "$lib") does not embed kvsCurlSslCtxCallback (links producer-c dynamically?)"
      else
        pass "$(basename "$lib") has no AWS-LC-only code"
      fi
    fi
  done <<< "$CPP_SHARED"
fi

# --- 3. Crypto library identity -------------------------------------------------
CRYPTO_LIB=""
if [[ ! -d "$CRYPTO_LIB_DIR" ]]; then
  fail "crypto lib dir not found: $CRYPTO_LIB_DIR"
else
  CRYPTO_LIB=$(find "$CRYPTO_LIB_DIR" -maxdepth 1 -name 'libcrypto.*' 2>/dev/null \
    | grep -E '\.(a|so|dylib)(\.[0-9.]+)?$' | sort | head -1 || true)
  [[ -n "$CRYPTO_LIB" ]] || fail "no libcrypto found in $CRYPTO_LIB_DIR"
fi
if [[ -n "$CRYPTO_LIB" ]]; then
  VERSION_TEXT=$("$STRINGS" "$CRYPTO_LIB" | grep -m1 -E 'AWS-LC [0-9]+\.[0-9]+\.[0-9]+|^OpenSSL [0-9]+\.[0-9]+\.[0-9]+' || true)
  echo "libcrypto: $CRYPTO_LIB"
  echo "libcrypto version string: ${VERSION_TEXT:-<none>}"
  if [[ "$EXPECTED" == "aws-lc" ]]; then
    if [[ "$VERSION_TEXT" == *"AWS-LC"* ]]; then
      pass "libcrypto is AWS-LC"
    else
      fail "libcrypto is not AWS-LC"
    fi
  else
    if [[ "$VERSION_TEXT" == OpenSSL* && "$VERSION_TEXT" != *"AWS-LC"* ]]; then
      pass "libcrypto is OpenSSL"
    else
      fail "libcrypto is not plain OpenSSL (an AWS-LC library may have been picked up)"
    fi
  fi
fi

# --- 4. Headers producer-c compiled against -----------------------------------
CURL_LIB=$(echo "$KVSC_LIBS" | grep 'libkvsCommonCurl\.' | head -1 || true)
if [[ -z "$CURL_LIB" ]]; then
  fail "libkvsCommonCurl not found under $BUILD_DIR; cannot tell which headers producer-c compiled against"
elif "$STRINGS" "$CURL_LIB" | grep -q 'AWS-LC'; then
  if [[ "$EXPECTED" == "aws-lc" ]]; then
    pass "$(basename "$CURL_LIB") was compiled against AWS-LC headers"
  else
    fail "$(basename "$CURL_LIB") embeds an AWS-LC version string in an OpenSSL build"
  fi
else
  if [[ "$EXPECTED" == "aws-lc" ]]; then
    fail "$(basename "$CURL_LIB") carries no AWS-LC version string: producer-c was not compiled against AWS-LC headers"
  else
    pass "$(basename "$CURL_LIB") carries no AWS-LC version string"
  fi
fi

# --- 5. Default build links OpenSSL dynamically (Linux) ------------------------
if [[ "$EXPECTED" == "openssl" ]]; then
  if [[ "$(uname)" != "Linux" ]]; then
    echo "SKIP: dynamic libssl/libcrypto link check runs on Linux only"
  else
    LINKED=0
    for lib in "$BUILD_DIR"/libgstkvssink.so "$BUILD_DIR"/libKinesisVideoProducer.so; do
      [[ -e "$lib" ]] || continue
      LINKED=$((LINKED + 1))
      DEPS=$(ldd "$lib" 2>&1 || true)
      if echo "$DEPS" | grep -q 'not found'; then
        fail "$(basename "$lib") has unresolved dynamic dependencies:"
        echo "$DEPS" | grep 'not found' | while IFS= read -r l; do echo "    $l"; done
      fi
      for want in libssl libcrypto; do
        if echo "$DEPS" | grep -qE "^[[:space:]]*${want}\.so[^ ]* => /"; then
          pass "$(basename "$lib") links ${want} ($(echo "$DEPS" | grep -E "^[[:space:]]*${want}\.so" | awk '{print $3}' | head -1))"
        else
          fail "$(basename "$lib") does not link ${want} dynamically: the default build lost its TLS stack"
        fi
      done
    done
    if [[ "$LINKED" -eq 0 ]]; then
      echo "SKIP: no libgstkvssink.so / libKinesisVideoProducer.so in $BUILD_DIR (static-only build links OpenSSL statically)"
    fi
  fi
fi

if [[ "$FAILED" -ne 0 ]]; then
  echo "Crypto backend check FAILED for expected backend '$EXPECTED'"
  exit 1
fi
echo "Crypto backend check passed for '$EXPECTED'"
