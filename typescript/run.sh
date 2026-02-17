#!/bin/bash
echo "hi what is your name" | npx tsx src/main.ts -- --base-url https://api.anthropic.com/v1 --model claude-haiku-4-5 --api-key ANTHROPIC_API_KEY
