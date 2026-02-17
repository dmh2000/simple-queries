# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Multi-language repository implementing simple CLI tools that send prompts to OpenAI-compatible chat completions APIs. Each language lives in its own top-level directory. Rust, Go, Python, and TypeScript are implemented; C++ is a placeholder.

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

## Go (`go/`)

### Build, Run, and Test

```bash
# Build
go build -C go -o super .

# Run (reads prompt from stdin)
echo "hello" | go run ./go --base-url https://api.anthropic.com/v1 --model claude-haiku-4-5 --api-key ANTHROPIC_API_KEY

# Run tests
go test -v ./go/...
```

### Architecture

Single-file implementation in `go/main.go` (module `super_cli`). Uses only standard library: `flag` for CLI args, `net/http` for HTTP, `encoding/json` for JSON. Tests in `go/main_test.go` use `net/http/httptest` for mock API server.

## Python (`python/`)

### Run and Test

```bash
# Run (reads prompt from stdin)
echo "hello" | python3 python/super_cli.py --base-url https://api.anthropic.com/v1 --model claude-haiku-4-5 --api-key ANTHROPIC_API_KEY

# Run tests
python3 -m unittest python/test_super -v
```

### Architecture

Single-file implementation in `python/super_cli.py`. Uses only standard library: `argparse` for CLI args, `urllib.request` for HTTP, `json` for serialization. Tests in `python/test_super.py` use `http.server.HTTPServer` with a threaded fake handler.

## TypeScript (`typescript/`)

### Setup, Run, and Test

```bash
# Install dependencies (from typescript/)
cd typescript && npm install

# Run (reads prompt from stdin, uses tsx)
echo "hello" | npx tsx src/main.ts -- --base-url https://api.anthropic.com/v1 --model claude-haiku-4-5 --api-key ANTHROPIC_API_KEY

# Run tests
npm test
```

### Architecture

Single-file implementation in `typescript/src/main.ts`. Uses `tsx` as the runner (dev dependency). All runtime code uses Node.js built-ins only: `util.parseArgs` for CLI args, `node:http`/`node:https` for HTTP, JSON for serialization. Tests in `src/main.test.ts` use `node:test` with a real `http.createServer`.

## Conventions

- Each language implementation is self-contained in its directory with its own build system.
- Debug info (base URL, model, redacted key, prompt) is printed to stderr; only the API response goes to stdout.
- API keys are passed indirectly via `--api-key` which names an environment variable (not the key itself).
