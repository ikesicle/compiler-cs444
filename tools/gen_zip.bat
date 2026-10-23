@echo off
setlocal

if exist joosc del /q joosc
git log > git.log

powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference = 'Stop'; $root = (Resolve-Path '.').Path; $zipPath = Join-Path $root 'joosc.zip'; $files = Get-ChildItem -Path $root -Recurse -File | Where-Object { $rel = $_.FullName.Substring($root.Length + 1).Replace('/','\'); if ($rel -like '*.zip') { return $false }; if ($rel -match '^(build|docs|\.git|\.cache)\\') { return $false }; if ($rel -match '^tests\\[^\\]+_marmoset\\') { return $false }; return $true } | ForEach-Object { $_.FullName }; Compress-Archive -Path $files -DestinationPath $zipPath -Force"

del /q git.log

endlocal