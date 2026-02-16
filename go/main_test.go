package main

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
)

func TestExecuteQuery_Success(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Verify request method and path
		if r.Method != http.MethodPost {
			t.Errorf("expected POST, got %s", r.Method)
		}
		if r.URL.Path != "/chat/completions" {
			t.Errorf("expected /chat/completions, got %s", r.URL.Path)
		}

		// Verify headers
		if r.Header.Get("Authorization") != "Bearer test-key" {
			t.Errorf("expected Bearer test-key, got %s", r.Header.Get("Authorization"))
		}
		if r.Header.Get("Content-Type") != "application/json" {
			t.Errorf("expected application/json, got %s", r.Header.Get("Content-Type"))
		}

		// Verify request body
		var req ChatRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			t.Fatalf("failed to decode request body: %v", err)
		}
		if req.Model != "test-model" {
			t.Errorf("expected model test-model, got %s", req.Model)
		}
		if len(req.Messages) != 1 || req.Messages[0].Role != "user" || req.Messages[0].Content != "hello" {
			t.Errorf("unexpected messages: %+v", req.Messages)
		}

		// Return a valid response
		resp := ChatResponse{
			Choices: []Choice{
				{Message: Message{Role: "assistant", Content: "Hi there!"}},
			},
		}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(resp)
	}))
	defer server.Close()

	result, err := executeQuery(server.URL, "test-model", "test-key", "hello")
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if result != "Hi there!" {
		t.Errorf("expected 'Hi there!', got '%s'", result)
	}
}

func TestExecuteQuery_APIError(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusUnauthorized)
		w.Write([]byte("invalid api key"))
	}))
	defer server.Close()

	_, err := executeQuery(server.URL, "test-model", "bad-key", "hello")
	if err == nil {
		t.Fatal("expected error for 401 response, got nil")
	}
}

func TestExecuteQuery_EmptyChoices(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		resp := ChatResponse{Choices: []Choice{}}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(resp)
	}))
	defer server.Close()

	_, err := executeQuery(server.URL, "test-model", "test-key", "hello")
	if err == nil {
		t.Fatal("expected error for empty choices, got nil")
	}
}

func TestExecuteQuery_InvalidJSON(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		w.Write([]byte("not json"))
	}))
	defer server.Close()

	_, err := executeQuery(server.URL, "test-model", "test-key", "hello")
	if err == nil {
		t.Fatal("expected error for invalid JSON, got nil")
	}
}

func TestParseArgs_AllFlags(t *testing.T) {
	args := parseArgs([]string{"--base-url", "http://localhost", "--model", "gpt-4", "--api-key", "MY_KEY"})
	if args.BaseURL != "http://localhost" {
		t.Errorf("expected base-url http://localhost, got %s", args.BaseURL)
	}
	if args.Model != "gpt-4" {
		t.Errorf("expected model gpt-4, got %s", args.Model)
	}
	if args.APIKey != "MY_KEY" {
		t.Errorf("expected api-key MY_KEY, got %s", args.APIKey)
	}
}

func TestParseArgs_MissingRequired(t *testing.T) {
	// Missing --model and --api-key
	args := parseArgs([]string{"--base-url", "http://localhost"})
	if args.Model != "" {
		t.Errorf("expected empty model, got %s", args.Model)
	}
	if args.APIKey != "" {
		t.Errorf("expected empty api-key, got %s", args.APIKey)
	}
}
