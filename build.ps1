$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot
$buildCommand = 'set -e; source /app/drumlogue/environment; cd /workspace/lpc-robot; make; make install'
docker run --rm --user 0 -v "${repoRoot}:/workspace" --entrypoint /bin/bash xiashj/logue-sdk@sha256:e4d85a16c38dc4d34b0e93cabb6378df21729688dbcd88808c84e8d41aa0c9ba -lc $buildCommand
if ($LASTEXITCODE -ne 0) { throw 'drumlogue build failed.' }
[void](New-Item -ItemType Directory -Path (Join-Path $repoRoot 'dist') -Force)
Copy-Item -LiteralPath (Join-Path $repoRoot 'lpc-robot/lpc_robot7.drmlgunit') -Destination (Join-Path $repoRoot 'dist/lpc_robot7.drmlgunit') -Force
Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $repoRoot 'dist/lpc_robot7.drmlgunit')
