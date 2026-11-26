# Razum Discord package

`import discord;` provides Discord REST requests, Gateway events, command registration, interaction replies, file uploads and signed HTTP request verification. Run `rz add umbx-dot/razum-discord` from a Razum project to install it.

To try the example, clone this repository and run `rz build --profile release`. Set `DISCORD_TOKEN` to a bot token. Set `DISCORD_APPLICATION_ID` and optionally `DISCORD_GUILD_ID`, launch the built bot with `register`, then launch it without arguments. Run `/ping` in Discord.

Requires [Razum 0.2.0](https://github.com/umbx-dot/Razum/releases/tag/v0.2.0) or later. The API guide covers Linux WSS requirements, event handling and supported features. [Discord guide](https://github.com/umbx-dot/Razum/blob/v0.2.0/docs/discord.md)
