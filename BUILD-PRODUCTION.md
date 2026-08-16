# Zerqavon production binaries

Version: `6.26.0-zqv2`

## Windows x64

Run from a terminal:

```text
xmrig-zqv.exe -a rx/zqv -o POOL:PORT -u WALLET_OR_USERNAME -p x
```

Run the terminal as administrator if you want XMRig to apply the Windows MSR
optimization. Keep `WinRing0x64.sys` next to the executable.

## Ubuntu x64

The Linux executable is statically linked for portability across Ubuntu 20.04,
22.04, 24.04 and 26.04. Make it executable after extracting if required:

```bash
chmod +x xmrig-zqv
./xmrig-zqv -a rx/zqv -o POOL:PORT -u WALLET_OR_USERNAME -p x
```

## HiveOS

Upload the HiveOS archive as a custom miner and configure `POOL:PORT`, the
wallet or pool username, and password in the Flight Sheet. The wrapper selects
`rx/zqv` automatically.

All builds retain the upstream GPLv3 license and use the standard 1% donation
schedule configured for the Zerqavon production endpoint.
