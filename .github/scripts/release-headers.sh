#!/usr/bin/env bash
# Publish every public header on the v<project version> GitHub release.
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
stage="$(mktemp -d)"
prefix="${stage}/base-${version}"
mkdir -p "${prefix}/include"

mapfile -d '' headers < <(find include src -type f -name '*.hpp' -print0 | sort -z)
if [[ ${#headers[@]} -eq 0 ]]; then
  echo "No public headers found under include/ or src/" >&2
  exit 1
fi

for header in "${headers[@]}"; do
  name="$(basename "${header}")"
  if [[ -e "${prefix}/include/${name}" ]]; then
    echo "Duplicate header file name: ${name}" >&2
    exit 1
  fi
  cp "${header}" "${prefix}/include/${name}"
done

archive="base-${version}.tar.gz"
tar -C "${stage}" -czf "${archive}" "base-${version}"

notes_file="$(mktemp)"
{
  echo "BASE ${version}"
  echo
  echo "Header files from commit ${GITHUB_SHA:-unknown}."
  echo
  echo '```'
  find "${prefix}/include" -type f -name '*.hpp' -printf '%f\n' | sort
  echo '```'
} > "${notes_file}"

assets=("${archive}")
while IFS= read -r name; do
  assets+=("${prefix}/include/${name}")
done < <(find "${prefix}/include" -type f -name '*.hpp' -printf '%f\n' | sort)

if [[ "${RELEASE_HEADERS_DRY_RUN:-}" == "1" ]]; then
  echo "tag=${tag}"
  echo "archive=${archive}"
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

gh release create "${tag}" "${assets[@]}" \
  --target "${GITHUB_SHA}" \
  --title "${tag}" \
  --notes-file "${notes_file}"
