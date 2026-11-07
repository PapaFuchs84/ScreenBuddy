# AI Status (development, not released)

[Deutsch](AI-STATUS.md)

The optional `ai` page shows Claude Code and Codex cards at 480 × 320, their last observed state and its duration, tool and model, plus up to three sessions. Priority: waiting, working, idle; ties use latest event then stable provider/ID order. Working dots pulse, waiting dots blink softly, and session rows fade and move as sessions change or end. Fading rows retain their last ordinal (starting at 1). Animations use the existing render loop, without additional UI threads or sleeps. The existing `material-orange` default is dark with Fuchs orange; the page follows the selected display theme.

## Quick start and removal

1. Enable **AI Status** in native settings, select it for the intended display under **Pages**, save and restart ScreenBuddy. Its separate HTTP receiver binds only to loopback (default 18765, configurable with automatic fallback); it does not use settings port 8765. An occupied port selects a free fallback; the status line shows the actual port. No firewall exception is needed. Add it to per-device Companion page lists as required.
2. **Preview setup** shows only the owned hook entries before and after the change for `%USERPROFILE%/.claude/settings.json` and `%USERPROFILE%/.codex/hooks.json`, plus the adapter path. Hook files are still untouched. Other settings, environment variables and foreign hook commands are excluded from the preview.
3. **Apply previewed change** creates the PowerShell adapter and a random 256-bit key in `<configuration directory>/ai-status/`. `key.dpapi` uses the existing Windows DPAPI mechanism, bound to the Windows account. The key is absent from hook commands and JSON and is never logged. Requires Windows PowerShell 5.1; execution policy is not bypassed.
4. Existing hook files receive a one-time `.bak` before their first change. Other entries are preserved. Files changed after preview are rejected. Orphaned regular `.screenbuddy-ai.tmp` files are replaced; temporary symlinks (including dangling links) are rejected. Failed writes/replacements clean up the temporary file created by this apply. Each replacement is atomic; the pair of files is not a single transaction. Reload the preview after a partial failure.
5. Restart clients and approve their own hook trust prompts if necessary. The adapter never approves permissions or returns policy decisions. It exits with code 0 and no output.

**Preview removal → Apply previewed change** removes only commands exactly matching this adapter and provider, preserving foreign commands even in shared groups. Empty event arrays may remain. The adapter, encrypted key and `.bak` remain locally. Disable the page, save and restart ScreenBuddy to stop its receiver. If moving the configuration directory, first remove old commands using the old adapter path. Backups may contain other private settings.

## Local event protocol

`POST http://127.0.0.1:<actual-port>/events` (Standard/default 18765), header `X-ScreenBuddy-AI-Key: <local key>`, JSON, maximum 8192 bytes. No CORS; requests with `Origin` or `Sec-Fetch-Site` are rejected. No event files or transcripts are stored. Status is lost on app restart; session data remains missing until another hook arrives.

Example using invented data only (key intentionally omitted):

```json
{
  "provider": "claude",
  "session_id": "example-1",
  "hook_event_name": "PreToolUse",
  "tool_name": "Read",
  "model": "example-model",
  "context": 42,
  "five_hour": 23,
  "five_hour_reset": 1800000500
}
```

Providers: `claude` or `codex`. Events: `SessionStart` → idle; `UserPromptSubmit`/`PreToolUse`/`PostToolUse`/`PostToolUseFailure` → working; `PermissionRequest` → waiting; `Stop`/`Interrupt` → idle; `SessionEnd` → off. Late tool/stop deliveries cannot resurrect an ended session; SessionStart can. Prompt, Stop, Interrupt and end clear the tool. Storage is capped at 128 sessions, evicting ended sessions first. Without an end hook the last observed state remains and the footer shows its age. No timeout detection is invented.

The adapter forwards only session ID, event, tool name and model when present. It does not read transcript or credential files; prompts and tool arguments present on hook stdin are not inspected or forwarded. It waits approximately 1.2 seconds at most for HTTP so work can continue; command hooks have a three-second timeout. Restrictive PowerShell policies or cloud-only Codex sessions may prevent local command hooks from running. Adapter paths containing shell metacharacters are rejected.

Additional authorized local producers must explicitly provide optional measurements: `context`, `five_hour`, `week` as percentages 0–100; `five_hour_reset`, `week_reset` as Unix seconds; `cost_today` as the provider-wide estimated total in USD today; `tokens_today` as the provider-wide integer token total today. Do not send token deltas: values replace previous totals rather than accumulating. Omitted fields retain the latest reported value. Daily values become unknown when the local date changes and are not persisted. Missing measurements display **n/a**, never invented quota usage or prices. Standard hooks do not automatically supply limits or context. ScreenBuddy does not read sign-in files, price lists or transcripts. Optional account usage through the provider client is described below.

