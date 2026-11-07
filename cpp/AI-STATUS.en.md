# AI Status (development, not released)

[Deutsch](AI-STATUS.md)

The optional `ai` page shows Claude Code and Codex cards at 480 × 320, their last observed state and its duration, tool and model, plus up to three sessions. Priority: waiting, working, idle; ties use latest event then stable provider/ID order. Working dots pulse, waiting dots blink softly, and session rows fade and move as sessions change or end. Fading rows retain their last ordinal (starting at 1). Animations use the existing render loop, without additional UI threads or sleeps. The existing `material-orange` default is dark with Fuchs orange; the page follows the selected display theme.

## Quick start and removal

1. Enable **AI Status** in native settings, select it for the intended display under **Pages**, save and restart ScreenBuddy. Its separate HTTP receiver binds only to `127.0.0.1:18765`; it does not use settings port 8765. An occupied port prevents reception. No firewall exception is needed. Add it to per-device Companion page lists as required.
2. **Preview setup** shows only the owned hook entries before and after the change for `%USERPROFILE%/.claude/settings.json` and `%USERPROFILE%/.codex/hooks.json`, plus the adapter path. Hook files are still untouched. Other settings, environment variables and foreign hook commands are excluded from the preview.
3. **Apply previewed change** creates the PowerShell adapter and a random 256-bit key in `<configuration directory>/ai-status/`. `key.dpapi` uses the existing Windows DPAPI mechanism, bound to the Windows account. The key is absent from hook commands and JSON and is never logged. Requires Windows PowerShell 5.1; execution policy is not bypassed.
4. Existing hook files receive a one-time `.bak` before their first change. Other entries are preserved. Files changed after preview are rejected. Orphaned regular `.screenbuddy-ai.tmp` files are replaced; temporary symlinks (including dangling links) are rejected. Failed writes/replacements clean up the temporary file created by this apply. Each replacement is atomic; the pair of files is not a single transaction. Reload the preview after a partial failure.
5. Restart clients and approve their own hook trust prompts if necessary. The adapter never approves permissions or returns policy decisions. It exits with code 0 and no output.

**Preview removal → Apply previewed change** removes only commands exactly matching this adapter and provider, preserving foreign commands even in shared groups. Empty event arrays may remain. The adapter, encrypted key and `.bak` remain locally. Disable the page, save and restart ScreenBuddy to stop its receiver. If moving the configuration directory, first remove old commands using the old adapter path. Backups may contain other private settings.

## Local event protocol

`POST http://127.0.0.1:18765/events`, header `X-ScreenBuddy-AI-Key: <local key>`, JSON, maximum 8192 bytes. No CORS; requests with `Origin` or `Sec-Fetch-Site` are rejected. No event files or transcripts are stored. Status is lost on app restart; session data remains missing until another hook arrives.

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

Additional authorized local producers must explicitly provide optional measurements: `context`, `five_hour`, `week` as percentages 0–100; `five_hour_reset`, `week_reset` as Unix seconds; `cost_today` as the provider-wide estimated total in USD today; `tokens_today` as the provider-wide integer token total today. Do not send token deltas: values replace previous totals rather than accumulating. Omitted fields retain the latest reported value. Daily values become unknown when the local date changes and are not persisted. Missing measurements display **n/a**, never invented quota usage or prices. Standard hooks do not automatically supply limits or context. No external account APIs, price lists or logs are queried.

## Tile sources

The built-in read-only `ai` source is available to declarative plugin/zone templates without an HTTP source, e.g. `{{ai.claude.state}}`, `{{ai.codex.tokens_today}}`, `{{ai.claude.s1.tool}}`. `ai` is reserved and cannot be overridden by plugins. Direct AI references in links are rejected.

| Under each `ai.claude` / `ai.codex` | Value |
| --- | --- |
| `state`, `tool`, `model`, `state_seconds` | Highest active priority; otherwise latest known off state, or off with unknown duration |
| `context`, `five_hour`, `week` | Percentage or `null` |
| `five_hour_reset`, `week_reset` | Unix seconds or `null` |
| `cost_today`, `tokens_today` | Reported daily total or `null` |
| `s1`, `s2`, `s3` | Up to three active sessions per provider, with state/tool/model/state_seconds, otherwise `null`; no private session IDs |

The standard page and its existing overlays retain their geometry. Custom views reuse its renderer for provider/limits tiles; see [Phase 8 P8-4](../docs/PHASE8-EIGENE-ANSICHTEN.md). The AI source is enabled separately.

## Validation and references

`fl-tests` includes parser, state, sorting, storage bounds, hook file, DPAPI, authenticated HTTP and rendering checks. All file writes use a unique temporary directory; receiver tests select an available loopback port in the dynamic range. `fl-tests ai-status <output directory>` exports DE/EN sample images. See [ai-status-report.md](ai-status-report.md) for builds and checks actually executed. Real client hooks, physical displays and multiple devices require separate acceptance.

Hook formats follow [Anthropic: Hooks reference](https://code.claude.com/docs/en/hooks) and [OpenAI Docs: Hooks](https://learn.chatgpt.com/docs/hooks), checked 2026-11-07. Independent implementation without libre-panel/GPL code.

## Usage query (phase 2)

Optional and switchable (AI Status settings page). A background worker queries at most one program at a time, sends no model message, uses an isolated home and no API credentials; after a failure it pauses for 15 minutes, and data older than 15 minutes counts as stale.

- **Codex:** via `codex app-server` (stdio). The 5-hour and weekly limits come from the reported windows (300 / 10080 minutes).
- **Claude Code:** limits, cost and context come through the status line (per the Claude Code docs: `rate_limits`, `cost`, `context_window` on stdin; Pro/Max/Ultra only, after the first API response). The adapter is registered as `statusLine` only if you have none of your own. Whether the desktop app/IDE run the status line is not documented; there it stays **n/a**. No separate CLI usage command without a model message is verified.
- The page shows where values came from and why there are none. Real CLI runs are not yet accepted.

## Editor tiles (P8-4)

Custom views offer `ai.claude`, `ai.codex`, `ai.limits` and individual metrics. Public conditions, density, per-device layouts and page touch targets use the existing tile contract. Missing data stays blank; missing limits stay `n/a`. See [usage, validation and port collision](../docs/PHASE8-EIGENE-ANSICHTEN.md).
