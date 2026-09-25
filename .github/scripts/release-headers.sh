#!/usr/bin/env bash
# Publish the contents of src/ as the v<project version> GitHub release archive.
# The archive root is that tree, which is the include prefix.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${root}"

version="$(
  sed -nE 's/^[[:space:]]*VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt | head -n1
)"
if [[ -z "${version}" ]]; then
  echo "Could not read project version from CMakeLists.txt" >&2
  exit 1
fi

tag="v${version}"
archive="base-${version}.tar.gz"
tar -czf "${archive}" -C src .

if [[ "${RELEASE_HEADERS_DRY_RUN:-}" == "1" ]]; then
  tar -tzf "${archive}"
  rm -f "${archive}"
  exit 0
fi

if [[ -z "${GITHUB_SHA:-}" ]]; then
  echo "GITHUB_SHA is required" >&2
  exit 1
fi

if gh release view "${tag}" >/dev/null 2>&1; then
  gh release delete "${tag}" --yes --cleanup-tag
fi

git tag -f "${tag}" "${GITHUB_SHA}"
git push origin "refs/tags/${tag}" --force

gh release create "${tag}" "${archive}" \
  --target "${GITHUB_SHA}" \
  --title "${tag}" \
  --notes "BASE ${version}"
