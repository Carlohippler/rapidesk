$jsonFile = "version.json"

# Se o arquivo não existir, cria com a versão 1.0
if (-not (Test-Path $jsonFile)) {
    $data = @{ version = 1.0 }
    $data | ConvertTo-Json | Set-Content $jsonFile
    Write-Output "1.0"
} else {
    # Se existir, lê, soma 0.1, arredonda e salva
    $data = Get-Content $jsonFile | ConvertFrom-Json
    $data.version = [math]::Round($data.version + 0.1, 1)
    $data | ConvertTo-Json | Set-Content $jsonFile
    Write-Output $data.version
}