// Validates live responses against docs/web/openapi.yaml (THE contract; never edited here).
//
//   import { assertMatches } from './contract.mjs';
//   assertMatches('POST', '/v1/device/register', res);   // res from lib.mjs api()
//
// Checks: the status code is documented for that operation (undocumented -> failure),
// the content type matches the documented media type, the JSON body validates against
// the documented schema ($refs resolved, oneOf handled by ajv), documented response
// headers are present, and bodiless responses (e.g. 204) are empty.
//
// Contract path: env CONTRACT_PATH, else ../../docs/web/openapi.yaml relative to this file.

import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
import YAML from 'yaml';
import Ajv2020 from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';

const here = path.dirname(fileURLToPath(import.meta.url));
export const CONTRACT_PATH = process.env.CONTRACT_PATH || path.resolve(here, '../../docs/web/openapi.yaml');

if (!fs.existsSync(CONTRACT_PATH)) {
  throw new Error(`Contract not found at ${CONTRACT_PATH}. Set CONTRACT_PATH or copy docs/web/openapi.yaml into the repo.`);
}

const DOC_ID = 'openapi.json';
export const doc = YAML.parse(fs.readFileSync(CONTRACT_PATH, 'utf8').replace(/^﻿/, ''));

const ajv = new Ajv2020({ strict: false, allErrors: true, validateFormats: true });
addFormats(ajv);
ajv.addFormat('binary', true);
ajv.addSchema({ ...doc, $id: DOC_ID });

const METHODS = ['get', 'post', 'put', 'patch', 'delete'];

/**
 * Known contract gaps: [method, pathTemplate, status] combinations the tests expect even
 * though openapi.yaml does not document them yet. They are reported (console.warn) instead
 * of failing, so the suite stays meaningful while the lead decides. Each entry is a contract
 * change proposal in README.md. Remove an entry as soon as the contract documents it.
 */
export const KNOWN_GAPS = [
  // PATCH /api/devices/{id} validates `label` (Label pattern) but documents no 400.
  ['patch', '/api/devices/{device_id}', '400'],
];

