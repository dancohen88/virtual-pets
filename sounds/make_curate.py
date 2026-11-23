import json, sys, html
manifest = json.load(open(sys.argv[1], encoding="utf-8"))
page = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>__TITLE__</title>
<style>
:root{--bg:#faf6f1;--card:#fff;--ink:#2b211b;--muted:#7a6a5e;--line:#e8ddd2;--accent:#c7743a;--saus:#b5652f;--husk:#3d6f9e;--both:#6b8e4e;--rej:#a33}
@media (prefers-color-scheme:dark){:root{--bg:#1c1814;--card:#26211c;--ink:#f1e9e1;--muted:#a8998c;--line:#3a322b;--accent:#e0955c;--saus:#e0955c;--husk:#7fb0dd;--both:#9cc27a;--rej:#e07070}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.45 system-ui,-apple-system,sans-serif}
header{position:sticky;top:0;z-index:2;background:var(--bg);border-bottom:1px solid var(--line);padding:12px 16px;display:flex;flex-wrap:wrap;gap:8px 16px;align-items:center}
h1{font-size:18px;margin:0;flex:1 1 auto}.stats{color:var(--muted);font-size:13px}
button{font:inherit;border:1px solid var(--line);background:var(--card);color:var(--ink);border-radius:8px;padding:5px 10px;cursor:pointer}
button:hover{border-color:var(--accent)}button.on{color:#fff;border-color:transparent}
button.on[data-v=sausage]{background:var(--saus)}button.on[data-v=husky]{background:var(--husk)}button.on[data-v=both]{background:var(--both)}button.on[data-v=reject]{background:var(--rej)}
main{max-width:980px;margin:0 auto;padding:16px}h2{font-size:15px;text-transform:uppercase;letter-spacing:.06em;color:var(--muted);margin:24px 0 8px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px 14px;margin:0 0 10px}
.card.reject{opacity:.5}.top{display:flex;gap:8px;align-items:baseline;flex-wrap:wrap}.title{font-weight:600}
.meta{color:var(--muted);font-size:13px}.meta a{color:inherit}audio{width:100%;margin:8px 0 6px;height:36px}
.row{display:flex;flex-wrap:wrap;gap:6px;align-items:center;margin-top:4px}.row label{font-size:13px;color:var(--muted)}
input[type=text]{font:inherit;border:1px solid var(--line);background:var(--bg);color:var(--ink);border-radius:8px;padding:4px 8px;min-width:0;flex:1 1 160px}
.trim{font-variant-numeric:tabular-nums;font-size:13px}.hint{color:var(--muted);font-size:13px;margin:0 0 12px}
</style></head><body>
<header><h1>__TITLE__</h1><span class="stats" id="stats"></span>
<button id="copy">Copy results</button><button id="dl">Download curation.json</button></header>
<main>
<p class="hint">For each clip: pick a pet (or reject), adjust the role, and if only part is useful, play it and press <b>Mark start</b> / <b>Mark end</b> at the right moments.
<b>▲ Baby pitch</b> previews it sped up and pitched up, like the baby stage would sound. Choices save in this browser; when done, <b>Copy results</b> and paste them to Claude.</p>
<div id="list"></div></main>
<script>
const CLIPS = __MANIFEST__;
const KEY = "__KEY__";
let state = {}; try { state = JSON.parse(localStorage.getItem(KEY) || "{}"); } catch (e) {}
const save = () => { try { localStorage.setItem(KEY, JSON.stringify(state)); } catch (e) {} stats(); };
const esc = s => String(s).replace(/[&<>"]/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
const fmt = t => t == null ? "–" : t.toFixed(2) + "s";
const groups = [["sausage","Suggested for the sausage dog"],["husky","Suggested for the husky"],["both","Shared / everyday sounds"]];
const list = document.getElementById("list");
for (const [g, label] of groups) {
  const h = document.createElement("h2"); h.textContent = label; list.appendChild(h);
  for (const c of CLIPS.filter(c => c.pet === g)) {
    const s = state[c.key] ||= { verdict: null, role: c.role, start: null, end: null, notes: "" };
    const el = document.createElement("div"); el.className = "card";
    el.innerHTML = `<div class="top"><span class="title">${esc(c.title)}</span><span class="meta">${esc(c.length)} · suggested: ${esc(c.role)}</span></div>
      <div class="meta">${esc(c.desc)} — <a href="${esc(c.page)}" target="_blank" rel="noopener">${esc(c.source)}</a> · ${esc(c.licence)}</div>
      <audio controls preload="none" src="__BASE__${esc(c.file)}"></audio>
      <div class="row">${["sausage","husky","both","reject"].map(v => `<button data-v="${v}">${{sausage:"Sausage dog",husky:"Husky",both:"Both",reject:"✗ Reject"}[v]}</button>`).join("")}
        <button class="pitch">▲ Baby pitch</button></div>
      <div class="row"><label>Role</label><input type="text" class="role"><label>Notes</label><input type="text" class="notes"></div>
      <div class="row trim"><button class="ms">Mark start</button><button class="me">Mark end</button><span class="tv"></span><button class="pl">▶ Play selection</button><button class="cl">Clear</button></div>`;
    const audio = el.querySelector("audio"), tv = el.querySelector(".tv");
    const paint = () => {
      el.querySelectorAll("[data-v]").forEach(b => b.classList.toggle("on", b.dataset.v === s.verdict));
      el.classList.toggle("reject", s.verdict === "reject");
      const bad = s.start != null && s.end != null && s.end < s.start;
      tv.textContent = `selection ${fmt(s.start)} → ${fmt(s.end)}` + (bad ? "  ⚠ end is before start: re-mark" : "");
      tv.style.color = bad ? "var(--rej)" : "";
    };
    el.querySelectorAll("[data-v]").forEach(b => b.onclick = () => { s.verdict = s.verdict === b.dataset.v ? null : b.dataset.v; paint(); save(); });
    const role = el.querySelector(".role"), notes = el.querySelector(".notes");
    role.value = s.role; notes.value = s.notes;
    role.oninput = () => { s.role = role.value; save(); }; notes.oninput = () => { s.notes = notes.value; save(); };
    el.querySelector(".ms").onclick = () => { if (audio.ended || audio.currentTime >= audio.duration - 0.05) { tv.textContent = "⚠ playback has finished: play it, then Mark start while it plays"; tv.style.color = "var(--rej)"; return; } s.start = audio.currentTime; paint(); save(); };
    el.querySelector(".me").onclick = () => { s.end = audio.currentTime; paint(); save(); };
    el.querySelector(".cl").onclick = () => { s.start = s.end = null; paint(); save(); };
    el.querySelector(".pl").onclick = () => {
      audio.currentTime = s.start || 0; audio.play();
      if (s.end != null) { const stop = () => { if (audio.currentTime >= s.end) { audio.pause(); audio.removeEventListener("timeupdate", stop); } }; audio.addEventListener("timeupdate", stop); }
    };
    const pitch = el.querySelector(".pitch");
    pitch.onclick = () => { const on = audio.playbackRate === 1; audio.preservesPitch = false; audio.playbackRate = on ? 1.5 : 1; pitch.classList.toggle("on", on); pitch.style.background = on ? "var(--accent)" : ""; };
    paint(); list.appendChild(el);
  }
}
function results() { return CLIPS.map(c => ({ key: c.key, file: c.file, title: c.title, ...state[c.key] })).filter(r => r.verdict && r.verdict !== "reject"); }
function stats() { const v = Object.values(state); const n = k => v.filter(s => s.verdict === k).length;
  document.getElementById("stats").textContent = `${n("sausage")} sausage · ${n("husky")} husky · ${n("both")} both · ${n("reject")} rejected · ${CLIPS.length - v.filter(s => s.verdict).length} to go`; }
document.getElementById("copy").onclick = async e => { const t = JSON.stringify(results(), null, 1);
  try { await navigator.clipboard.writeText(t); e.target.textContent = "Copied ✓"; } catch (_) { prompt("Copy this:", t); } setTimeout(() => e.target.textContent = "Copy results", 1500); };
document.getElementById("dl").onclick = () => { const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([JSON.stringify(results(), null, 1)], { type: "application/json" })); a.download = "curation.json"; a.click(); };
stats();
</script></body></html>"""
base = sys.argv[3] if len(sys.argv) > 3 else "raw/"
key = sys.argv[4] if len(sys.argv) > 4 else "pet-sound-curation-v1"
title = sys.argv[5] if len(sys.argv) > 5 else "Pet sound curation"
page = page.replace("__BASE__", base).replace("__KEY__", key).replace("__TITLE__", html.escape(title))
open(sys.argv[2], "w", encoding="utf-8").write(page.replace("__MANIFEST__", json.dumps(manifest, ensure_ascii=False).replace("</", "<\\/")))
print("wrote", sys.argv[2], len(manifest), "clips")
