#!/usr/bin/env bash
# Publish every public header on the v<project version> GitHub release.
# The archive root is an include prefix. Sources live under src/; the archive
# contains base.hpp, base/, core/, and core/math/ so other block libraries can
# #include <core/....hpp> and #include <core/math/....hpp>.
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

mapfile -d '' headers < <(find src -type f -name '*.hpp' -print0 | sort -z)
if [[ ${#headers[@]} -eq 0 ]]; then
  echo "No public headers found under src/" >&2
  exit 1
fi

for header in "${headers[@]}"; do
  rel="${header#./}"
  rel="${rel#src/}"
  dest="${stage}/${rel}"
  mkdir -p "$(dirname "${dest}")"
  cp "${header}" "${dest}"
done

mapfile -t names < <(cd "${stage}" && find . -type f -name '*.hpp' -printf '%P\n' | sort)

archive="base-${version}.tar.gz"
# core/ is a directory in the archive so #include <core/....hpp> resolves
# when the unpacked tree is on the include path.
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
has_base=0
has_base_dir=0
has_core=0
has_core_math=0
for member in "${members[@]}"; do
  if [[ ! "${member}" =~ ^([A-Za-z0-9_]+/)*[A-Za-z0-9_]+\.hpp$ ]]; then
    echo "Release archive member is not a header path: ${member}" >&2
    rm -f "${archive}"
    exit 1
  fi
  if [[ "${member}" == "base.hpp" ]]; then
    has_base=1
  fi
  if [[ "${member}" == base/*.hpp ]]; then
    has_base_dir=1
  fi
  if [[ "${member}" == core/*.hpp ]]; then
    has_core=1
  fi
  if [[ "${member}" == core/math/*.hpp ]]; then
    has_core_math=1
  fi
done
if [[ "${has_base}" -ne 1 || "${has_base_dir}" -ne 1 || "${has_core}" -ne 1 || "${has_core_math}" -ne 1 ]]; then
  echo "Release archive must contain base.hpp, base/, core/, and core/math/" >&2
  rm -f "${archive}"
  exit 1
fi

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
