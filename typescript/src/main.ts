import http from "node:http";
import https from "node:https";
import { parseArgs as nodeParseArgs } from "node:util";

interface Message {
  role: string;
  content: string;
}

interface ChatRequest {
  model: string;
  messages: Message[];
}

interface ChatResponse {
  choices: { message: Message }[];
}

interface Args {
  baseUrl: string;
  model: string;
  apiKey: string;
}

export function parseArgs(argv: string[]): Args {
  const { values } = nodeParseArgs({
    args: argv,
    options: {
      "base-url": { type: "string" },
      model: { type: "string" },
      "api-key": { type: "string" },
    },
    strict: true,
  });

  const baseUrl = values["base-url"];
  const model = values["model"];
  const apiKey = values["api-key"];

  if (!baseUrl || !model || !apiKey) {
    throw new Error("Usage: super --base-url URL --model MODEL --api-key ENV_VAR");
  }

  return { baseUrl, model, apiKey };
}

export function executeQuery(
  baseUrl: string,
  model: string,
  apiKey: string,
  content: string
): Promise<string> {
  return new Promise((resolve, reject) => {
    const url = new URL("/chat/completions", baseUrl);
    const transport = url.protocol === "https:" ? https : http;

    const body = JSON.stringify({
      model,
      messages: [{ role: "user", content }],
    } satisfies ChatRequest);

    const req = transport.request(
      url,
      {
        method: "POST",
        headers: {
          Authorization: `Bearer ${apiKey}`,
          "Content-Type": "application/json",
          "Content-Length": Buffer.byteLength(body).toString(),
        },
      },
      (res) => {
        let data = "";
        res.on("data", (chunk) => (data += chunk));
        res.on("end", () => {
          if (res.statusCode !== 200) {
            reject(new Error(`API returned status ${res.statusCode}: ${data}`));
            return;
          }

          let parsed: ChatResponse;
          try {
            parsed = JSON.parse(data);
          } catch {
            reject(new Error(`Failed to parse response: ${data}`));
            return;
          }

          if (!parsed.choices || parsed.choices.length === 0) {
            reject(new Error("No choices in response"));
            return;
          }

          resolve(parsed.choices[0].message.content);
        });
      }
    );

    req.on("error", reject);
    req.write(body);
    req.end();
  });
}

async function main() {
  const args = parseArgs(process.argv.slice(2));

  const apiKey = process.env[args.apiKey];
  if (!apiKey) {
    process.stderr.write(`Error: ${args.apiKey} environment variable not set\n`);
    process.exit(1);
  }

  const chunks: Buffer[] = [];
  for await (const chunk of process.stdin) {
    chunks.push(chunk as Buffer);
  }
  const content = Buffer.concat(chunks).toString().trim();

  if (!content) {
    process.stderr.write("Error: No input provided on stdin\n");
    process.exit(1);
  }

  process.stderr.write(`base_url: ${args.baseUrl}\n`);
  process.stderr.write(`model:    ${args.model}\n`);
  process.stderr.write(`api_key:  ******** (from ${args.apiKey})\n`);
  process.stderr.write(`prompt:   ${content}\n`);

  const result = await executeQuery(args.baseUrl, args.model, apiKey, content);
  console.log(result);
}

const isMain = require.main === module;
if (isMain) {
  main().catch((err) => {
    process.stderr.write(`Error: ${err.message}\n`);
    process.exit(1);
  });
}
