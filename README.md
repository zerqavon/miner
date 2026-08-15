# Zerqavon Miner

Minero CPU independiente para Zerqavon. Implementa el formato de prueba de trabajo `ZQVXPOW v1` sobre RandomX, conexión Stratum y minería directa mediante el RPC del daemon.

No contiene ni reutiliza código de XMRig. RandomX se incluye como dependencia de terceros bajo su licencia BSD-3-Clause, conservada en `external/randomx/LICENSE`.

## Pool

```text
zerqavon-miner.exe -o POOL:PORT -u WALLET_O_USUARIO -p x -t 4
```

La pool debe entregar trabajos `ZQVXPOW v1`. Un trabajo CryptoNote `rx/0` normal no es válido para Zerqavon.

## Minería directa al daemon

Mainnet:

```text
zerqavon-miner.exe --daemon -o 127.0.0.1:27771 -u DIRECCION_PRINCIPAL -t 4
```

Testnet:

```text
zerqavon-miner.exe --daemon -o 127.0.0.1:37771 -u DIRECCION_TESTNET -t 1 --light
```

No se puede minar directamente a una subdirección. Debe utilizarse una dirección principal.

## Fee y recuperación de conexión

- Fee predeterminado: `1%`.
- Rango permitido: `--fee 1` hasta `--fee 100`.
- Destino compilado: `fee.zerqavon.org:7456`.
- Usuario del fee: `fee`.
- Algoritmo del fee: RandomX estándar `rx/0`.
- La conexión del fee se mantiene separada de la conexión del usuario.
- Si el destino de fee no está disponible durante una ventana programada, continúa el trabajo del usuario.
- Cuando la pool del usuario falla, se efectúan diez intentos con inicio separado por un segundo.
- Después del décimo fallo se usa la pool de fee como respaldo mientras otro hilo continúa intentando recuperar la pool principal.
- Al recuperarse la pool del usuario se vuelve inmediatamente a ella, salvo durante una ventana de fee programada.

El modo RandomX completo es el predeterminado. `--light` reduce mucho el uso de memoria, pero también el rendimiento y está pensado para pruebas.

## Compilar en Windows con MSYS2/MinGW

```text
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/windows --parallel
```

## Pruebas incluidas

- `tests/run_testnet.ps1`: mina bloques reales contra un daemon Zerqavon testnet.
- `tests/run_failover.ps1`: simula la caída de la pool, el cambio a fee y el regreso automático.

## HiveOS

El paquete de publicación para HiveOS debe llamarse `zerqavon-miner-1.0.3.tar.gz` y contener una carpeta raíz `zerqavon-miner`.

Las credenciales y wallets dentro de `testnet-run` son exclusivamente desechables.
