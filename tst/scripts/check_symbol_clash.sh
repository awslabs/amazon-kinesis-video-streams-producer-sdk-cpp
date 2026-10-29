#!/bin/bash
#
# Guards against crypto symbol interposition when kvssink is loaded into a
# GStreamer process that also uses the system TLS stack.
#
#   tst/scripts/check_symbol_clash.sh <build-dir> <static-libcrypto>
#
# <static-libcrypto> is required: the static libcrypto.a that was linked into the
# plugin (for builds that compile their dependencies,
# <repo>/open-source/local/lib/libcrypto.a). It is the positive control for check 1;
# the script does not guess it. A missing argument is a usage error and a path
# that does not exist is a FAIL.
#
# Scope: static AWS-LC builds (USE_AWS_LC=ON, BUILD_STATIC=ON), which link
# libcrypto/libssl and libcurl INTO the plugin. If those symbols stay exported
# with default ELF visibility, the dynamic linker may bind the plugin's own
# crypto calls to a system libcrypto loaded earlier (glib-networking, libsoup,
# another plugin) - or bind the system library's calls to the plugin's copy.
# Either way two incompatible libcrypto implementations service one call graph,
# which corrupts state at runtime. CMakeLists.txt hides them with
# -Wl,--exclude-libs,ALL for AWS-LC builds on ELF platforms only.
#
# Not covered: shared AWS-LC builds (BUILD_STATIC=OFF) do not embed the crypto
# stack; they load a shared AWS-LC libcrypto, and which libcrypto the loader
# picks there is outside what a symbol table check can prove. A static OpenSSL
# build is also out of scope (no --exclude-libs; check 1 would fail on it).
#
# Checks:
#   1. libgstkvssink exports no crypto or curl implementation symbols
#      (SSL_/EVP_/curl_/... see CRYPTO_RE).
#      A GStreamer plugin only needs its gst_plugin_* entry points. HARD FAIL.
#   2. No artifact mixes linkage: anything that embeds the static TLS stack
#      (has kvsCurlSslCtxCallback) must not ALSO dynamically need libssl/libcrypto.
#   3. Positive control: the same pattern matches hundreds of symbols in the
#      given static libcrypto, proving check 1 cannot pass vacuously.
#
# Works on Linux and macOS. Exit 0 only if every check passes.

set -u

BUILD_DIR="${1:-}"
CONTROL_LIBCRYPTO="${2:-}"
NM="${NM:-nm}"

if [[ -z "$BUILD_DIR" || -z "$CONTROL_LIBCRYPTO" ]]; then
  echo "usage: $0 <build-dir> <static-libcrypto>" >&2
  echo "  <static-libcrypto>: the libcrypto.a linked into the plugin (e.g. ./open-source/local/lib/libcrypto.a)" >&2
  exit 2
fi

FAILED=0
pass() { echo "PASS: $*"; }
fail() { echo "FAIL: $*"; FAILED=1; }

# Crypto (OpenSSL and AWS-LC share this API surface, plus AWS-LC's CBS_/CBB_
# byte-string helpers) and libcurl implementation prefixes. A denylist rather
# than an allowlist of gst_plugin_* exports: the plugin legitimately exports C++
# and GStreamer symbols that change with the code, while these prefixes never
# belong in its dynamic symbol table. The leading underscore difference between
# macOS and Linux is normalized below.
CRYPTO_RE='^(SSL_|TLS_|EVP_|CRYPTO_|BIO_|X509_|EC_|ECDSA_|ECDH_|RSA_|DH_|DSA_|HMAC_|ERR_|BN_|OPENSSL_|SHA1_|SHA256_|SHA512_|MD5_|AES_|PEM_|ASN1_|RAND_|OBJ_|PKCS|d2i_|i2d_|CBS_|CBB_|curl_|Curl_)'