const esc = (s) => s.replace(/~/g, '~0').replace(/\//g, '~1');
const unesc = (s) => s.replace(/~1/g, '/').replace(/~0/g, '~');

function getPtr(pointer) {
  let node = doc;
  for (const part of pointer.replace(/^#\/?/, '').split('/').filter(Boolean)) node = node?.[unesc(part)];
  return node;
}

/** Follow `$ref`s to components; returns { node, ptr }. */
function deref(node, ptr) {
  let guard = 0;
  while (node && typeof node.$ref === 'string' && guard++ < 10) {
    ptr = node.$ref;
    node = getPtr(ptr);
  }
  return { node, ptr };
}

/** Map a concrete path ('/api/devices/dev_..') to its template ('/api/devices/{device_id}'). */
export function templateFor(actualPath) {
  const p = actualPath.split('?')[0];
  if (doc.paths[p]) return p;
  for (const tpl of Object.keys(doc.paths)) {
    const re = new RegExp('^' + tpl.replace(/[.*+?^$()|[\]\\]/g, '\\$&').replace(/\\?\{[^}]+\\?\}/g, '[^/]+') + '$');
    if (re.test(p)) return tpl;
  }
  return null;
}

const validators = new Map(); // ptr -> compiled validator

function validatorFor(schemaPtr) {
  if (!validators.has(schemaPtr)) validators.set(schemaPtr, ajv.compile({ $ref: DOC_ID + schemaPtr }));
  return validators.get(schemaPtr);
}

/** Resolved response object for (method, template, status) or null. */
export function responseSpec(method, tpl, status) {
  const op = doc.paths[tpl]?.[method.toLowerCase()];
  if (!op) return null;
  const code = String(status);
  const raw = op.responses?.[code] ?? op.responses?.[code[0] + 'XX'] ?? op.responses?.default;
  if (!raw) return null;
  const basePtr = `#/paths/${esc(tpl)}/${method.toLowerCase()}/responses/${esc(op.responses[code] ? code : code[0] + 'XX')}`;
  return deref(raw, basePtr);
}

/** Compile every documented response schema (run once to prove the contract is usable). */
export function compileAll() {
  let n = 0;
  for (const [tpl, item] of Object.entries(doc.paths)) {
    for (const m of METHODS) {
      const op = item[m];
      if (!op) continue;
      for (const code of Object.keys(op.responses || {})) {
        const { node, ptr } = responseSpec(m, tpl, code);
        for (const media of Object.keys(node?.content || {})) {
          if (node.content[media].schema) {
            validatorFor(`${ptr}/content/${esc(media)}/schema`);
            n++;
          }
        }
      }
      if (op.requestBody) {
        const { node, ptr } = deref(op.requestBody, `#/paths/${esc(tpl)}/${m}/requestBody`);
        for (const media of Object.keys(node?.content || {})) {
          if (node.content[media].schema) {
            validatorFor(`${ptr}/content/${esc(media)}/schema`);
            n++;
          }
        }
      }
    }
  }
  return n;
}

/** Validate an arbitrary value against a named component schema, e.g. 'Error'. */
export function assertSchema(name, value, label = name) {
  const v = validatorFor(`#/components/schemas/${esc(name)}`);
  assert.ok(v(value), `${label} does not match components.schemas.${name}: ${ajv.errorsText(v.errors)}\n${JSON.stringify(value).slice(0, 500)}`);
}

/** Validate a request body we are about to send (catches broken test fixtures). */
export function assertRequestValid(method, tpl, body) {
  const op = doc.paths[tpl]?.[method.toLowerCase()];
  assert.ok(op?.requestBody, `no requestBody documented for ${method} ${tpl}`);
  const { ptr } = deref(op.requestBody, `#/paths/${esc(tpl)}/${method.toLowerCase()}/requestBody`);
  const v = validatorFor(`${ptr}/content/application~1json/schema`);
  assert.ok(v(body), `request fixture invalid for ${method} ${tpl}: ${ajv.errorsText(v.errors)}`);
}

/**
 * res: { status, headers (Headers), text, json, bytes? } as produced by lib.mjs api().
 * Returns the parsed JSON body (or null).
 */
export function assertMatches(method, actualPath, res) {
  const tpl = templateFor(actualPath);
  assert.ok(tpl, `path ${actualPath} is not in the contract`);
  const label = `${method.toUpperCase()} ${tpl} -> ${res.status}`;
  const found = responseSpec(method, tpl, res.status);
  if (!found || !found.node) {
    const gap = KNOWN_GAPS.find(([m, p, s]) => m === method.toLowerCase() && p === tpl && s === String(res.status));
    if (gap) {
      console.warn(`[contract gap] ${label} is not documented in openapi.yaml (see README "Contract change proposals")`);
      if (res.json) assertSchema('Error', res.json, label);
      return res.json ?? null;
    }
    const documented = Object.keys(doc.paths[tpl]?.[method.toLowerCase()]?.responses || {}).join(', ');
    assert.fail(`${label}: status not documented (documented: ${documented}). Body: ${String(res.text ?? '').slice(0, 300)}`);
  }
  const { node, ptr } = found;

  // Headers documented on this response must be present (none are marked optional in the contract).
  for (const h of Object.keys(node.headers || {})) {
    const value = h.toLowerCase() === 'set-cookie'
      ? (res.headers.getSetCookie?.() || []).join('\n') || res.headers.get('set-cookie')
      : res.headers.get(h);
    assert.ok(value, `${label}: documented header ${h} missing`);
    const hs = deref(node.headers[h], ptr).node?.schema;
    if (hs?.type === 'integer') assert.match(value, /^\d+$/, `${label}: header ${h} must be an integer, got ${value}`);
  }

  const content = node.content || {};
  const medias = Object.keys(content);
  const ct = (res.headers.get('content-type') || '').split(';')[0].trim().toLowerCase();
  if (medias.length === 0) {
    assert.ok(!res.text || res.text.length === 0, `${label}: documented without a body but got: ${String(res.text).slice(0, 200)}`);
    return null;
  }
  assert.ok(medias.includes(ct), `${label}: content-type "${ct}" not in documented [${medias.join(', ')}]`);
  if (ct === 'application/json') {
    let body = res.json;
    if (body === undefined || body === null) {
      try {
        body = JSON.parse(res.text);
      } catch {
        assert.fail(`${label}: body is not JSON: ${String(res.text).slice(0, 200)}`);
      }
    }
    const v = validatorFor(`${ptr}/content/application~1json/schema`);
    assert.ok(v(body), `${label}: body does not match schema: ${ajv.errorsText(v.errors)}\n${JSON.stringify(body).slice(0, 500)}`);
    return body;
  }
  return null;
}
