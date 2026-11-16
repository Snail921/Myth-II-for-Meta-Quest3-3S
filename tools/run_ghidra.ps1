param([switch]$Existing, [string]$AdditionalEntries = '', [switch]$MapEvidence, [switch]$BridgeEvidence)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$ghidraRoot = Join-Path $PSScriptRoot 'runtime/ghidra_11.3.2_PUBLIC'
$jdkRoot = Join-Path $PSScriptRoot 'runtime/jdk-21.0.13+9'
$stateRoot = Join-Path $PSScriptRoot 'ghidra-state'
$projectDir = Join-Path $projectRoot 'research/ghidra'
$reportDir = Join-Path $projectRoot 'research/reports/ghidra-1.8.1'
New-Item -ItemType Directory -Force $stateRoot, $projectDir, $reportDir | Out-Null
# Invoke the same bootstrap as support/launch.bat with explicit JVM arguments.
# This avoids LaunchSupport writing JDK preferences to the Windows user profile.
    $flow = Get-Content (Join-Path $projectRoot 'research/reports/disassembly-1.8.1/reviewed-flow.json') -Raw | ConvertFrom-Json
    $entries = ($flow.routines | ForEach-Object { '{0:x8}' -f $_.entry }) -join ','
    if ($AdditionalEntries) { $entries += ',' + $AdditionalEntries }
    $arguments = @($projectDir, 'Myth181', '-max-cpu', '2', '-analysisTimeoutPerFile', '600', '-scriptPath', (Join-Path $PSScriptRoot 'ghidra-scripts'),
        '-preScript', 'EnsureReviewedFunctions.java', '-postScript', 'ExportReviewed.java', $reportDir, $entries, '-log', (Join-Path $reportDir 'analysis.log'),
        '-scriptlog', (Join-Path $reportDir 'script.log'))
    if ($MapEvidence) { $arguments += @('-postScript', 'ExportMapEvidence.java', (Join-Path $projectRoot 'research/reports/map-evidence')) }
    if ($BridgeEvidence) { $arguments += @('-postScript', 'ExportBridgeEvidence.java', (Join-Path $projectRoot 'research/reports/bridge-evidence')) }
    if ($Existing) {
        $arguments += @('-process', 'Myth II.exe', '-noanalysis')
    } else {
        $arguments += @('-import', (Join-Path $projectRoot 'research/extracted/update-1.8.1/Myth II.exe'))
    }
    $jvmArguments = @('-Xmx2G', '-Xshare:off', '-XX:ParallelGCThreads=2', '-XX:CICompilerCount=2',
        '-Djava.system.class.loader=ghidra.GhidraClassLoader', '-Dfile.encoding=UTF8',
        '-Duser.country=US', '-Duser.language=en', '-Dlog4j.skipJansi=true',
        "-Duser.home=$stateRoot", "-Dapplication.settingsdir=$stateRoot/settings",
        "-Dapplication.cachedir=$stateRoot/cache", "-Dapplication.tempdir=$stateRoot/tmp",
        '-cp', (Join-Path $ghidraRoot 'Ghidra/Framework/Utility/lib/Utility.jar'),
        'ghidra.Ghidra', 'ghidra.app.util.headless.AnalyzeHeadless')
    $startedAt = [DateTime]::UtcNow
    & (Join-Path $jdkRoot 'bin/java.exe') @jvmArguments @arguments
    if ($LASTEXITCODE -ne 0) { throw "Ghidra exited with $LASTEXITCODE" }
    $indexPath = Join-Path $reportDir 'index.tsv'
    if (!(Test-Path $indexPath) -or (Get-Item $indexPath).LastWriteTimeUtc -lt $startedAt) {
        throw 'Ghidra did not produce a fresh export index; inspect script.log'
    }
    $exported = Import-Csv $indexPath -Delimiter "`t"
    foreach ($routine in $flow.routines) {
        $address = '{0:x8}' -f $routine.entry
        if (!($exported | Where-Object { $_.entry -eq $address -and $_.status -eq 'decompiled' })) {
            throw "Reviewed entry $address was not decompiled; inspect index.tsv"
        }
    }
