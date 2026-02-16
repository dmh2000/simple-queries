# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Multi-language repository implementing simple CLI tools that send prompts to OpenAI-compatible chat completions APIs. Each language lives in its own top-level directory. Currently only Rust is implemented; Go, Python, TypeScript, and C++ are placeholders.

All implementations should follow the same pattern: read a prompt from stdin, call an OpenAI-compatible `/chat/completions` endpoint, and print the response to stdout. CLI arguments: `--base-url`, `--model`, `--api-key` (env var name).

## Rust (`rust/`)

### Build and Run

```bash
# Build
cargo build --manifest-path rust/Cargo.toml

# Run (reads prompt from stdin)
echo "hello" | cargo run --manifest-path rust/Cargo.toml -- \
  --base-url https://api.anthropic.com/v1 \
  --model claude-haiku-4-5 \
  --api-key ANTHROPIC_API_KEY

# Or from within rust/
cd rust && echo "hello" | cargo run -- --base-url https://api.anthropic.com/v1 --model claude-haiku-4-5 --api-key ANTHROPIC_API_KEY
```

### Architecture

Single-file implementation in `rust/src/main.rs`. Binary name is `super` (package `super_cli`). Uses `clap` (derive) for CLI args, `reqwest` (blocking + rustls-tls) for HTTP, `serde` for JSON serialization. No tests or lint config yet.

## Conventions

- Each language implementation is self-contained in its directory with its own build system.
- Debug info (base URL, model, redacted key, prompt) is printed to stderr; only the API response goes to stdout.
- API keys are passed indirectly via `--api-key` which names an environment variable (not the key itself).
