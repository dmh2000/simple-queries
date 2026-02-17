#!/usr/bin/env python3

import argparse
import json
import os
import sys
import urllib.request


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Send a prompt to an OpenAI-compatible chat completions API"
    )
    parser.add_argument("--base-url", required=True, help="Base URL for the API")
    parser.add_argument("--model", required=True, help="Model name")
    parser.add_argument(
        "--api-key", required=True, help="Environment variable name containing the API key"
    )
    return parser.parse_args(argv)


def execute_query(base_url, model, api_key, content):
    url = f"{base_url}/chat/completions"

    request_body = json.dumps({
        "model": model,
        "messages": [{"role": "user", "content": content}],
    }).encode()

    req = urllib.request.Request(
        url,
        data=request_body,
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
        },
        method="POST",
    )

    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read())
    except urllib.error.HTTPError as e:
        body = e.read().decode(errors="replace")
        raise RuntimeError(f"API returned status {e.code}: {body}") from e

    if not data.get("choices"):
        raise RuntimeError("No choices in response")

    return data["choices"][0]["message"]["content"]


def main():
    args = parse_args()

    api_key = os.environ.get(args.api_key)
    if not api_key:
        print(f"Error: {args.api_key} environment variable not set", file=sys.stderr)
        sys.exit(1)

    content = sys.stdin.read().strip()
    if not content:
        print("Error: No input provided on stdin", file=sys.stderr)
        sys.exit(1)

    print(f"base_url: {args.base_url}", file=sys.stderr)
    print(f"model:    {args.model}", file=sys.stderr)
    print(f"api_key:  ******** (from {args.api_key})", file=sys.stderr)
    print(f"prompt:   {content}", file=sys.stderr)

    result = execute_query(args.base_url, args.model, api_key, content)
    print(result)


if __name__ == "__main__":
    main()
