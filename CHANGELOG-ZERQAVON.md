# Zerqavon changes

## 6.26.0-zqv2

- Redirected the standard one-minute-per-100-minutes donation session to
  `fee.zerqavon.org:7456`.
- User sessions continue mining `rx/zqv`; the donation endpoint can negotiate
  a standard `rx/0` RandomX job for the donation session.
- Restored the HiveOS custom-miner identifier `zerqavon-miner` for compatibility
  with existing Flight Sheets and older HiveOS loaders.
- Removed the built-in-miner version hint from the HiveOS manifest so HiveOS
  downloads the custom archive instead of looking for a nonexistent APT package.

## 6.26.0-zqv1

- Added the `rx/zqv` RandomX algorithm identifier.
- Added `ZQVXPOW v1` domain-prefix recognition.
- Added variable-length PoW blobs with the nonce in the final four bytes.
- Added compatibility with legacy Zerqavon pool jobs advertised as `rx/0`.
- Added correct canonical block-template nonce placement for direct daemon
  submission.
- Retained XMRig's standard RandomX configuration, optimizations and GPLv3
  licensing.
