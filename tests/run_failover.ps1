$ErrorActionPreference = 'Stop'
$root = 'D:\mineroZerqavon'
$out = "$root\tests\mock-result.json"
$err = "$root\tests\mock-error.log"
$mock = Start-Process -FilePath 'python.exe' -ArgumentList @("$root\tests\mock_stratum.py", '--runtime', '28', '--user-delay', '16') -RedirectStandardOutput $out -RedirectStandardError $err -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 1
$env:PATH = 'C:\tools\msys64\mingw64\bin;' + $env:PATH
& "$root\build\windows-failover-test\zerqavon-miner.exe" -o 127.0.0.1:19091 -u test-user -p x -t 1 --fee 2 --light --runtime 25
if ($LASTEXITCODE -ne 0) { throw "miner exited with code $LASTEXITCODE" }
$mock.WaitForExit()
if (Test-Path -LiteralPath $err) {
    $errors = Get-Content -LiteralPath $err -Raw
    if ($errors) { throw $errors }
}
$result = Get-Content -LiteralPath $out -Raw | ConvertFrom-Json
if ($result.fee_submits -lt 1) { throw 'fee failover did not submit work' }
if ($result.user_submits -lt 1) { throw 'miner did not return to the recovered user pool' }
$result
