# AI-Assisted Analysis

SAVT now includes a bounded AI adapter for architecture explanations. It still keeps the original SAVT-only guardrails, but it can now talk to either the official DeepSeek API or an OpenAI-compatible proxy/gateway endpoint.

## What exists right now

- A dedicated `savt_ai` module for loading AI configuration and building chat-completions requests.
- A restricted SAVT-only system prompt that limits the model to architecture-reading tasks.
- A structured evidence package builder so the UI only sends bounded module evidence instead of whole repositories.
- A live desktop-side integration: after selecting a node, the right inspector can trigger the AI explanation card for that node.

## Network boundary

This integration is API-only. It requires network access and is not a local model runtime.

## Where to fill your API settings

1. Copy `config/deepseek-ai.template.json` to `config/deepseek-ai.local.json`.
2. Fill in your `apiKey`.
3. Set `enabled` to `true`.

`config/deepseek-ai.local.json` is ignored by git so your real key stays out of the repository.

## Official DeepSeek example

```json
{
  "provider": "deepseek",
  "enabled": true,
  "baseUrl": "https://api.deepseek.com",
  "endpointPath": "",
  "model": "deepseek-chat",
  "apiKey": "your-real-deepseek-api-key",
  "timeoutMs": 30000
}
```

With the official DeepSeek URL, SAVT will call:

`https://api.deepseek.com/chat/completions`

## Cherry Studio style / compatible endpoint example

If your platform gives you an API base address like `https://chat.cqjtu.edu.cn/ds/api`, you can fill it directly.

```json
{
  "provider": "cherry-studio-proxy",
  "enabled": true,
  "baseUrl": "https://chat.cqjtu.edu.cn/ds/api",
  "endpointPath": "",
  "model": "deepseek-chat",
  "apiKey": "your-compatible-api-key",
  "timeoutMs": 30000
}
```

SAVT will automatically resolve that to:

`https://chat.cqjtu.edu.cn/ds/api/v1/chat/completions`

If your gateway needs a different path, set `endpointPath` explicitly, for example:

```json
{
  "provider": "custom-gateway",
  "enabled": true,
  "baseUrl": "https://example.com/proxy",
  "endpointPath": "/openai/chat/completions",
  "model": "deepseek-chat",
  "apiKey": "your-key",
  "timeoutMs": 30000
}
```

## Current rule boundary

The built-in SAVT AI policy forces the model to:

- work only on SAVT architecture reading tasks
- refuse unrelated requests and general chat
- use only the supplied evidence package
- avoid inventing files, modules, symbols, or business meaning
- reply in Simplified Chinese
- return structured JSON for UI consumption

## Current implementation scope

This step now covers adapter/config loading, SAVT-only rules, request payloads, response parsing, and a manual UI trigger in the desktop inspector.
The current UI is still intentionally conservative: AI is manually triggered per selected node and is kept separate from the static-analysis facts shown elsewhere in the tool.
