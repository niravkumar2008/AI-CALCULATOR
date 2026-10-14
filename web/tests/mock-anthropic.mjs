// Mock Anthropic Messages API for the contract tests (PLAN.md section 4b).
//
//   POST /v1/messages   canned SSE stream in Anthropic streaming format
//   GET  /__last        { headers, body } of the last /v1/messages request (api key value never echoed)
//   GET  /__count       { count } of /v1/messages requests received (including rejected ones)
//   POST /__reset       zero the counter and forget the last request
//   GET  /healthz       { ok: true }
//
// Behaviour switches: a marker inside the first 256 decoded bytes of any base64 image
// in the request (the test images are a JPEG header followed by the marker):
//   FAIL500  -> HTTP 500 (Anthropic error JSON) before any streaming
//   MIDFAIL  -> send message_start, then destroy the socket
//   SLOW     -> wait MOCK_SLOW_MS (default 2000) before streaming normally
//
// Run alone: `node mock-anthropic.mjs` (port MOCK_PORT, default 8788).

import http from 'node:http';
import { pathToFileURL } from 'node:url';

export const MOCK_ANSWER = {
  readable: true,
  confidence: 0.97,
  unclear: [],
  expression: '2x+3=11',
  choice: '',
  answer: 'x = 4',
  check: '2(4)+3 = 11',
  read_as: 'Solve 2x + 3 = 11',
  steps: ['Subtract 3: 2x = 8', 'Divide by 2: x = 4'],
};
export const MOCK_INPUT_TOKENS = 1523;
export const MOCK_OUTPUT_TOKENS = 187;

const MARKERS = ['FAIL500', 'MIDFAIL', 'SLOW'];

function sse(event, data) {
  return `event: ${event}\ndata: ${JSON.stringify(data)}\n\n`;
}

function imagesOf(body) {
  const out = [];
  for (const m of Array.isArray(body?.messages) ? body.messages : []) {
    for (const c of Array.isArray(m?.content) ? m.content : []) {
      if (c?.type === 'image' && c.source?.type === 'base64' && typeof c.source.data === 'string') out.push(c.source.data);
    }
  }
  return out;
}

export function markerOf(body) {
  for (const b64 of imagesOf(body)) {
    const head = Buffer.from(b64.slice(0, 344), 'base64').toString('latin1'); // ~256 bytes
    for (const m of MARKERS) if (head.includes(m)) return m;
  }
  return null;
}

function anthropicError(res, status, type, message) {
  res.writeHead(status, { 'content-type': 'application/json', 'request-id': 'req_mock' });
  res.end(JSON.stringify({ type: 'error', error: { type, message } }));
}

