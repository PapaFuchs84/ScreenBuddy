# Synthetic metadata only. Parses the production adapter without decrypting a key or using HTTP.
$ErrorActionPreference='Stop'
$aiSource=Get-Content "$PSScriptRoot/../src/ai_status.cpp" -Raw
$aiStart=$aiSource.IndexOf('R"PS(param(')+5
$aiEnd=$aiSource.IndexOf(')PS";', $aiStart)
$aiScript=$aiSource.Substring($aiStart,$aiEnd-$aiStart)
$aiScript=$aiScript.Replace(')PS"+std::to_string(d_->port)+R"PS(', '18765')
$aiTokens=$null; $aiErrors=$null
[void][Management.Automation.Language.Parser]::ParseInput($aiScript,[ref]$aiTokens,[ref]$aiErrors)
if($aiErrors.Count) {throw 'Adapter syntax failed'}
$aiPrefix=$aiScript.Substring(0,$aiScript.IndexOf('  Add-Type -AssemblyName'))
$aiPrefix=$aiPrefix.Replace('[Console]::In.ReadToEnd()','$AiSyntheticInput')+'  $body | ConvertTo-Json -Compress'+"`n} catch { throw }"
$aiBlock=[scriptblock]::Create($aiPrefix)
$AiSyntheticInput='{"session_id":"synthetic","cost":{"total_cost_usd":0.125},"context_window":{"used_percentage":0},"rate_limits":{"five_hour":{"used_percentage":0,"resets_at":1800000500},"seven_day":{"used_percentage":51,"resets_at":1800000700}},"model":{"id":"example-model"},"prompt":"synthetic-private","tool_input":{"private":"synthetic"}}'
$aiResult=(& $aiBlock -Provider claude)|ConvertFrom-Json
$aiChecks=@(
    $aiResult.source -eq 'claude-statusline'
    $aiResult.hook_event_name -eq 'Usage'
    $aiResult.cost_session -eq 0.125
    $aiResult.context -eq 0
    $aiResult.five_hour -eq 0
    $aiResult.week -eq 51
    $aiResult.five_hour_reset -eq 1800000500
    $aiResult.week_reset -eq 1800000700
    $aiResult.model -eq 'example-model'
    -not ($aiResult.PSObject.Properties.Name -contains 'prompt')
    -not ($aiResult.PSObject.Properties.Name -contains 'tool_input')
    -not ($aiResult.PSObject.Properties.Name -contains 'cost_today')
)
$AiSyntheticInput='{"session_id":"synthetic","hook_event_name":"Stop","model":"example-model"}'
$aiMissing=(& $aiBlock -Provider claude)|ConvertFrom-Json
$aiChecks+=@($aiMissing.hook_event_name -eq 'Stop'; -not ($aiMissing.PSObject.Properties.Name -contains 'five_hour'); -not ($aiMissing.PSObject.Properties.Name -contains 'cost_session'))
$aiFailed=@($aiChecks|Where-Object {$_ -ne $true}).Count
Write-Output "AI adapter: $($aiChecks.Count-$aiFailed) passed, $aiFailed failed; AST syntax checked; no DPAPI/HTTP"
if($aiFailed) {exit 1}
