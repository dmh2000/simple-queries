# OpenAI-Compatible API Caller Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a single-file Rust CLI that sends "What is 2+2?" to an OpenAI-compatible chat completions endpoint and prints the response.

**Architecture:** A blocking HTTP POST to `{BASE_URL}/chat/completions` using reqwest. Request/response bodies are modeled with serde structs. Constants at top of main.rs control provider URL and model name.

**Tech Stack:** Rust, reqwest (blocking + json), serde, serde_json

---

### Task 1: Initialize Cargo Project

**Files:**
- Create: `Cargo.toml`
- Create: `src/main.rs`

**Step 1: Create the Cargo project**

Run: `cargo init /home/dmh2000/projects/super --name super`

**Step 2: Add dependencies to Cargo.toml**

Edit `Cargo.toml` to have these dependencies:

```toml
[dependencies]
reqwest = { version = "0.12", features = ["blocking", "json"] }
serde = { version = "1", features = ["derive"] }
serde_json = "1"
```

**Step 3: Verify it compiles**

Run: `cargo build`
Expected: Compiles successfully (with warnings about unused imports, that's fine)

**Step 4: Commit**

```bash
git add Cargo.toml Cargo.lock src/main.rs
git commit -m "feat: initialize cargo project with dependencies"
```

---

### Task 2: Define Request/Response Structs and Constants

**Files:**
- Modify: `src/main.rs`

**Step 1: Write the structs and constants**

Replace `src/main.rs` with:

```rust
use serde::{Deserialize, Serialize};

// ---- Configuration: edit these to change provider/model ----
const BASE_URL: &str = "https://api.openai.com/v1";
const MODEL: &str = "gpt-4o-mini";
// ------------------------------------------------------------

#[derive(Serialize)]
struct ChatRequest {
    model: String,
    messages: Vec<Message>,
}

#[derive(Serialize, Deserialize)]
struct Message {
    role: String,
    content: String,
}

#[derive(Deserialize)]
struct ChatResponse {
    choices: Vec<Choice>,
}

#[derive(Deserialize)]
struct Choice {
    message: Message,
}

fn main() {
    println!("structs defined");
}
```

**Step 2: Verify it compiles**

Run: `cargo build`
Expected: Compiles (may warn about unused items, that's fine)

**Step 3: Commit**

```bash
git add src/main.rs
git commit -m "feat: add request/response structs and config constants"
```

---

### Task 3: Implement Main Logic

**Files:**
- Modify: `src/main.rs`

**Step 1: Replace the main function**

Replace the `fn main()` block with:

```rust
fn main() {
    // Read API key from environment
    let api_key = match std::env::var("OPENAI_API_KEY") {
        Ok(key) => key,
        Err(_) => {
            eprintln!("Error: OPENAI_API_KEY environment variable not set");
            std::process::exit(1);
        }
    };

    let url = format!("{}/chat/completions", BASE_URL);

    let request_body = ChatRequest {
        model: MODEL.to_string(),
        messages: vec![Message {
            role: "user".to_string(),
            content: "What is 2+2?".to_string(),
        }],
    };

    let client = reqwest::blocking::Client::new();
    let response = match client
        .post(&url)
        .header("Authorization", format!("Bearer {}", api_key))
        .header("Content-Type", "application/json")
        .json(&request_body)
        .send()
    {
        Ok(resp) => resp,
        Err(e) => {
            eprintln!("Error: HTTP request failed: {}", e);
            std::process::exit(1);
        }
    };

    if !response.status().is_success() {
        eprintln!("Error: API returned status {}", response.status());
        std::process::exit(1);
    }

    let chat_response: ChatResponse = match response.json() {
        Ok(parsed) => parsed,
        Err(e) => {
            eprintln!("Error: Failed to parse response: {}", e);
            std::process::exit(1);
        }
    };

    if let Some(choice) = chat_response.choices.first() {
        println!("{}", choice.message.content);
    } else {
        eprintln!("Error: No choices in response");
        std::process::exit(1);
    }
}
```

**Step 2: Verify it compiles**

Run: `cargo build`
Expected: Compiles with no errors and no warnings

**Step 3: Commit**

```bash
git add src/main.rs
git commit -m "feat: implement main logic for chat completions API call"
```

---

### Task 4: Manual Smoke Test

**Step 1: Run with a valid API key**

Run: `OPENAI_API_KEY=<your-key> cargo run`
Expected: Prints something like "2+2 equals 4" or "4"

**Step 2: Run without API key to verify error handling**

Run: `unset OPENAI_API_KEY && cargo run`
Expected: Prints "Error: OPENAI_API_KEY environment variable not set" to stderr, exits 1

**Step 3: Final commit if any tweaks were needed**

```bash
git add -A
git commit -m "chore: finalize after smoke test"
```
