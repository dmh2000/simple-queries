use clap::Parser;
use serde::{Deserialize, Serialize};
use std::io::{self, Read};

#[derive(Parser)]
#[command(about = "Send a prompt to an OpenAI-compatible chat completions API")]
struct Args {
    /// Base URL for the API (e.g. https://api.anthropic.com/v1)
    #[arg(long)]
    base_url: String,

    /// Model name (e.g. claude-haiku-4-5)
    #[arg(long)]
    model: String,

    /// Name of the environment variable containing the API key
    #[arg(long)]
    api_key: String,
}

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

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Args::parse();

    // Read API key from the named environment variable
    let api_key = std::env::var(&args.api_key)
        .map_err(|_| format!("Error: {} environment variable not set", args.api_key))?;

    // Read message content from stdin
    let mut content = String::new();
    io::stdin().read_to_string(&mut content)?;
    let content = content.trim().to_string();
    if content.is_empty() {
        return Err("Error: No input provided on stdin".into());
    }

    // Debug: print arguments and input (redact API key)
    eprintln!("base_url: {}", args.base_url);
    eprintln!("model:    {}", args.model);
    eprintln!("api_key:  ******** (from {})", args.api_key);
    eprintln!("prompt:   {}", content);

    let chat_response = execute_query(&args, &api_key, &content)?;

    if let Some(choice) = chat_response.choices.first() {
        println!("{}", choice.message.content);
    } else {
        return Err("Error: No choices in response".into());
    }

    Ok(())
}

fn execute_query(
    args: &Args,
    api_key: &str,
    content: &str,
) -> Result<ChatResponse, Box<dyn std::error::Error>> {
    let url = format!("{}/chat/completions", args.base_url);

    let request_body = ChatRequest {
        model: args.model.clone(),
        messages: vec![Message {
            role: "user".to_string(),
            content: content.to_string(),
        }],
    };

    let client = reqwest::blocking::Client::new();
    let response = client
        .post(&url)
        .header("Authorization", format!("Bearer {}", api_key))
        .header("Content-Type", "application/json")
        .json(&request_body)
        .send()?;

    if !response.status().is_success() {
        let status = response.status();
        let body = response.text().unwrap_or_else(|_| "Could not read error body".into());
        return Err(format!("Error: API returned status {}: {}", status, body).into());
    }

    let chat_response: ChatResponse = response.json()?;
    Ok(chat_response)
}
