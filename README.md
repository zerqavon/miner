# LiquidMiner

LiquidMiner is an open-source CPU/GPU cryptocurrency miner focused on CivicLight v2, RandomX variants and KawPow. It supports ordinary single-pool mining, two active pool connections and automatic fee mining.

This project is derived from XMRig. See [NOTICE.md](NOTICE.md) and [LICENSE](LICENSE) for attribution and license terms.

## 0.1.0 performance update

The current Windows MSVC release includes a dedicated automatic CPU profile for
Vexta RandomX (`rx/vexta`). It detects the available logical processors on the
host instead of assuming a fixed 12-thread CPU. Users can still override the
automatic selection with `-t N`.

In a controlled comparison on an AMD Ryzen 5 9600X using six manually selected
threads and the same Vexta pool, LiquidMiner measured approximately **3.10 kH/s**
versus **2.98 kH/s** from Vexta CPU Miner 0.1.2: about **3.5% higher average
hashrate**. Results vary with CPU model, temperature, power limits, background
load and thread count; this is a measurement, not a guaranteed gain for every
system.

The update also includes:

- portable MSVC support for the Vexta 256-bit share-target calculation;
- runtime CPU capability selection for Intel and AMD x86-64 processors;
- RandomX JIT, ASM and huge-page support in the Windows release;
- a separate Vexta thread profile so generic RandomX tuning does not silently
  force Vexta to use half of the logical processors;
- the same automatic CPU-profile logic in the generic Ubuntu build.

## Supported algorithms

| Algorithm | Option | Backend |
| --- | --- | --- |
| RandomX / Monero | `rx/0` | CPU, OpenCL, CUDA |
| RandomX variants | `rx/v2`, `rx/wow`, `rx/arq`, `rx/graft`, `rx/sfx`, `rx/yada` | CPU, OpenCL, CUDA |
| Zerqavon RandomX | `rx/zqv` | CPU |
| CivicLight v2 | `civiclight` or `civiclight/v2` | CPU |
| KawPow | `kawpow` or `kawpow/rvn` | OpenCL, CUDA |

GPU support depends on how the binary was built and on the installed vendor runtime. CPU algorithms do not automatically become GPU algorithms.

## Features

- Automatic CPU thread selection.
- Manual thread control with `-t`.
- `--cpu-max-threads-hint=N` / `--max-cpu-usage=N` for percentage-based CPU selection.
- Two simultaneous pool connections when both jobs are valid.
- Majority/minority dual-pool mode for different CPU algorithms.
- Fixed percentage split with `--split-pool0` and `--split-pool1`.
- RandomX fee mining with a minimum configurable fee of 1%.
- Periodic hashrate and share reporting.
- Optional HTTP API, OpenCL and CUDA backends.

## Quick start

### Zerqavon

```bash
liquidminer -o pool.liquidpool.net:4008 -a rx/zqv -u YOUR_ZQV_WALLET -p x --print-time=10
```



### Two pools with automatic majority switching

Both pool connections remain active. When the algorithms are incompatible, the miner assigns the majority of CPU work to one pool and a small minority to the other. An accepted share from the majority side switches the majority to the other pool. This mode is not used with a fixed split.

```bash
liquidminer -o pool.liquidpool.net:4008 -a rx/zqv -u YOUR_ZQV_WALLET -p x -o pool2.example.com:4008 -a rx/zqv -u YOUR_ZQV_WALLET -p x --print-time=10
```

### Two pools with a fixed percentage split

The following keeps approximately 80% of CPU work on pool 0 and 20% on pool 1. Shares do not flip the percentages.

```bash
liquidminer -o pool.liquidpool.net:4008 -a rx/zqv -u YOUR_ZQV_WALLET -p x -o pool2.example.com:4008 -a rx/zqv -u YOUR_ZQV_WALLET -p x --split-pool0=80 --split-pool1=20 --print-time=10
```

The two values must be between 1 and 99 and must add up to 100. The fixed split is for CPU backends; GPU split mode is a separate backend configuration.

### GPU-only KawPow

```bash
liquidminer -o pool.example.com:6060 -a kawpow -u YOUR_RVN_WALLET -p x --no-cpu --opencl --print-time=10
```

For NVIDIA CUDA builds, use `--cuda` instead of `--opencl`.

## CPU controls

| Option | Description |
| --- | --- |
| `-t, --threads=N` | Explicit number of CPU mining threads. |
| `--cpu-max-threads-hint=N` | Select approximately N% of logical CPU threads automatically. |
| `--max-cpu-usage=N` | Compatibility alias for the percentage-based CPU hint. |
| `--cpu-affinity=N` | Bind mining to a CPU affinity mask. |
| `--cpu-priority=N` | Set process priority. |
| `--cpu-no-yield` | Prefer hashrate over desktop responsiveness. |
| `--no-huge-pages` | Disable huge pages. This can reduce RandomX performance. |
| `--asm=MODE` | Assembly mode: `auto`, `none`, `intel`, `ryzen` or `bulldozer`. |

If no thread option is supplied, LiquidMiner selects a profile automatically. In dual-pool mode the selected CPU work is divided between the active pool jobs according to the selected strategy.

## Pool, fee and status options