## Tile sources

The built-in read-only `ai` source is available to declarative plugin/zone templates without an HTTP source, e.g. `{{ai.claude.state}}`, `{{ai.codex.tokens_today}}`, `{{ai.claude.s1.tool}}`. `ai` is reserved and cannot be overridden by plugins. Direct AI references in links are rejected.

| Under each `ai.claude` / `ai.codex` | Value |
| --- | --- |
| `state`, `tool`, `model`, `state_seconds` | Highest active priority; otherwise latest known off state, or off with unknown duration |
| `context`, `five_hour`, `week` | Percentage or `null` |
| `five_hour_reset`, `week_reset` | Unix seconds or `null` |
| `cost_today`, `tokens_today` | Reported daily total or `null` |
| `s1`, `s2`, `s3` | Up to three active sessions per provider, with state/tool/model/state_seconds, otherwise `null`; no private session IDs |

The standard page and existing built-in arrangement renderer use the same status data. The current checkout includes AI in the built-in catalog. Excluded editor files were not changed in this task; this does not establish interactive editor acceptance.

## Validation and references

`fl-tests` includes parser, state, sorting, storage bounds, hook file, DPAPI, authenticated HTTP and rendering checks. All file writes use a unique temporary directory; receiver tests select an available loopback port in the dynamic range. `fl-tests ai-status <output directory>` exports DE/EN sample images. See [ai-status-report.md](ai-status-report.md) for builds and checks actually executed. Real client hooks, physical displays and multiple devices require separate acceptance.

Hook formats follow [Anthropic: Hooks reference](https://code.claude.com/docs/en/hooks) and [OpenAI Docs: Hooks](https://learn.chatgpt.com/docs/hooks), checked 2026-11-07. Independent implementation without libre-panel/GPL code.

## Usage queries and real accounts (opt-in)

Existing separate **Use real account read-only** switches (ai.codex_real_account / ai.claude_real_account), default OFF. Enable automatic usage queries, read consent, confirm activation, save and restart. Switching off pauses the entire worker until restart; in-flight HTTPS requests finish within their time limits and results are discarded. Hook setup requires separate confirmation.

**Risk:** Unofficial interfaces, subject to change; provider tolerance is unclear. Read-only GET requests without model messages. Tokens leave the computer only over HTTPS to the respective provider. No token storage, display, logging or refresh. Login files are never written; refresh tokens are never used. Windows files only, no Keychain access.

- Claude: only %USERPROFILE%/.claude/.credentials.json, field claudeAiOauth.accessToken; GET https://api.anthropic.com/api/oauth/usage, Bearer and anthropic-beta: oauth-2025-04-20. five_hour/seven_day with utilization and ISO-8601 resets. Optional model windows and extra_usage are tolerated but not presented as overall limits or daily costs.
- Codex: only %USERPROFILE%/.codex/auth.json, field tokens.access_token, optional tokens.account_id as ChatGPT-Account-Id; GET https://chatgpt.com/backend-api/wham/usage. Windows map by limit_window_seconds: 18000 = 5h, 604800 = week, regardless of primary/secondary. secondary_window may be null, percentages are clamped to 0–100. API keys do not replace login tokens.
- Missing file: Not signed in. Invalid/incomplete file: readable reason. Expired Claude timestamp, expired Codex JWT or HTTP 401: Token expired: start Claude/Codex once. Errors clear direct limits to n/a.

One background worker schedules providers independently at least 3 minutes apart; every failure (including 401/403/429 or missing login) pauses that provider for 15 minutes. Toggling cannot bypass the pause. Account mode starts no provider programs. Without account consent the existing isolated CLI path remains, without user login files. Hooks/Claude status line still supply context and session costs. No invented daily totals.

Fixed HTTPS hosts/paths, all redirects disabled; Windows certificate/hostname checks enabled. No proxy, cookies or automatic HTTP authentication. Time limits: 3 seconds per resolution/connection/send/receive step, plus a 12-second body-read deadline. Maximum 64 KiB response and 256 KiB login file. Owned credential/parser/header buffers are wiped on release, including failures. Operating system/WinHTTP internal copies are outside owned buffer management.

ai.port is configurable in config and AI settings: default 18765, 1024–65535, settings port excluded. Occupied ports fall back to a free dynamic port. The settings page status line and shared hook/Claude-status-line adapter use the actual port. DPAPI/file failures keep the receiver inactive.

Executed counts and pending account/UI/hardware acceptance: [report](ai-status-report.md). This task uses synthetic responses and temporary files only, no real logins or provider network requests.

## Editor tile (interface)

Data: template source `ai` (see above). Existing AI built-in blocks and their renderer remain available. No palette or editor changes in this task.
