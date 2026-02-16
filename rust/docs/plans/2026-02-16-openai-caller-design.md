# Design: `super` — Simple OpenAI-compatible API Caller

## Purpose

A single-file Rust CLI that sends "What is 2+2?" to an OpenAI-compatible chat completions endpoint and prints the AI's response.

## Configuration

- `BASE_URL` and `MODEL` are `&str` constants at the top of `main.rs` — edit directly to change provider/model
- API key read from `OPENAI_API_KEY` environment variable at runtime

## Dependencies

- `reqwest` (blocking, with JSON feature) — HTTP client
- `serde` + `serde_json` — JSON serialization/deserialization

## Flow

1. Read `OPENAI_API_KEY` from env (exit with error if missing)
2. POST to `{BASE_URL}/chat/completions` with model and user message "What is 2+2?"
3. Parse JSON response, extract `choices[0].message.content`
4. Print content to stdout and exit

## Structure

Single file: `src/main.rs`. No modules.

## Error Handling

Print error messages to stderr, exit with code 1 on failure (missing key, HTTP error, parse error).
