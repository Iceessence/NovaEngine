param(
  [string]$Repo = "Iceessence/NovaEngine",
  [int]$PollSeconds = 10,
  [switch]$RerunOnFail,
  [int]$MaxReruns = 2,
  [string]$WorkflowFile = ""   # e.g. ".github/workflows/windows-build.yml" if you want to trigger when no run exists
)

$ErrorActionPreference = "Stop"
$gh = (Get-Command gh).Source

& $gh repo set-default $Repo | Out-Null

function Save-LogsForRun($runId) {
  $dir = ".github\logs\$runId"
  New-Item -ItemType Directory -Force -Path $dir | Out-Null
  try { & $gh run view $runId --log > "$dir\run-all.log" } catch {}
  try {
    $jobIds = & $gh run view $runId --json jobs -q ".jobs[].databaseId"
    $jobIds | ForEach-Object { & $gh run view --job $_ --log > "$dir\job-$_.log" }
  } catch {}
  try { & $gh run download $runId --dir $dir 2>$null } catch {}
  if (Test-Path "$dir\run-all.log") {
    Select-String -Path "$dir\run-all.log" -Pattern "error C\d{4}|fatal error|LNK\d{4}" |
      Set-Content "$dir\errors.txt"
  }
  return $dir
}

function Latest-Run {
  $arr = & $gh run list --limit 1 --json databaseId,status,conclusion,displayTitle,number,workflowName,updatedAt | ConvertFrom-Json
  if ($arr.Count -ge 1) { return $arr[0] }
  return $null
}

# If there are no runs yet, optionally trigger one
$run = Latest-Run
if (-not $run -and $WorkflowFile) {
  Write-Host "No runs found. Triggering $WorkflowFile..."
  & $gh workflow run $WorkflowFile | Out-Null
  Start-Sleep -Seconds 3
  $run = Latest-Run
}

if (-not $run) {
  Write-Host "No runs found. Push a commit or enable workflow_dispatch on your workflow."
  exit 1
}

$reruns = 0

while ($true) {
  $run = Latest-Run
  if (-not $run) { Start-Sleep -Seconds $PollSeconds; continue }

  Write-Host ("[{0}] {1} (#{2}) status={3} conclusion={4}" -f (Get-Date), $run.displayTitle, $run.number, $run.status, $run.conclusion)

  if ($run.status -in @("queued","in_progress","waiting","requested")) {
    Start-Sleep -Seconds $PollSeconds
    continue
  }

  # Completed
  $logDir = Save-LogsForRun $run.databaseId

  if ($run.conclusion -eq "success") {
    Write-Host "✅ CI succeeded. Logs: $logDir"
    Start-Process explorer $logDir
    break
  }

  Write-Host "❌ CI failed. Logs: $logDir"

  if ($RerunOnFail -and $reruns -lt $MaxReruns) {
    $reruns++
    Write-Host "Re-running (attempt $reruns of $MaxReruns)..."
    try {
      & $gh run rerun $run.databaseId | Out-Null
      Start-Sleep -Seconds 5
      continue
    } catch {
      Write-Warning "Rerun failed: $_"
      break
    }
  } else {
    Write-Host "Stopping (no rerun requested or max reruns reached)."
    Start-Process explorer $logDir
    break
  }
}
