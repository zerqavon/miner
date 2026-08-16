# Zerqavon changes

## 6.26.0-zqv3

- Enabled hwloc topology detection in the Windows production build, allowing
  XMRig to select SMT threads and affinity correctly on supported CPUs.
- Cached each worker job's nonce offset instead of recalculating the dynamic
  Zerqavon end-of-blob position in the hashing loop.
- On a Ryzen 5 9600X without MSR access, the automatic profile changed from 6
  to 12 threads and measured about 5.49 kH/s versus 4.19 kH/s in the baseline
  short benchmark (approximately 31% higher).
- Live Zerqavon pool validation completed with 222 accepted and 0 rejected
  shares.
- Corrected the HiveOS custom loader contract: `miner_ver()` now returns an
  empty value so Hive installs from `CUSTOM_INSTALL_URL` instead of APT, and
  the Hive archive uses the loader-compatible name `zerqavon-miner-zqv3`.

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
