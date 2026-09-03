/* Orderkoll – en enda gemensam kö för allt bakgrundsarbete (arbetsorder 5.7).
 *
 * Regler som koden garanterar:
 *  - Jobben körs sekventiellt, aldrig parallellt.
 *  - Mellan två jobb som rör Amazon läggs en slumpad paus (default 3–9 s).
 *  - Per-nyckel-minimiintervall (t.ex. lagerbevakning max 1 gång/timme).
 *  - CAPTCHA/inloggningsvägg upptäcks och stoppar kön i stället för att
 *    hamras vidare. */
(function (root) {
  'use strict';
  const AOM = (root.AOM = root.AOM || {});

  function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }

  function randomBetween(min, max) {
    return Math.floor(min + Math.random() * Math.max(0, max - min));
  }

  class SyncQueue {
    constructor(options = {}) {
      this.minDelayMs = options.minDelayMs ?? 3000;
      this.maxDelayMs = options.maxDelayMs ?? 9000;
      this.onEvent = options.onEvent || (() => {});
      this.queue = [];
      this.running = false;
      this.paused = false;
      this.pauseReason = null;
      this.lastNetworkAt = 0;
      this.lastRunByKey = new Map();
      this.log = [];
    }

    size() {
      return this.queue.length;
    }

    /** job: { type, key, network=true, minIntervalMs, run() } */
    enqueue(job) {
      if (!job || typeof job.run !== 'function') throw new Error('Ogiltigt jobb');
      if (job.key && this.queue.some((j) => j.key === job.key)) {
        this._trace('skip_duplicate', job);
        return false;
      }
      if (job.key && job.minIntervalMs) {
        const last = this.lastRunByKey.get(job.key) || 0;
        if (Date.now() - last < job.minIntervalMs) {
          this._trace('skip_rate_limited', job);
          return false;
        }
      }
      this.queue.push(Object.assign({ network: true, enqueuedAt: Date.now() }, job));
      this._trace('enqueued', job);
      this._drain();
      return true;
    }

    pause(reason) {
      this.paused = true;
      this.pauseReason = reason || 'okänd';
      this.onEvent({ type: 'paused', reason: this.pauseReason });
    }

    resume() {
      this.paused = false;
      this.pauseReason = null;
      this.onEvent({ type: 'resumed' });
      this._drain();
    }

    clear() {
      this.queue = [];
    }

    _trace(event, job) {
      const entry = {
        at: new Date().toISOString(),
        event,
        type: job && job.type,
        key: job && job.key,
      };
      this.log.push(entry);
      if (this.log.length > 200) this.log.shift();
      // Synlig i chrome://extensions -> service worker-loggen (grind steg 8).
      console.debug('[Orderkoll][kö]', entry.event, entry.type || '', entry.key || '');
    }

    async _drain() {
      if (this.running) return;
      this.running = true;
      try {
        while (this.queue.length && !this.paused) {
          const job = this.queue.shift();
          if (job.network) {
            const since = Date.now() - this.lastNetworkAt;
            const wait = randomBetween(this.minDelayMs, this.maxDelayMs) - since;
            if (this.lastNetworkAt && wait > 0) {
              this._trace(`waiting_${wait}ms`, job);
              await sleep(wait);
            }
          }
          this._trace('start', job);
          try {
            const result = await job.run();
            if (job.key) this.lastRunByKey.set(job.key, Date.now());
            if (job.network) this.lastNetworkAt = Date.now();
            this._trace('done', job);
            this.onEvent({ type: 'job_done', job, result });
          } catch (err) {
            if (job.network) this.lastNetworkAt = Date.now();
            this._trace('error', job);
            this.onEvent({ type: 'job_error', job, error: String(err && err.message ? err.message : err) });
            if (err && err.wall) this.pause(err.wall);
          }
        }
      } finally {
        this.running = false;
      }
    }
  }

  /**
   * Hämtar en URL med extensionens värdbehörigheter. Kastar ett fel märkt med
   * .wall när Amazon svarar med CAPTCHA eller inloggningsvägg, så att kön
   * pausar i stället för att fortsätta.
   */
  async function fetchText(url, options = {}) {
    const res = await fetch(url, {
      credentials: 'include',
      redirect: 'follow',
      headers: Object.assign({ 'Accept-Language': 'sv-SE,sv;q=0.9' }, options.headers || {}),
    });
    const body = await res.text();
    const wall = AOM.parse.detectWall(res.url) || AOM.parse.detectWall(body.slice(0, 8000));
    if (wall) {
      const err = new Error(`Amazon svarade med ${wall}`);
      err.wall = wall;
      throw err;
    }
    if (!res.ok) {
      const err = new Error(`HTTP ${res.status} för ${url}`);
      err.status = res.status;
      throw err;
    }
    return { url: res.url, status: res.status, text: body };
  }

  AOM.SyncQueue = SyncQueue;
  AOM.queueUtils = { sleep, randomBetween, fetchText };
})(typeof self !== 'undefined' ? self : globalThis);
