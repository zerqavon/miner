$ErrorActionPreference = 'Stop'
$root = 'D:\mineroZerqavon'
$daemon = 'D:\forkrandomx\zerqavon-cli-source\build\win64-release\bin\zerqavond.exe'
$miner = "$root\build\windows\zerqavon-miner.exe"
$data = "$root\testnet-run\daemon"
$address = 'Z7kC29VXR5uZDdj3XLSUQYfbhA5Pzj25AGxJWnDB7oXCLE6q8a9RYfd5NjXTKFRAycbEJYoyDbKkp4N4Huvzg4sC1wzfXsndv'
if (Test-Path -LiteralPath $data) {
    $resolved = (Resolve-Path -LiteralPath $data).Path.TrimEnd('\')
    if ($resolved -ne 'D:\mineroZerqavon\testnet-run\daemon') { throw "unsafe test cleanup target: $resolved" }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
New-Item -ItemType Directory -Path $data -Force | Out-Null
$daemonArgs = @(
    '--testnet', '--offline', '--no-igd', '--data-dir', $data,
    '--rpc-bind-ip', '127.0.0.1', '--rpc-bind-port', '37771',
    '--p2p-bind-ip', '127.0.0.1', '--p2p-bind-port', '37770',
    '--log-file', "$data\daemon.log", '--non-interactive'
)
Start-Process -FilePath $daemon -ArgumentList $daemonArgs -WindowStyle Hidden | Out-Null
try {
    Start-Sleep -Seconds 4
    $env:PATH = 'C:\tools\msys64\mingw64\bin;' + $env:PATH
    & $miner --daemon -o 127.0.0.1:37771 -u $address -t 1 --fee 2 --light --runtime 20
    if ($LASTEXITCODE -ne 0) { throw "miner exited with code $LASTEXITCODE" }
    $info = Invoke-RestMethod -Uri 'http://127.0.0.1:37771/get_info' -Method Get
    [PSCustomObject]@{
        Network = $info.nettype
        Height = $info.height
        Status = $info.status
        Synchronized = $info.synchronized
    }
}
finally {
    try {
        Invoke-RestMethod -Uri 'http://127.0.0.1:37771/stop_daemon' -Method Post -ContentType 'application/json' -Body '{}' | Out-Null
    }
    catch {}
}
