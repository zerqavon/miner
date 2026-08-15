# Zerqavon integration test results

Date: 2026-08-16

Platform: Windows x64, AMD Ryzen 5 9600X, 12 mining threads.

## Pool compatibility

Command shape:

```text
xmrig-zqv.exe -a rx/zqv -o POOL:PORT -u POOL_USER -p x --threads=12
```

Observed results against a live Zerqavon pool:

- Pool advertised its legacy algorithm value `rx/0`.
- Miner validated the `ZQVXPOW v1` prefix and selected `rx/zqv`.
- Job height and variable difficulty updates were processed correctly.
- 261 accepted shares.
- 11 stale shares returned as `block expired` during rapid startup job and
  difficulty changes.
- No `invalid result`, invalid nonce or incompatible-PoW errors.
- 60-second average: 4.8486 kH/s.
- Maximum observed: 5.4461 kH/s.

## Build verification

- `rx/zqv` configuration passed XMRig's `--dry-run` validation.
- Windows executable has no MinGW, libstdc++, libuv or OpenSSL runtime DLL
  dependency; only standard Windows system DLLs are imported.
