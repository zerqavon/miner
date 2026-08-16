# XMRig for Zerqavon

This repository is a GPLv3 fork of XMRig 6.26.0 with CPU mining support for
Zerqavon's `rx/zqv` algorithm (`ZQVXPOW v1` over the standard RandomX
configuration).

## Mining

```text
xmrig.exe -a rx/zqv -o POOL:PORT -u WALLET_OR_POOL_USERNAME -p x
```

The wallet address or username format is defined by the selected pool. Use
`--threads=N` only when you want to override XMRig's automatic CPU profile.
Running as administrator can allow Huge Pages and MSR optimizations when the
operating system permits them.

The miner accepts native `rx/zqv` jobs. For compatibility with the first
Zerqavon pools, it also recognizes legacy jobs advertised as `rx/0` when the
blob begins with the eight-byte `ZQVXPOW v1` domain separator and the miner was
started explicitly with `-a rx/zqv`.

## Implementation

- Algorithm name: `rx/zqv`
- RandomX configuration: standard/reference RandomX
- PoW domain prefix: `ZQVXPOW\x01`
- PoW nonce: final four bytes of the hashing blob
- Pool protocol: CryptoNote JSON-RPC Stratum
- Direct daemon submission retains the canonical nonce position in the block
  template while hashing the Zerqavon domain-separated PoW blob.

## Verified test

The Windows x64 build was tested against a live Zerqavon pool on 2026-08-16:

- Pool jobs were identified and displayed as `rx/zqv`.
- 261 shares were accepted during the measured run.
- 11 initial stale shares were reported as `block expired` while the pool
  rapidly changed its startup difficulty; no invalid-PoW shares were observed.
- Measured 60-second average on a Ryzen 5 9600X: approximately 4.85 kH/s.
- Maximum observed rate: approximately 5.45 kH/s.

## License and origin

This is a modified version of [XMRig](https://github.com/xmrig/xmrig). It is
distributed under the GNU General Public License version 3 or later. The
original copyright notices, source history and `LICENSE` file are retained.
The standard XMRig donation schedule remains enabled and is shown by the miner
at startup. In the `zqv2` and later production builds, that session connects to
`fee.zerqavon.org:7456` and can negotiate standard `rx/0` RandomX work.

---

## Uso en español

Este repositorio es un fork GPLv3 de XMRig 6.26.0 compatible con el algoritmo
`rx/zqv` de Zerqavon.

```text
xmrig.exe -a rx/zqv -o POOL:PUERTO -u WALLET_O_USUARIO_DE_POOL -p x
```

El formato de wallet, usuario y worker depende de cada pool. El minero conserva
la licencia, los avisos de copyright y la donación estándar de XMRig.
