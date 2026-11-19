// Farm usage reporter: a tiny mod loaded into every farm run (`--plugin-dir`).
// Whenever the plan's 5-hour or weekly "% used" moves, it writes the real numbers to
// <PROOFLINE_FARM_DIR>/usage.json, so the farm manager can keep your weekly reserve.
import type { Register } from 'claude-code'

export const register: Register = on => {
  on('session.measure', async ($, e, next) => {
    const out = await next(e)
    if (!e.rateLimits?.length) return out
    const dir = await $.env.get('PROOFLINE_FARM_DIR')
    if (!dir) return out
    const body = {
      at: await $.clock.now(),
      source: 'farm-run',
      task: (await $.env.get('PROOFLINE_TASK_ID')) ?? null,
      rateLimits: e.rateLimits,
    }
    try { await $.fs.write(`${dir}/usage.json`, JSON.stringify(body, null, 2)) } catch { /* best effort */ }
    return out
  })
}
