# Zerqavon XMRig for HiveOS

Use this archive as a HiveOS custom miner named `zerqavon-miner`. Keeping this
name is required for compatibility with existing Zerqavon Flight Sheets. Set:

- Miner name: `zerqavon-miner`
- Installation URL: the release asset named `zerqavon-miner-zqv3.tar.gz`

- Pool URL: `POOL:PORT`
- Wallet and worker: your pool username or Zerqavon wallet, as required by the pool
- Password: `x`, unless the pool specifies another value
- Extra config arguments: optional XMRig command-line arguments

The wrapper always selects `rx/zqv`. The miner uses the standard 1% donation
schedule and the production donation endpoint configured in the source.