| Option | Description |
| --- | --- |
| `-o, --url=HOST:PORT` | Add a pool connection. Repeat for a second pool. |
| `-a, --algo=ALGO` | Select the algorithm for the preceding pool. |
| `-u, --user=USER` | Wallet, username or pool login. |
| `-p, --pass=PASS` | Pool password, commonly `x`. |
| `--rig-id=ID` | Worker name where supported. |
| `--tls` | Enable TLS for the pool connection. |
| `--split-pool0=N` | Fixed CPU percentage for pool 0. |
| `--split-pool1=N` | Fixed CPU percentage for pool 1. |
| `--donate-level=N` | Fee percentage; values below 1% are rejected. |
| `--print-time=N` | Print hashrate every N seconds. |
| `--log-file=FILE` | Write output to a log file. |
| `--no-color` | Disable colored console output. |
| `--verbose` | Enable detailed logging. |

The built-in fee window is handled automatically and uses the configured project fee endpoint. The fee connection is intentionally not printed as a normal user pool.

## RandomX options

| Option | Description |
| --- | --- |
| `--randomx-init=N` | Dataset initialization threads. |
| `--randomx-mode=MODE` | `auto`, `fast` or `light`. |
| `--randomx-no-numa` | Disable NUMA support. |
| `--randomx-1gb-pages` | Request 1 GB pages where supported. |
| `--randomx-wrmsr=N` | Configure or disable MSR tuning. Use `-1` to disable writes. |
| `--randomx-no-rdmsr` | Do not restore MSR values on exit. |

## Other commands

| Option | Description |
| --- | --- |
| `--no-cpu` | Disable CPU mining. |
| `--opencl` | Enable OpenCL. |
| `--cuda` | Enable CUDA. |
| `--opencl-devices=N` | Select OpenCL devices. |
| `--cuda-devices=N` | Select CUDA devices. |
| `--http-host=HOST` | HTTP API bind address. |
| `--http-port=N` | HTTP API port. |
| `-c, --config=FILE` | Load JSON configuration. |
| `--dry-run` | Validate configuration and exit. |
| `--bench=N` | Run a local benchmark. |
| `--version` | Print version. |
| `-h, --help` | Print all available options. |

Interactive keys are `h` for hashrate, `s` for results, `p` to pause, `r` to resume and `c` for connection information.

## Building on Ubuntu 20.04+

The recommended Linux release is compiled on Ubuntu 20.04.6 with generic x86_64 settings. A binary built against Ubuntu 20.04 is normally compatible with later Ubuntu releases such as 22.04 and 24.04. Test hardware-specific builds separately before distributing them.

Install dependencies:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git pkg-config libuv1-dev libssl-dev libhwloc-dev libmicrohttpd-dev
```

Configure and build the generic CPU release:

```bash
cmake -S . -B build-ubuntu20-generic-static -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_STATIC=ON \
  -DXMRIG_NATIVE_OPTIMIZATIONS=OFF \
  -DWITH_CUDA=OFF \
  -DWITH_OPENCL=OFF \
  -DWITH_HWLOC=ON
cmake --build build-ubuntu20-generic-static -j"$(nproc)"
./build-ubuntu20-generic-static/liquidminer --version
```

`XMRIG_NATIVE_OPTIMIZATIONS=OFF` keeps the generic build usable on older x86_64 CPUs. For a local machine build, set it to `ON` after testing. CUDA and OpenCL require their respective SDKs and are built separately.

## Building on Windows with Visual Studio

Visual Studio 2022 and the Windows 10/11 SDK are required. The project uses the static MSVC runtime (`/MT`) for Release builds. Third-party libraries are installed with vcpkg.

Install vcpkg and the dependencies:

```powershell
git clone https://github.com/microsoft/vcpkg.git thirdparty-build/vcpkg
thirdparty-build/vcpkg/bootstrap-vcpkg.bat -disableMetrics
thirdparty-build/vcpkg/vcpkg.exe install libuv:x64-windows openssl:x64-windows hwloc:x64-windows
thirdparty-build/vcpkg/vcpkg.exe install libuv:x64-windows-static openssl:x64-windows-static hwloc:x64-windows-static
```

Dynamic dependency build:

```powershell
cmake -S . -B build-msvc-dynamic -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=thirdparty-build/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DBUILD_STATIC=OFF -DXMRIG_NATIVE_OPTIMIZATIONS=OFF
cmake --build build-msvc-dynamic --config Release
```

This produces `liquidminer.exe` plus the OpenSSL and libuv DLLs in `build-msvc-dynamic/Release`.

Single-executable dependency build:

```powershell
cmake -S . -B build-msvc-static -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=thirdparty-build/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  -DBUILD_STATIC=ON -DXMRIG_NATIVE_OPTIMIZATIONS=OFF
cmake --build build-msvc-static --config Release
```

This produces a self-contained `liquidminer.exe`; `WinRing0x64.sys` remains separate because Windows loads it as a driver. Always distribute the `Release` executable together with the applicable license and notice files.

## Open-source development

Pull requests should include the affected platform, compiler, build command and a short test result. Do not commit wallet addresses, pool passwords, private keys or generated build directories.

Before publishing a release, generate checksums:

```bash
sha256sum liquidminer
```

## Antivirus and safety

Mining software is commonly classified as a potentially unwanted application because it intentionally connects to mining pools and uses substantial CPU/GPU resources. Run it only on hardware you own or are authorized to use. Releases should include visible command-line configuration, documented fee behavior, checksums and the source/license files.
