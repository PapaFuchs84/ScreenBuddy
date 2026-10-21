// Screenshots der laufenden Einstellungsseite per Edge (headless, CDP). Klickt nur Reiter, speichert nichts.
import { spawn } from 'node:child_process';
import { writeFileSync, mkdirSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const OUT = process.argv[2];
mkdirSync(OUT, { recursive: true });
const edge = 'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe';
const prof = join(tmpdir(), 'fl-shot-profile');
const p = spawn(edge, ['--headless=new', '--remote-debugging-port=9333', '--user-data-dir=' + prof,
  '--window-size=1280,860', '--hide-scrollbars', 'about:blank'], { stdio: 'ignore' });
const sleep = ms => new Promise(r => setTimeout(r, ms));
let targets;
for (let i = 0; i < 40; i++) { try { targets = await (await fetch('http://127.0.0.1:9333/json')).json(); break; } catch { await sleep(250); } }
const t = targets.find(x => x.type === 'page');
const ws = new WebSocket(t.webSocketDebuggerUrl);
await new Promise(r => ws.onopen = r);
let id = 0; const pend = {};
ws.onmessage = e => { const m = JSON.parse(e.data); if (m.id && pend[m.id]) { pend[m.id](m); delete pend[m.id]; } };
const cmd = (method, params = {}) => new Promise(r => { const i = ++id; pend[i] = r; ws.send(JSON.stringify({ id: i, method, params })); });
const js = async expr => (await cmd('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true })).result?.result?.value;

// Optional: neue webui.html aus dem Quellordner zeigen, bevor die EXE neu gebaut ist (Daten kommen weiter von der App)
const LOCAL = process.argv[3];
if (LOCAL) {
  const { readFileSync } = await import('node:fs');
  const body = Buffer.from(readFileSync(LOCAL)).toString('base64');
  await cmd('Fetch.enable', { patterns: [{ urlPattern: 'http://127.0.0.1:8765/' }] });
  ws.addEventListener('message', e => { const m = JSON.parse(e.data);
    if (m.method === 'Fetch.requestPaused') cmd('Fetch.fulfillRequest', { requestId: m.params.requestId, responseCode: 200,
      responseHeaders: [{ name: 'Content-Type', value: 'text/html; charset=utf-8' }], body }); });
}

// Echte Namen schon in den Daten ersetzen, bevor die Seite sie anzeigt. Wichtig, weil sich manche Anzeigen (Verbindungsstatus)
// alle paar Sekunden selbst neu laden und eine nachträgliche Ersetzung im Bild sonst wieder überschreiben.
await cmd('Page.addScriptToEvaluateOnNewDocument', { source: `(() => {
  const EX = ['Lena','Max','Tom','Mia','Ben','Emma','Paul','Lia','Finn','Ida','Noah','Ella','Leon','Lea'];
  const nm = i => EX[i % EX.length];
  const clean = (url, d) => {
    if (url.includes('/api/connections')) for (const k in d) if (d[k] && d[k].name) d[k].name = 'FuchsKumpel';
    if (url.includes('/api/discord/members')) (d.members || []).forEach((m, i) => { m.name = nm(i); m.guild = 'Mein Server'; });
    if (url.includes('/api/friends/accounts')) { (d.accounts || []).forEach((a, i) => { a.name = nm(i); }); d.suggestions = []; }
    if (url.includes('/api/config') && d.config) {
      const c = d.config;
      if (c.bf6) c.bf6.players = ['FuchsKumpel', 'Lena_Sniper', 'MaxPower99', 'TomTheTank', 'Mia2015'];
      if (c.rss) c.rss.feeds = ['https://winfuture.de/', 'https://www.heise.de/', 'https://www.golem.de/', 'https://www.computerbase.de/'];
      if (c.friends) { if (c.friends.steam_id) c.friends.steam_id = 'https://steamcommunity.com/id/meinname';
        c.friends.links = (c.friends.links || []).map((l, i) => [nm(i), ...l.split('|').slice(1)].join('|')); }
      if (c.discord) { c.discord.friend_ids = []; c.discord.ignore_ids = []; c.discord.guild_ids = []; }
    }
    return d;
  };
  const f = window.fetch;
  window.fetch = async (input, init) => {
    const r = await f(input, init), url = String(input);
    if (!url.includes('/api/') || !(r.headers.get('content-type') || '').includes('json')) return r;
    return new Response(JSON.stringify(clean(url, await r.json())), { status: r.status, headers: { 'Content-Type': 'application/json' } });
  };
})();` });

await cmd('Emulation.setDeviceMetricsOverride', { width: 1280, height: 860, deviceScaleFactor: 1, mobile: false });
await cmd('Page.navigate', { url: 'http://127.0.0.1:8765/' });
await sleep(2500);

// Private Namen durch Beispiele ersetzen, nichts an der App ändern
const anon = `(() => {
  const ex = ['Lena','Max','Tom','Mia','Ben','Emma','Paul','Lia','Finn','Ida','Noah','Ella','Leon','Lea'];
  document.querySelectorAll('.member span:not(.note)').forEach((s,i) => s.textContent = ex[i % ex.length]);
  document.querySelectorAll('.member small').forEach(s => s.textContent = 'Mein Server');
  document.querySelectorAll('main input').forEach(e => { if (/P4p4F0x/.test(e.placeholder)) e.placeholder = e.placeholder.replace('P4p4F0x', 'FuchsKumpel'); });
  const h1 = document.querySelector('h1')?.textContent;
  const items = [...document.querySelectorAll('.list .item')];
  if (h1 === 'Battlefield 6') {
    const pl = [['FuchsKumpel', 'Rang 42 · Sergeant · K/D 1.12'], ['Lena_Sniper', 'Rang 37 · K/D 0.94'], ['MaxPower99', null], ['TomTheTank', null], ['Mia2015', null]];
    items.forEach((it, i) => { const [n, s] = pl[i % pl.length]; it.querySelector('b').textContent = n;
      const sp = it.querySelector('.t span'); if (s) { sp.textContent = s; sp.style.color = '#58d68d'; } });
  }
  if (h1 === 'Blogs') {
    const bl = ['https://winfuture.de/', 'https://www.heise.de/', 'https://www.golem.de/', 'https://www.computerbase.de/'];
    items.forEach((it, i) => { it.querySelector('b').textContent = bl[i % bl.length];
      if (i === 0) { const sp = it.querySelector('.t span'); sp.textContent = 'WinFuture: Neuer Beitrag gefunden'; sp.style.color = '#58d68d'; } });
  }
  if (h1 === 'Freunde') {
    const fr = ['Lena', 'Max', 'Tom', 'Mia'];
    items.forEach((it, i) => it.querySelector('b').textContent = fr[i % fr.length]);
    document.querySelectorAll('option').forEach((o, i) => o.textContent = o.textContent.replace(/^.*\\(/, fr[i % fr.length] + ' ('));

    // Auch private Xbox-Statusnamen und Prüftexte durch neutrale Beispieldaten ersetzen.
    document.querySelectorAll('.conn .t').forEach(e => e.textContent = 'Verbunden als FuchsKumpel · Beispielkonto · live');
    document.querySelectorAll('main input').forEach(e => { if (e.type === 'password') e.value = '';
      else if (e.type === 'text' && e.value) e.value = /steamcommunity/.test(e.placeholder) ? 'https://steamcommunity.com/id/meinname' : 'Beispielname'; });
    document.querySelectorAll('.note, .ok, .bad').forEach(e => { if (/Verbunden:|Prüfe|Fehler|Gamertag|Xbox/i.test(e.textContent)) e.textContent = 'Verbunden: 4 Freunde, 2 gerade online'; });
  }
  return true; })()`;

const tabs = await js(`[...document.querySelectorAll('#nav button')].map(b => b.textContent)`);
console.log('Tabs:', tabs);
async function shot(name, clip) {
  const r = await cmd('Page.captureScreenshot', { format: 'png', ...(clip ? { clip: { ...clip, scale: 1 } } : {}) });
  writeFileSync(join(OUT, name + '.png'), Buffer.from(r.result.data, 'base64'));
  console.log('saved', name);
}
const go = async label => { await js(`[...document.querySelectorAll('#nav button')].find(b => b.textContent.startsWith(${JSON.stringify(label)}))?.click()`); await sleep(1800); await js(anon); await sleep(200); };

// [Reiter, Datei, Höhe, welche Klapp-Hilfen offen sind]
// (Übersicht nicht hier: die Live-Vorschau zeigt echte Daten; dafür unten die prev-Aufnahmen und eine neutrale aussuchen)
const VIEWS = [['Start', 'start', 860], ['Discord', 'discord', 1180, [0]],
  ['Battlefield', 'bf6', 1180, [0]], ['Blogs', 'blogs', 1180, [0]], ['System', 'system', 860], ['Display', 'display', 860],
  ['Freunde', 'freunde', 1900, [0, 1]]];
// Lage wichtiger Bedienelemente ausgeben, damit die Markierungen in der Anleitung stimmen
const RECTS = `[...document.querySelectorAll('main input:not([type=checkbox]), main .howto, main .members, main .add, main .sw, main .btn, main select, main h3')]
  .map(e => { const r = e.getBoundingClientRect(); return (e.tagName + '.' + e.className + ' ' + (e.placeholder || e.textContent || '').slice(0, 30)).padEnd(52) +
  [r.x, r.y, r.width, r.height].map(Math.round).join(', '); }).join('\\n')`;
for (const [label, file, height, open = []] of VIEWS) {
  if (!tabs.some(t => t.startsWith(label))) continue;
  await cmd('Emulation.setDeviceMetricsOverride', { width: 1280, height, deviceScaleFactor: 1, mobile: false });
  await go(label);
  await js(`document.querySelectorAll('main .howto').forEach((d, i) => d.open = ${JSON.stringify(open)}.includes(i))`);
  await sleep(1500); await js(anon); await sleep(200);  // Listen laden teils verzögert nach
  await shot(file);
  console.log('--- ' + file + '\n' + await js(RECTS));
}
await cmd('Emulation.setDeviceMetricsOverride', { width: 1280, height: 860, deviceScaleFactor: 1, mobile: false });
// Speichern-Leiste zeigen (nur Anzeige, ohne Änderung)
await go('Übersicht');
await js(`document.getElementById('bar').classList.add('show')`); await sleep(400);
await shot('speichern');
await js(`document.getElementById('bar').classList.remove('show')`);
// Vorschau zeigt reihum die Seiten: mehrere Aufnahmen, die neutrale wird ausgesucht
if (!process.env.NO_PREV) for (let i = 0; i < 7; i++) { await shot('prev' + i); await sleep(12000); }
ws.close(); p.kill();
