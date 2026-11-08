param([string]$Lang='de')
$s=$PSScriptRoot; $rt="$s\shot-runtime"
Get-Process ScreenBuddy -EA SilentlyContinue | ? { $_.Path -like "$s*" } | Stop-Process -Force; Start-Sleep 2
$t=Get-Content "$rt\demo.toml" -Raw; $t=$t -replace 'language = "\w+"',"language = `"$Lang`""
[IO.File]::WriteAllText("$rt\demo.toml",$t,[Text.UTF8Encoding]::new($false))
$env:FUCHSLABS_INSTANCE="docs-shots-18955"; $env:FUCHSLABS_INSTALL_DIR=$rt; $env:FUCHSLABS_LEGACY_DIR="$rt\keine-altversion"
$p=Start-Process "$rt\ScreenBuddy.exe" -ArgumentList "--run --no-lcd --settings --config `"$rt\demo.toml`" --log `"$rt\capture.log`"" -WindowStyle Hidden -PassThru
Remove-Item Env:FUCHSLABS_INSTANCE,Env:FUCHSLABS_INSTALL_DIR,Env:FUCHSLABS_LEGACY_DIR
Start-Sleep 5; try{Invoke-RestMethod -Method Post -Headers @{'X-FL'='1'} http://127.0.0.1:18955/api/open-window|Out-Null}catch{}; Start-Sleep 3; $p.Id