export function startMock({ port = Number(process.env.MOCK_PORT || 8788), host = '127.0.0.1', slowMs = Number(process.env.MOCK_SLOW_MS || 2000), log = false } = {}) {
  const state = { count: 0, last: null };
  const sockets = new Set();

  const server = http.createServer((req, res) => {
    const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
    const chunks = [];
    req.on('data', (c) => chunks.push(c));
    req.on('end', () => {
      const raw = Buffer.concat(chunks);
      if (req.method === 'GET' && url.pathname === '/healthz') {
        res.writeHead(200, { 'content-type': 'application/json' });
        return res.end('{"ok":true}');
      }
      if (req.method === 'GET' && url.pathname === '/__count') {
        res.writeHead(200, { 'content-type': 'application/json' });
        return res.end(JSON.stringify({ count: state.count }));
      }
      if (req.method === 'GET' && url.pathname === '/__last') {
        res.writeHead(state.last ? 200 : 404, { 'content-type': 'application/json' });
        return res.end(JSON.stringify(state.last));
      }
      if (req.method === 'POST' && url.pathname === '/__reset') {
        state.count = 0;
        state.last = null;
        res.writeHead(204);
        return res.end();
      }
      if (!(req.method === 'POST' && url.pathname === '/v1/messages')) {
        return anthropicError(res, 404, 'not_found_error', `mock: no route ${req.method} ${url.pathname}`);
      }

      state.count += 1;
      let body = null;
      try {
        body = JSON.parse(raw.toString('utf8'));
      } catch {
        /* recorded as null */
      }
      state.last = {
        received_at: Date.now(),
        headers: {
          has_api_key: typeof req.headers['x-api-key'] === 'string' && req.headers['x-api-key'].length > 0,
          has_authorization: typeof req.headers.authorization === 'string',
          'anthropic-version': req.headers['anthropic-version'] ?? null,
          'anthropic-beta': req.headers['anthropic-beta'] ?? null,
          'content-type': req.headers['content-type'] ?? null,
          'x-device-id': req.headers['x-device-id'] ?? null,
        },
        body,
      };
      if (log) console.log(`[mock] POST /v1/messages #${state.count} marker=${markerOf(body)}`);

      if (!state.last.headers.has_api_key) return anthropicError(res, 401, 'authentication_error', 'x-api-key header is required');
      if (!state.last.headers['anthropic-version']) return anthropicError(res, 400, 'invalid_request_error', 'anthropic-version header is required');
      if (!body || !Array.isArray(body.messages)) return anthropicError(res, 400, 'invalid_request_error', 'messages: field required');

      const marker = markerOf(body);
      if (marker === 'FAIL500') return anthropicError(res, 500, 'api_error', 'Internal server error');

      const model = typeof body.model === 'string' ? body.model : 'unknown';
      const start = () => {
        res.writeHead(200, {
          'content-type': 'text/event-stream; charset=utf-8',
          'cache-control': 'no-cache',
          'request-id': 'req_mock',
        });
        res.write(sse('message_start', {
          type: 'message_start',
          message: {
            id: 'msg_mock_' + state.count, type: 'message', role: 'assistant', model, content: [],
            stop_reason: null, stop_sequence: null,
            usage: { input_tokens: MOCK_INPUT_TOKENS, cache_creation_input_tokens: 0, cache_read_input_tokens: 0, output_tokens: 1 },
          },
        }));
        if (marker === 'MIDFAIL') {
          setTimeout(() => res.socket?.destroy(), 50);
          return;
        }
        res.write(sse('content_block_start', { type: 'content_block_start', index: 0, content_block: { type: 'text', text: '' } }));
        res.write(sse('ping', { type: 'ping' }));
        const text = JSON.stringify(MOCK_ANSWER);
        for (let i = 0; i < text.length; i += 40) {
          res.write(sse('content_block_delta', { type: 'content_block_delta', index: 0, delta: { type: 'text_delta', text: text.slice(i, i + 40) } }));
        }
        res.write(sse('content_block_stop', { type: 'content_block_stop', index: 0 }));
        res.write(sse('message_delta', { type: 'message_delta', delta: { stop_reason: 'end_turn', stop_sequence: null }, usage: { output_tokens: MOCK_OUTPUT_TOKENS } }));
        res.write(sse('message_stop', { type: 'message_stop' }));
        res.end();
      };
      if (marker === 'SLOW') setTimeout(start, slowMs);
      else start();
    });
  });
  server.on('connection', (s) => {
    sockets.add(s);
    s.on('close', () => sockets.delete(s));
  });

  return new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(port, host, () => {
      resolve({
        url: `http://${host}:${port}`,
        state,
        close: () => new Promise((r) => {
          for (const s of sockets) s.destroy();
          server.close(() => r());
        }),
      });
    });
  });
}

if (import.meta.url === pathToFileURL(process.argv[1] || '').href) {
  const m = await startMock({ log: true });
  console.log(`mock Anthropic listening on ${m.url}`);
  const stop = async () => { await m.close(); process.exit(0); };
  process.on('SIGINT', stop);
  process.on('SIGTERM', stop);
}
