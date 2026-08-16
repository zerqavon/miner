#!/usr/bin/env bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[[ -f "$script_dir/h-manifest.conf" ]] && source "$script_dir/h-manifest.conf"

miner_ver() {
    echo "6.26.0-zqv3"
}

miner_config_echo() {
    local config="${CUSTOM_CONFIG_FILENAME:-/hive/miners/custom/zerqavon-miner/zerqavon.conf}"
    if [[ -f "$config" ]]; then
        sed -E 's/^(POOL_PASSWORD=).*/\1********/' "$config"
    else
        echo "Zerqavon configuration has not been generated yet"
    fi
}

miner_config_gen() {
    local config="${CUSTOM_CONFIG_FILENAME:-/hive/miners/custom/zerqavon-miner/zerqavon.conf}"
    local pool="${CUSTOM_URL%%$'\n'*}"
    local wallet="${CUSTOM_TEMPLATE:-${CUSTOM_WALLET:-}}"
    local password="${CUSTOM_PASS:-x}"
    local extra="${CUSTOM_USER_CONFIG//$'\n'/ }"

    pool="${pool%% *}"
    pool="${pool#stratum+tcp://}"
    pool="${pool#stratum+ssl://}"
    pool="${pool#stratum://}"
    pool="${pool#tcp://}"

    [[ -z "$pool" ]] && echo "Zerqavon: Pool URL is required" >&2 && return 1
    [[ -z "$wallet" ]] && echo "Zerqavon: Wallet or pool username is required" >&2 && return 1

    mkdir -p "$(dirname "$config")"
    {
        printf 'POOL_URL=%q\n' "$pool"
        printf 'POOL_USER=%q\n' "$wallet"
        printf 'POOL_PASSWORD=%q\n' "$password"
        printf 'EXTRA_ARGS=%q\n' "$extra"
    } > "$config"
}

# Older HiveOS custom launchers only source this file.
if [[ -n "${CUSTOM_URL:-}" && -n "${CUSTOM_TEMPLATE:-${CUSTOM_WALLET:-}}" ]]; then
    miner_config_gen
fi
