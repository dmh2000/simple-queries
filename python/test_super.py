import json
import unittest
from http.server import HTTPServer, BaseHTTPRequestHandler
from threading import Thread

from super_cli import execute_query, parse_args


class FakeHandler(BaseHTTPRequestHandler):
    """Test handler that stores the last request and returns a configurable response."""

    response_body = None
    response_status = 200
    last_request_body = None
    last_request_headers = None
    last_request_path = None

    def do_POST(self):
        FakeHandler.last_request_path = self.path
        FakeHandler.last_request_headers = dict(self.headers)
        length = int(self.headers.get("Content-Length", 0))
        FakeHandler.last_request_body = json.loads(self.rfile.read(length))

        self.send_response(FakeHandler.response_status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        if FakeHandler.response_body is not None:
            self.wfile.write(json.dumps(FakeHandler.response_body).encode())

    def log_message(self, format, *args):
        pass  # suppress log output during tests


def start_server():
    server = HTTPServer(("127.0.0.1", 0), FakeHandler)
    thread = Thread(target=server.serve_forever)
    thread.daemon = True
    thread.start()
    return server


class TestExecuteQuery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = start_server()
        cls.base_url = f"http://127.0.0.1:{cls.server.server_address[1]}"

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()

    def setUp(self):
        FakeHandler.response_status = 200
        FakeHandler.response_body = None
        FakeHandler.last_request_body = None
        FakeHandler.last_request_headers = None
        FakeHandler.last_request_path = None

    def test_success(self):
        FakeHandler.response_body = {
            "choices": [{"message": {"role": "assistant", "content": "Hi there!"}}]
        }
        result = execute_query(self.base_url, "test-model", "test-key", "hello")

        self.assertEqual(result, "Hi there!")
        self.assertEqual(FakeHandler.last_request_path, "/chat/completions")
        self.assertEqual(FakeHandler.last_request_body["model"], "test-model")
        self.assertEqual(len(FakeHandler.last_request_body["messages"]), 1)
        self.assertEqual(FakeHandler.last_request_body["messages"][0]["role"], "user")
        self.assertEqual(FakeHandler.last_request_body["messages"][0]["content"], "hello")
        self.assertIn("Bearer test-key", FakeHandler.last_request_headers.get("Authorization", ""))

    def test_api_error(self):
        FakeHandler.response_status = 401
        FakeHandler.response_body = {"error": "invalid api key"}

        with self.assertRaises(Exception) as ctx:
            execute_query(self.base_url, "test-model", "bad-key", "hello")
        self.assertIn("401", str(ctx.exception))

    def test_empty_choices(self):
        FakeHandler.response_body = {"choices": []}

        with self.assertRaises(Exception) as ctx:
            execute_query(self.base_url, "test-model", "test-key", "hello")
        self.assertIn("no choices", str(ctx.exception).lower())

    def test_invalid_json(self):
        FakeHandler.response_body = None  # handler won't write body

        with self.assertRaises(Exception):
            execute_query(self.base_url, "test-model", "test-key", "hello")


class TestParseArgs(unittest.TestCase):
    def test_all_flags(self):
        args = parse_args(["--base-url", "http://localhost", "--model", "gpt-4", "--api-key", "MY_KEY"])
        self.assertEqual(args.base_url, "http://localhost")
        self.assertEqual(args.model, "gpt-4")
        self.assertEqual(args.api_key, "MY_KEY")

    def test_missing_required(self):
        with self.assertRaises(SystemExit):
            parse_args(["--base-url", "http://localhost"])


if __name__ == "__main__":
    unittest.main()
