#!/usr/bin/env bash
# Publish every public header on the v<project version> GitHub release.
# base-<version>.tar.gz keeps include/, core/, blocks/, and math/ paths.
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

mapfile -d '' headers < <(find include core blocks math -type f -name '*.hpp' -print0 | sort -z)
if [[ ${#headers[@]} -eq 0 ]]; then
  echo "No public headers found under include/, core/, blocks/, or math/" >&2
  exit 1
fi

for header in "${headers[@]}"; do
  rel="${header#./}"
  dest="${stage}/${rel}"
  mkdir -p "$(dirname "${dest}")"
  cp "${header}" "${dest}"
done

mapfile -t names < <(cd "${stage}" && find . -type f -name '*.hpp' -printf '%P\n' | sort)

archive="base-${version}.tar.gz"
# Members keep the include tree (include/, core/, blocks/, math/) so quoted
# paths such as "core/hal.hpp" still resolve after unpacking.
(
  cd "${stage}"
  tar -czf "${root}/${archive}" -- "${names[@]}"
)

mapfile -t members < <(tar -tzf "${archive}")
if [[ ${#members[@]} -eq 0 ]]; then
  echo "Release archive is empty" >&2
  rm -f "${archive}"
  exit 1
fi
for member in "${members[@]}"; do
  if [[ ! "${member}" =~ ^([A-Za-z0-9_]+/)*[A-Za-z0-9_]+\.hpp$ ]]; then
    echo "Release archive member is not a header path: ${member}" >&2
    rm -f "${archive}"
    exit 1
  fi
done

notes_file="$(mktemp)"
{
  echo "BASE ${version}"
  echo
  echo "Header files from commit ${GITHUB_SHA:-unknown}."
  echo
  echo '```'
  printf '%s\n' "${names[@]}"
  echo '```'
} > "${notes_file}"

assets=("${archive}")
for name in "${names[@]}"; do
  assets+=("${stage}/${name}")
done

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
