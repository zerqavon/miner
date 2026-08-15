# Test results

Date: 2026-08-15

## Accepted-share display and 20-second hashrate

- A strict local Stratum test confirmed 510 accepted-share responses: 32 through fee failover and 478 after the user pool recovered.
- Every confirmed response produced an `ACCEPTED SHARE` line with the source, block height and cumulative total.
- The status line appeared once during a 22-second run and reported `H/s`, accepted, submitted and rejected counters.
- The measured status contained 469 accepted, 469 submitted and 0 rejected shares.

## Stratum compatibility regression

- Login and submit request IDs are encoded as JSON numbers, as required by XMRig Proxy.
- A strict local Stratum server accepted one fee login, 274 fee shares, one user login and 482 user shares.
- Failover activated after ten unavailable user-pool attempts and returned to the user pool automatically.
- A separate local build connected directly to XMRig Proxy on `127.0.0.1:7456`, received a live `rx/0` job and mined continuously without `End of file` disconnects.

## Zerqavon testnet

Command under test:

```text
zerqavon-miner.exe --daemon -o 127.0.0.1:37771 -u TESTNET_ADDRESS -t 1 --fee 2 --light --runtime 20
```

Result:

- Network reported by daemon: `testnet`.
- Started from a fresh testnet datadir at height 1.
- Three blocks submitted and accepted by the Zerqavon daemon.
- Final blockchain height: 4.
- Rejected submissions: 0.
- Observed light-mode rate: approximately 40-54 H/s.
- The unavailable production fee hostname did not stop user mining.

## Pool failover simulation

The test started a fee Stratum endpoint immediately and delayed the user Stratum endpoint. Result:

- `fee failover active` appeared after exactly 10 failed user-pool attempts.
- Fee endpoint received 252 valid submissions during failover.
- User endpoint then became available.
- Miner returned automatically to the user endpoint.
- User endpoint received 505 valid submissions after recovery.
- The log confirmed `developer-fee cycle restarted` at the moment of recovery.
- Rejected submissions: 0 during the measured status interval.

The local-only test build used `127.0.0.1:19090` for fee injection. It is not a release artifact. The release binary is compiled for `fee.zerqavon.org:7456` with user `fee`.
