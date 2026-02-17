import { describe, it, before, after, beforeEach } from "node:test";
import assert from "node:assert/strict";
import http from "node:http";
import { executeQuery, parseArgs } from "./main.js";

interface RequestRecord {
  method: string;
  url: string;
  headers: http.IncomingHttpHeaders;
  body: Record<string, unknown>;
}

let server: http.Server;
let baseUrl: string;
let responseBody: unknown = null;
let responseStatus = 200;
let lastRequest: RequestRecord | null = null;

before(async () => {
  server = http.createServer((req, res) => {
    let data = "";
    req.on("data", (chunk) => (data += chunk));
    req.on("end", () => {
      lastRequest = {
        method: req.method!,
        url: req.url!,
        headers: req.headers,
        body: data ? JSON.parse(data) : null,
      };
      res.writeHead(responseStatus, { "Content-Type": "application/json" });
      if (responseBody !== null) {
        res.end(JSON.stringify(responseBody));
      } else {
        res.end("");
      }
    });
  });

  await new Promise<void>((resolve) => {
    server.listen(0, "127.0.0.1", resolve);
  });
  const addr = server.address() as { port: number };
  baseUrl = `http://127.0.0.1:${addr.port}`;
});

after(() => {
  server.close();
});

beforeEach(() => {
  responseStatus = 200;
  responseBody = null;
  lastRequest = null;
});

describe("executeQuery", () => {
  it("sends correct request and returns response content", async () => {
    responseBody = {
      choices: [{ message: { role: "assistant", content: "Hi there!" } }],
    };

    const result = await executeQuery(baseUrl, "test-model", "test-key", "hello");

    assert.equal(result, "Hi there!");
    assert.equal(lastRequest!.method, "POST");
    assert.equal(lastRequest!.url, "/chat/completions");
    assert.equal(lastRequest!.headers["authorization"], "Bearer test-key");
    assert.equal(lastRequest!.headers["content-type"], "application/json");
    assert.equal(lastRequest!.body.model, "test-model");
    assert.equal((lastRequest!.body.messages as Array<{ role: string; content: string }>)[0].role, "user");
    assert.equal((lastRequest!.body.messages as Array<{ role: string; content: string }>)[0].content, "hello");
  });

  it("throws on API error status", async () => {
    responseStatus = 401;
    responseBody = { error: "invalid api key" };

    await assert.rejects(
      () => executeQuery(baseUrl, "test-model", "bad-key", "hello"),
      (err: Error) => err.message.includes("401")
    );
  });

  it("throws on empty choices", async () => {
    responseBody = { choices: [] };

    await assert.rejects(
      () => executeQuery(baseUrl, "test-model", "test-key", "hello"),
      (err: Error) => err.message.toLowerCase().includes("no choices")
    );
  });

  it("throws on invalid JSON response", async () => {
    responseBody = null; // empty response body

    await assert.rejects(() =>
      executeQuery(baseUrl, "test-model", "test-key", "hello")
    );
  });
});

describe("parseArgs", () => {
  it("parses all flags", () => {
    const args = parseArgs(["--base-url", "http://localhost", "--model", "gpt-4", "--api-key", "MY_KEY"]);
    assert.equal(args.baseUrl, "http://localhost");
    assert.equal(args.model, "gpt-4");
    assert.equal(args.apiKey, "MY_KEY");
  });

  it("throws on missing required flags", () => {
    assert.throws(() => parseArgs(["--base-url", "http://localhost"]));
  });
});
