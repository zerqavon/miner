# Zerqavon XMRig Miner

Official CPU miner for the Zerqavon `rx/zqv` proof-of-work algorithm. This is
a GPLv3 fork of XMRig 6.26.0 with support for Zerqavon's `ZQVXPOW v1`
domain-separated RandomX jobs.

## Downloads

Download verified Windows, Ubuntu and HiveOS packages from the
[latest Zerqavon miner release](https://github.com/zerqavon/miner/releases/latest).
SHA-256 checksums are published with every release.

## Mining

```text
xmrig-zqv -a rx/zqv -o POOL:PORT -u WALLET_OR_POOL_USERNAME -p x
```

The wallet, username and worker format is defined by the selected pool. XMRig
automatically selects an appropriate CPU profile; use `--threads=N` only when
you want to override it.

## Zerqavon implementation

- Algorithm identifier: `rx/zqv`
- RandomX configuration: standard/reference RandomX
- PoW domain prefix: `ZQVXPOW\x01`
- PoW nonce: final four bytes of the hashing blob
- Pool protocol: CryptoNote JSON-RPC Stratum
- Default donation schedule: 1%
- Production donation endpoint: `fee.zerqavon.org:7456`

Legacy first-generation pool jobs advertised as `rx/0` are recognized only
when the miner was explicitly started with `-a rx/zqv` and the job contains
the Zerqavon domain prefix.

See [ZERQAVON.md](ZERQAVON.md) for implementation details, compatibility notes
and verified mining results. See [BUILD-PRODUCTION.md](BUILD-PRODUCTION.md) for
platform-specific instructions.

## Building

Clone recursively and use XMRig's CMake build system:

```bash
git clone --recursive https://github.com/zerqavon/miner.git
cd miner
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## License and attribution

This project is derived from [XMRig](https://github.com/xmrig/xmrig) and is
distributed under the GNU General Public License version 3 or later. Original
copyright notices, source history and the complete [LICENSE](LICENSE) are
retained.
