// node tools/farm/test/guard.test.cjs  - checks the farm guard blocks what it should and allows normal work.
'use strict';
const assert = require('assert');
const { decide } = require('../lib/guard.cjs');

const root = 'C:\\farm\\fields\\T-0001';
const env = { PROOFLINE_FIELD: root, USERPROFILE: 'C:\\Users\\r_kas' };
const run = (tool, input) => decide({ tool_name: tool, tool_input: input, cwd: root }, env);
const ok = (tool, input) => assert.strictEqual(run(tool, input), null, `should allow ${tool} ${JSON.stringify(input)}`);
const no = (tool, input) => assert.ok(run(tool, input), `should block ${tool} ${JSON.stringify(input)}`);

ok('Bash', { command: 'git status && git diff --stat' });
ok('Bash', { command: 'npm test --prefix calculator-core' });
ok('Bash', { command: 'grep -rn stripe proxy/ | head' });
ok('Bash', { command: 'npx wrangler dev --port 8787' });
ok('Bash', { command: 'curl https://example.com/docs' });
ok('Bash', { command: 'rm -rf node_modules build' });
ok('Bash', { command: 'rm -rf C:/farm/fields/T-0001/build' });
ok('Bash', { command: 'cd /c/farm/fields/T-0001; rm -rf build' });
ok('Bash', { command: 'echo look at 3 > notes.txt; node mail.js' });
ok('Read', { file_path: 'C:\\Users\\r_kas\\anything.txt' });
ok('Edit', { file_path: 'C:\\farm\\fields\\T-0001\\src\\a.c' });
ok('Write', { file_path: 'src/new.txt' });
ok('Write', { file_path: '.farm/REPORT.md' });

no('Bash', { command: 'git commit -m x' });
no('Bash', { command: 'cd x && git push origin main' });
no('Bash', { command: 'git reset --hard HEAD~1' });
no('Bash', { command: 'gh pr create' });
no('Bash', { command: 'stripe listen --forward-to localhost' });
no('Bash', { command: 'npx wrangler deploy' });
no('Bash', { command: 'cd site; wrangler secret put X' });
no('Bash', { command: 'curl -X POST https://x.com -d a=1' });
no('Bash', { command: 'ssh pi@raspberrypi' });
no('PowerShell', { command: 'Invoke-RestMethod https://x -Method Post -Body $b' });
no('PowerShell', { command: 'Send-MailMessage -To a@b.c' });
no('Bash', { command: 'rm -rf C:/Users/r_kas/Documents' });
no('Bash', { command: 'rm -rf ~/.claude' });
no('Bash', { command: 'rm -rf /c/Users/r_kas/Documents' });
no('Bash', { command: 'claude -p "do more"' });
no('PowerShell', { command: 'Restart-NetAdapter -Name Wi-Fi' });
no('Write', { file_path: 'C:\\Users\\r_kas\\.claude\\settings.json' });
no('Edit', { file_path: 'C:\\farm\\fields\\T-0001\\.git\\config' });
no('Write', { file_path: '..\\T-0002\\x.txt' });
no('mcp__gmail__send_message', {});

console.log('guard: all checks passed');
