package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
)

type Message struct {
	Role    string `json:"role"`
	Content string `json:"content"`
}

type ChatRequest struct {
	Model    string    `json:"model"`
	Messages []Message `json:"messages"`
}

type ChatResponse struct {
	Choices []Choice `json:"choices"`
}

type Choice struct {
	Message Message `json:"message"`
}

type Args struct {
	BaseURL string
	Model   string
	APIKey  string
}

func parseArgs(arguments []string) Args {
	fs := flag.NewFlagSet("super", flag.ContinueOnError)
	baseURL := fs.String("base-url", "", "Base URL for the API")
	model := fs.String("model", "", "Model name")
	apiKey := fs.String("api-key", "", "Environment variable name containing the API key")
	fs.Parse(arguments)
	return Args{
		BaseURL: *baseURL,
		Model:   *model,
		APIKey:  *apiKey,
	}
}

func executeQuery(baseURL, model, apiKey, content string) (string, error) {
	url := baseURL + "/chat/completions"

	reqBody := ChatRequest{
		Model: model,
		Messages: []Message{
			{Role: "user", Content: content},
		},
	}

	body, err := json.Marshal(reqBody)
	if err != nil {
		return "", fmt.Errorf("failed to marshal request: %w", err)
	}

	req, err := http.NewRequest(http.MethodPost, url, bytes.NewReader(body))
	if err != nil {
		return "", fmt.Errorf("failed to create request: %w", err)
	}
	req.Header.Set("Authorization", "Bearer "+apiKey)
	req.Header.Set("Content-Type", "application/json")

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return "", fmt.Errorf("request failed: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		errBody, _ := io.ReadAll(resp.Body)
		return "", fmt.Errorf("API returned status %d: %s", resp.StatusCode, string(errBody))
	}

	var chatResp ChatResponse
	if err := json.NewDecoder(resp.Body).Decode(&chatResp); err != nil {
		return "", fmt.Errorf("failed to decode response: %w", err)
	}

	if len(chatResp.Choices) == 0 {
		return "", fmt.Errorf("no choices in response")
	}

	return chatResp.Choices[0].Message.Content, nil
}

func main() {
	args := parseArgs(os.Args[1:])

	if args.BaseURL == "" || args.Model == "" || args.APIKey == "" {
		fmt.Fprintln(os.Stderr, "Usage: super --base-url URL --model MODEL --api-key ENV_VAR")
		os.Exit(1)
	}

	apiKey := os.Getenv(args.APIKey)
	if apiKey == "" {
		fmt.Fprintf(os.Stderr, "Error: %s environment variable not set\n", args.APIKey)
		os.Exit(1)
	}

	content, err := io.ReadAll(os.Stdin)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading stdin: %v\n", err)
		os.Exit(1)
	}

	prompt := string(bytes.TrimSpace(content))
	if prompt == "" {
		fmt.Fprintln(os.Stderr, "Error: No input provided on stdin")
		os.Exit(1)
	}

	fmt.Fprintf(os.Stderr, "base_url: %s\n", args.BaseURL)
	fmt.Fprintf(os.Stderr, "model:    %s\n", args.Model)
	fmt.Fprintf(os.Stderr, "api_key:  ******** (from %s)\n", args.APIKey)
	fmt.Fprintf(os.Stderr, "prompt:   %s\n", prompt)

	result, err := executeQuery(args.BaseURL, args.Model, apiKey, prompt)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		os.Exit(1)
	}

	fmt.Println(result)
}
