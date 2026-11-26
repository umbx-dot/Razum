# Discord bots

The Discord source package lives outside the standard library. Bot logic, payload handling, rate-limit state, and Gateway sessions are written in Razum. The package uses the standard HTTP, JSON, time, threads, and crypt libraries.

## Run the ping bot

From the Discord example folder, build the bot with `rz build --profile release`. Create an application and bot in Discord’s Developer Portal and install the application in a test server with the `bot` and `applications.commands` scopes. Copy its application ID and the test server’s ID.

Set `DISCORD_TOKEN`, `DISCORD_APPLICATION_ID`, and optionally `DISCORD_GUILD_ID` in the process environment. Use a secret store or a private shell for the bot token. The application ID is needed for registration. Setting a guild ID registers the command in that guild. Leaving it unset registers a global command.

```powershell
.\build\windows-x64\release\bot.exe register
.\build\windows-x64\release\bot.exe
```

```sh
./build/linux-x64/release/bot register
./build/linux-x64/release/bot
```

Wait for `Discord bot ready` and run `/ping` in Discord. The example responds with an ephemeral `Pong!` visible to the caller. It uses the Guilds intent and works without the Message Content intent. See the [Discord quick start](https://docs.discord.com/developers/quick-start/getting-started) and [applications portal](https://discord.com/developers/applications).

## REST requests

Initialize a zeroed `DiscordBot` with `discord_init(&bot, token)`. REST requests can run before starting the Gateway. Use an empty `HttpClientResponse` and release it with `http_client_free` after examining the result.

| Function | Use |
|---|---|
| `discord_request(&bot, &response, method, route, json)` | Send an authenticated JSON request to a relative API v10 route. Pass an empty string for a request without a body. |
| `discord_request_raw(&bot, &response, method, route, data, size, content_type, headers, authenticate)` | Send binary or custom content, with optional CRLF-separated headers. Use `authenticate = false` for token URLs and endpoints using a caller-supplied OAuth authorization header. |
| `discord_send_message(&bot, &response, channel, content)` | Send safely escaped text. The generated message disables mentions by default. |
| `discord_command(&bot, &response, application, guild, definition)` | Register a command from Discord’s command JSON. Give an empty guild for a global command. |
| `discord_interaction_reply(&bot, &response, interaction, token, callback)` | Send a raw interaction response. Use Discord callback types for defers, updates, autocomplete, modals, or structured messages. |
| `discord_interaction_message(&bot, &response, interaction, token, content, ephemeral)` | Reply with a text message to an interaction. |
| `discord_upload(&bot, &response, method, route, json, attachments, authenticate)` | Send `payload_json` and up to ten binary files. Each `DiscordAttachment` supplies a filename, byte pointer, and size. |

`discord_request` can use Discord’s documented routes for channels, guilds, roles, moderation, members, threads, reactions, polls, webhooks, and integrations. For example, use `PATCH /channels/{channel}/messages/{message}` with the fields you want to edit. Discord IDs are decimal strings. Discord enforces each action’s permissions and limits. [Discord API reference](https://docs.discord.com/developers/reference)

REST returns an HTTP status or a negative transport error. Responses with a body include its exact byte length. A 429 sets `bot.retry_after_ms`. Known route or global cooldowns also return 429 locally until they expire. The library leaves retry decisions with the caller so a timeout never resends a write automatically. Each bot instance owns its own rate-limit state. [Discord rate limits](https://docs.discord.com/developers/topics/rate-limits)

## Gateway events

`discord_start(&bot, intents, shard, shard_count)` opens the Gateway. Start with shard zero of one for a small bot. A larger application can read Discord’s recommended shard count from `GET /gateway/bot` and run one bot state per shard, following the Identify concurrency limits for its application.

`discord_next(&bot, &event, timeout_ms)` returns 1 for a dispatch, 0 on a poll timeout, or a negative error. It handles Hello, Identify, heartbeat ACKs, reconnects, and Resume using the last sequence number and READY session ID. In the returned event, `name` contains the event name and `data` indexes the event’s JSON payload in `tokens`. Copy any data you need to keep before calling `discord_next` again. See the example for looking up an interaction’s command name.

Poll regularly to service heartbeats. For slow interaction work, acknowledge or defer promptly, then send a follow-up through the REST API. `discord_gateway_send` takes an opcode and raw `d` JSON, so bots can send presence updates and other Gateway commands. A fatal Gateway close stops the session and exposes the Discord close code. Use `discord_close(&bot)` to release the connection and buffers. [Discord Gateway lifecycle](https://docs.discord.com/developers/events/gateway)

## Signed HTTP interactions

Use `discord_verify_request(request, public_key_hex)` in a standard-library HTTP handler. It checks the Ed25519 signature over the timestamp and the exact request body bytes and allows five minutes of clock skew. Return 401 for a failed check. For a valid PING, reply with `{"type":1}`. Use a request or interaction ID to deduplicate deliveries when performing side effects.

`discord_verify_signature` accepts the raw signature fields and bytes separately when using another HTTP server. Discord can also deliver interactions over the Gateway, as shown by the sample. [Discord interaction signatures and response types](https://docs.discord.com/developers/interactions/receiving-and-responding)

## Platform support

Windows uses WinHTTP and the system certificate store. Linux uses Razum’s embedded libcurl backend, built with WSS support. The executable uses the system trust store on both platforms, with no Go runtime or curl executable. The package uses JSON Gateway payloads. Voice or video streams require Discord’s DAVE media encryption and are outside this version’s support. [Discord voice and DAVE](https://docs.discord.com/developers/topics/voice-connections)
