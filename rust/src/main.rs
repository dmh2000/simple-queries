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

#[derive(Debug, Serialize)]
struct ChatRequest {
    model: String,
    messages: Vec<Message>,
}

#[derive(Debug, Serialize, Deserialize)]
struct Message {
    role: String,
    content: String,
}

#[derive(Debug, Deserialize)]
struct ChatResponse {
    choices: Vec<Choice>,
}

#[derive(Debug, Deserialize)]
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

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::{BufRead, BufReader, Write};
    use std::net::TcpListener;

    fn start_fake_server(status: u16, body: &str) -> String {
        let listener = TcpListener::bind("127.0.0.1:0").unwrap();
        let port = listener.local_addr().unwrap().port();
        let body = body.to_string();

        std::thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let mut reader = BufReader::new(stream.try_clone().unwrap());

            // Read request headers until empty line
            let mut content_length = 0usize;
            loop {
                let mut line = String::new();
                reader.read_line(&mut line).unwrap();
                if let Some(len) = line.strip_prefix("Content-Length: ") {
                    content_length = len.trim().parse().unwrap();
                }
                if line.trim().is_empty() {
                    break;
                }
            }

            // Read request body
            let mut req_body = vec![0u8; content_length];
            reader.read_exact(&mut req_body).unwrap();

            let status_text = if status == 200 { "OK" } else { "Error" };
            let response = format!(
                "HTTP/1.1 {} {}\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}",
                status, status_text, body.len(), body
            );
            stream.write_all(response.as_bytes()).unwrap();
        });

        format!("http://127.0.0.1:{}", port)
    }

    fn make_args(base_url: &str, model: &str) -> Args {
        Args {
            base_url: base_url.to_string(),
            model: model.to_string(),
            api_key: "TEST_KEY".to_string(),
        }
    }

    #[test]
    fn test_execute_query_success() {
        let body = r#"{"choices":[{"message":{"role":"assistant","content":"Hi there!"}}]}"#;
        let base_url = start_fake_server(200, body);
        let args = make_args(&base_url, "test-model");

        let result = execute_query(&args, "test-key", "hello").unwrap();
        assert_eq!(result.choices[0].message.content, "Hi there!");
        assert_eq!(result.choices[0].message.role, "assistant");
    }

    #[test]
    fn test_execute_query_api_error() {
        let body = r#"{"error":"invalid api key"}"#;
        let base_url = start_fake_server(401, body);
        let args = make_args(&base_url, "test-model");

        let err = execute_query(&args, "bad-key", "hello").unwrap_err();
        let msg = err.to_string();
        assert!(msg.contains("401"), "expected '401' in error: {}", msg);
    }

    #[test]
    fn test_execute_query_empty_choices() {
        let body = r#"{"choices":[]}"#;
        let base_url = start_fake_server(200, body);
        let args = make_args(&base_url, "test-model");

        let result = execute_query(&args, "test-key", "hello").unwrap();
        assert!(result.choices.is_empty());
    }

    #[test]
    fn test_execute_query_invalid_json() {
        let base_url = start_fake_server(200, "not json");
        let args = make_args(&base_url, "test-model");

        let result = execute_query(&args, "test-key", "hello");
        assert!(result.is_err());
    }
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