exported_syms() {
  # Defined, external symbols of a shared object, one name per line, Mach-O
  # leading underscore stripped.
  local lib="$1"
  case "$(uname)" in
    Darwin) "$NM" -gU "$lib" 2>/dev/null | awk '{print $NF}' | sed 's/^_//' ;;
    *)      "$NM" -D --defined-only "$lib" 2>/dev/null | awk '{print $NF}' ;;
  esac
}

dynamic_needs_crypto() {
  local lib="$1"
  case "$(uname)" in
    Darwin) otool -L "$lib" 2>/dev/null | tail -n +2 | grep -E 'lib(ssl|crypto)\.' ;;
    *)      ldd "$lib" 2>/dev/null | grep -E 'lib(ssl|crypto)\.so' ;;
  esac
}

# --- 1. Plugin exports -----------------------------------------------------------
PLUGIN=""
for candidate in "$BUILD_DIR"/libgstkvssink.so "$BUILD_DIR"/libgstkvssink.dylib; do
  if [[ -e "$candidate" ]]; then PLUGIN="$candidate"; break; fi
done
if [[ -z "$PLUGIN" ]]; then
  fail "libgstkvssink not found in $BUILD_DIR (build with -DBUILD_GSTREAMER_PLUGIN=ON)"
else
  CLASHING=$(exported_syms "$PLUGIN" | grep -cE "$CRYPTO_RE")
  if [[ "$CLASHING" -eq 0 ]]; then
    pass "$(basename "$PLUGIN") exports no crypto implementation symbols"
  else
    fail "$(basename "$PLUGIN") exports $CLASHING crypto symbols; a host process using the system TLS stack can interpose them"
    exported_syms "$PLUGIN" | grep -E "$CRYPTO_RE" | head -10 | sed 's/^/    /'
  fi
  if ! exported_syms "$PLUGIN" | grep -q '^gst_plugin_'; then
    fail "$(basename "$PLUGIN") does not export its gst_plugin_ entry point (over-hidden)"
  else
    pass "$(basename "$PLUGIN") still exports its gst_plugin_ entry point"
  fi
fi

# --- 2. No mixed linkage ----------------------------------------------------------
for lib in "$BUILD_DIR"/libgstkvssink.so "$BUILD_DIR"/libgstkvssink.dylib \
           "$BUILD_DIR"/libKinesisVideoProducer.so "$BUILD_DIR"/libKinesisVideoProducer.dylib; do
  [[ -e "$lib" ]] || continue
  if ! "$NM" "$lib" 2>/dev/null | grep -q 'kvsCurlSslCtxCallback'; then
    echo "SKIP: $(basename "$lib") does not embed the static TLS marker symbol; mixed-linkage check not applicable"
  else
    DYN=$(dynamic_needs_crypto "$lib" || true)
    if [[ -n "$DYN" ]]; then
      fail "$(basename "$lib") embeds the static AWS-LC TLS path AND dynamically needs:"
      while IFS= read -r line; do echo "    $line"; done <<< "$DYN"
    else
      pass "$(basename "$lib") embeds its TLS stack and needs no dynamic libssl/libcrypto"
    fi
  fi
done

# --- 3. Positive control -----------------------------------------------------------
if [[ ! -f "$CONTROL_LIBCRYPTO" ]]; then
  # The control must not degrade to a silent SKIP.
  fail "control libcrypto not found: $CONTROL_LIBCRYPTO"
else
  CONTROL_HITS=$("$NM" -g "$CONTROL_LIBCRYPTO" 2>/dev/null | awk '{print $NF}' | sed 's/^_//' | grep -cE "$CRYPTO_RE")
  if [[ "$CONTROL_HITS" -ge 100 ]]; then
    pass "control: pattern matches $CONTROL_HITS symbols in $(basename "$CONTROL_LIBCRYPTO") (detector is not vacuous)"
  else
    fail "control: pattern matched only $CONTROL_HITS symbols in the static libcrypto; the detector would miss real exports"
  fi
fi

if [[ "$FAILED" -ne 0 ]]; then
  echo "Symbol clash check FAILED"
  exit 1
fi
echo "Symbol clash check passed"
