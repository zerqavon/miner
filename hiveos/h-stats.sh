#!/usr/bin/env bash

log_base="${CUSTOM_LOG_BASENAME:-/var/log/miner/custom/xmrig-zqv}"
log_file="${log_base}.log"
speed_line="$(grep -a 'speed 10s/60s/15m' "$log_file" 2>/dev/null | tail -n 1 || true)"

# XMRig prints the current 10-second rate first and the unit last.
hashrate="$(sed -nE 's/.*speed 10s\/60s\/15m[[:space:]]+([0-9.]+).*/\1/p' <<< "$speed_line")"
unit="$(sed -nE 's/.*speed 10s\/60s\/15m.*[[:space:]](H\/s|kH\/s|MH\/s)[[:space:]]*$/\1/p' <<< "$speed_line")"
[[ -z "$hashrate" ]] && hashrate=0
case "$unit" in
    kH/s) hashrate="$(awk -v value="$hashrate" 'BEGIN { printf "%.3f", value * 1000 }')" ;;
    MH/s) hashrate="$(awk -v value="$hashrate" 'BEGIN { printf "%.3f", value * 1000000 }')" ;;
esac

accepted="$(grep -ac 'accepted (' "$log_file" 2>/dev/null || true)"
rejected="$(grep -ac 'rejected (' "$log_file" 2>/dev/null || true)"
pid="$(pgrep -o -f '/xmrig-zqv([[:space:]]|$)' 2>/dev/null || true)"
uptime=0
[[ -n "$pid" ]] && uptime="$(ps -o etimes= -p "$pid" 2>/dev/null | tr -d ' ')"
[[ -z "$uptime" ]] && uptime=0

khs="$(awk -v hs="$hashrate" 'BEGIN { printf "%.6f", hs / 1000 }')"
stats="$(printf '{"hs":[%s],"hs_units":"hs","uptime":%s,"ar":[%s,%s],"algo":"rx/zqv","ver":"6.26.0-zqv2"}' \
    "$hashrate" "$uptime" "$accepted" "$rejected")"
