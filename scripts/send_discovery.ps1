$url = 'http://localhost:8000/api/sensors'
for ($i=1; $i -le 60; $i++) {
    $temp = Get-Random -Minimum 20.0 -Maximum 30.0
    $body = @{sensor_id='VIRTUAL_TEMP_01'; value=$temp} | ConvertTo-Json
    try { Invoke-RestMethod -Uri $url -Method Post -Body $body -ContentType 'application/json' | Out-Null } catch {}
    
    $hum = Get-Random -Minimum 40.0 -Maximum 60.0
    $body = @{sensor_id='VIRTUAL_HUMID_01'; value=$hum} | ConvertTo-Json
    try { Invoke-RestMethod -Uri $url -Method Post -Body $body -ContentType 'application/json' | Out-Null } catch {}
    
    Write-Host "Sent ping $i..."
    Start-Sleep -Seconds 3
}
