#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_DIR="${ROOT_DIR}/doc/modules/ROOT/partials/generated"

mkdir -p "${TARGET_DIR}"

cp "${ROOT_DIR}/doc/prd.adoc" "${TARGET_DIR}/prd.adoc"
cp "${ROOT_DIR}/doc/status.adoc" "${TARGET_DIR}/status.adoc"
cp "${ROOT_DIR}/doc/spec-kit.adoc" "${TARGET_DIR}/spec-kit.adoc"
cp "${ROOT_DIR}/doc/doc.adoc" "${TARGET_DIR}/api.adoc"
