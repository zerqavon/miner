#!/usr/bin/env bash
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
config="${CUSTOM_CONFIG_FILENAME:-$script_dir/zerqavon.conf}"
log_base="${CUSTOM_LOG_BASENAME:-/var/log/miner/custom/xmrig-zqv}"

[[ ! -x "$script_dir/xmrig-zqv" ]] && echo "Zerqavon XMRig binary is missing" >&2 && exit 1
[[ ! -f "$config" ]] && echo "Zerqavon configuration is missing: $config" >&2 && exit 1

source "$config"
mkdir -p "$(dirname "$log_base")"

if command -v hugepages >/dev/null 2>&1; then
    hugepages -rx >/dev/null 2>&1 || true
fi

extra_args=()
if [[ -n "$EXTRA_ARGS" ]]; then
    read -r -a extra_args <<< "$EXTRA_ARGS"
fi

cd "$script_dir" || exit 1
./xmrig-zqv -a rx/zqv -o "$POOL_URL" -u "$POOL_USER" \
    -p "${POOL_PASSWORD:-x}" --print-time=20 "${extra_args[@]}" \
    2>&1 | tee --append "${log_base}.log"
