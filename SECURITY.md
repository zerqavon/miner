# Security and release verification

LiquidMiner is mining software. Antivirus products may classify miners as PUA/PUP or “coin miner” tools because they intentionally use CPU/GPU resources and connect to mining pools.

Before running a release:

1. Download only from the official LiquidMiner release page.
2. Compare the SHA256 checksum published with the release.
3. Run `liquidminer.exe --version` to verify the product name and version.
4. Review the command line you are launching; LiquidMiner mines only to the pools and wallets you provide, plus the documented fee endpoint.

LiquidMiner does not require .NET.

If Microsoft Defender blocks the file on your own development machine, submit the release to Microsoft Security Intelligence as a false positive or allow the specific release folder after verifying the checksum. Do not disable antivirus globally.

Only run miners on hardware you own or are authorized to use.
