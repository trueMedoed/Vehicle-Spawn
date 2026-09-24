# Compare a completed Workbench batch with a versioned reference.
# Сравнивает завершённый пакет Workbench с сохранённым эталоном.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Report,
    [Parameter(Mandatory)][string]$GameVersion,
    [string]$Baseline = (Join-Path $PSScriptRoot '../docs/audits/1.8.0.13/worlds'),
    [switch]$ErrorsOnly,
    [string]$Output
)
$ErrorActionPreference = 'Stop'

# Parse and validate completeness before comparing results.
# Проверяет завершённость отчёта до сравнения результатов.
function Read-Audit([string]$Path) {
    # A directory contains independent, complete per-world reports.
    # Каталог содержит самостоятельные завершённые отчёты по мирам.
    if (Test-Path -LiteralPath $Path -PathType Container) {
        $combined = @{ Worlds = @{}; Issues = @{} }
        $files = @(Get-ChildItem -LiteralPath $Path -Filter '*.txt' -File | Sort-Object Name)
        if (!$files.Count) { throw "Empty report directory: $Path" }
        foreach ($file in $files) {
            $part = Read-Audit $file.FullName
            foreach ($world in $part.Worlds.Keys) {
                if ($combined.Worlds.ContainsKey($world)) { throw "Duplicate world: $world" }
                $combined.Worlds[$world] = $part.Worlds[$world]
            }
            foreach ($key in $part.Issues.Keys) { $combined.Issues[$key] = $part.Issues[$key] }
        }
        return $combined
    }
    $worlds = @{}
    $issues = @{}
    $currentWorld = ''
    $requested = 0
    $finished = $false
    foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -eq '# scope=errors_only' -and !$ErrorsOnly) { throw 'Use -ErrorsOnly for filtered reports' }
        $fields = @{}
        foreach ($m in [regex]::Matches($line, '(\w+)=(\S+)')) {
            $fields[$m.Groups[1].Value] = $m.Groups[2].Value
        }
        if ($line -match '^status=STARTED ') { $requested = [int]$fields.requested }
        if ($line -match '^status=ABORTED ') { throw "Aborted report: $Path" }
        if ($line -match '^status=WORLD_STARTED ') { $currentWorld = $fields.world }
        if ($line -match '^status=WORLD_FINISHED ') {
            if ($worlds.ContainsKey($fields.world)) { throw "Duplicate world: $($fields.world)" }
            $worlds[$fields.world] = $fields
        }
        if ($line -match '^world=.+ coverage=') {
            if (!$worlds.ContainsKey($fields.world)) { throw 'Coverage without world summary' }
            foreach ($key in $fields.Keys) { $worlds[$fields.world][$key] = $fields[$key] }
        }
        if ($line -match '^\[ME_DEBUG_AVSP_AUDIT\] status=(ERROR|WARNING|UNAVAILABLE) ') {
            foreach ($required in @('status','reason','coordinates','pointPrefab')) {
                if (!$fields[$required] -or $fields[$required] -eq '<unavailable>') { throw "Missing $required in $Path" }
            }
            if (!$currentWorld) { throw 'Diagnostic without world' }
            if ($ErrorsOnly -and $fields.status -ne 'ERROR') { continue }
            # Stable issue identity excludes runtime IDs and empty entity names.
            # Устойчивый ключ исключает runtime ID и пустые имена сущностей.
            $key = (@($currentWorld) + @('status','reason','coordinates','pointPrefab','object','objectCoordinates','overlappingCoordinates' | ForEach-Object { $fields[$_] })) -join ' | '
            if (!$issues.ContainsKey($key)) { $issues[$key] = 0 }
            $issues[$key]++
        }
        if ($line -match '^status=FINISHED ') {
            $finished = ([int]$fields.completed -eq $requested -and [int]$fields.requested -eq $requested)
        }
    }
    if (!$finished -or $requested -le 0 -or $worlds.Count -ne $requested) { throw "Incomplete report: $Path" }
    foreach ($w in $worlds.Values) {
        foreach ($key in @('result','scanned','passed','errorPoints','warningPoints','unavailablePoints','staticObjectIntersections','clearanceErrorPoints','coverage','sourceCheck','matched','missed','unresolvedEntities','unresolvedPointSources')) {
            if (!$w.ContainsKey($key)) { throw "Missing summary field $key" }
        }
    }
    return @{ Worlds = $worlds; Issues = $issues }
}

$before = Read-Audit $Baseline
$after = Read-Audit $Report
$text = [Collections.Generic.List[string]]::new()
$text.Add('# Сравнение аудита ambient-точек')
$text.Add('')
$text.Add("Версия проверяемой игры: $GameVersion (указана при запуске). Дата сравнения: $(Get-Date -Format yyyy-MM-dd).")
$text.Add('')
$text.Add("Эталон: $([IO.Path]::GetFileName($Baseline)). Новый отчёт: $([IO.Path]::GetFileName($Report)).")
$text.Add('')
$text.Add('Изменения требуют проверки: увеличение и уменьшение числа замечаний сами по себе не доказывают регрессию или исправление. NO_POINTS не означает отсутствие ошибок; UNVERIFIED не подтверждает полный охват мира.')
$text.Add('')
$summaryFields = @('result','scanned','passed','errorPoints','warningPoints','unavailablePoints','staticObjectIntersections','clearanceErrorPoints','coverage','sourceCheck','matched','missed','unresolvedEntities','unresolvedPointSources')
if ($ErrorsOnly) {
    $summaryFields = @('scanned','errorPoints','clearanceErrorPoints','unavailablePoints','coverage','sourceCheck','matched','missed','unresolvedEntities','unresolvedPointSources')
    $text.Add('Режим ERROR: сравниваются ошибки, число проверенных точек и полнота проверки. Предупреждения, общий result и passed исключены.')
    $text.Add('')
}
$changes = 0
foreach ($world in @(@($before.Worlds.Keys) + @($after.Worlds.Keys) | Sort-Object -Unique)) {
    if (!$before.Worlds.ContainsKey($world) -or !$after.Worlds.ContainsKey($world)) {
        $text.Add("- Изменился набор миров: $world"); $changes++; continue
    }
    foreach ($field in $summaryFields) {
        $a = $before.Worlds[$world][$field]; $b = $after.Worlds[$world][$field]
        if ($a -ne $b) { $text.Add("- ${world}: ${field}: $a → $b"); $changes++ }
    }
}
$text.Add('')
$text.Add('## Изменения замечаний')
$text.Add('')
foreach ($key in @(@($before.Issues.Keys) + @($after.Issues.Keys) | Sort-Object -Unique)) {
    $a = [int]$before.Issues[$key]; $b = [int]$after.Issues[$key]
    if ($a -ne $b) { $text.Add("- $key : количество $a → $b"); $changes++ }
}
if (!$changes) { $text.Add('Изменений счётчиков, покрытия и замечаний не обнаружено.') }
$text.Add('')
$text.Add('Координаты сравниваются точно, без допуска. Перемещение объекта может выглядеть как исчезновение старого и появление нового замечания. Изменение алгоритма аудита тоже влияет на результат.')
if ($Output) {
    $fullOutput = [IO.Path]::GetFullPath($Output)
    if ($fullOutput -eq [IO.Path]::GetFullPath($Report) -or $fullOutput -eq [IO.Path]::GetFullPath($Baseline)) { throw 'Output must not overwrite an input' }
    [IO.File]::WriteAllLines($fullOutput, $text, [Text.UTF8Encoding]::new($false))
} else { $text }
Write-Host "Comparison completed: $changes changes."
