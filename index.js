var http = require("http");
var crypto = require("crypto");

const SUPABASE_URL = "https://gvdjwxxqhxgaeimrhmtf.supabase.co";
const SUPABASE_KEY = "sb_secret_YzxO5nLTgC_UUWvln-Fomg_3kmBwbrd";
const ADMIN_PASS = process.env.ADMIN_PASS || "8fvh3248rvf8723rvgy98j6753u463";
const API_SECRET = "xK9#mP2!vB8@qW5$nH3^jF7&lR4*dC1";

const API_ROUTE = "/x7k2m9p4q1w8v3n6s5a0d2f1g4h7j0k3";
const ADMIN_ROUTE = "/a3b8c1d6e9f2g5h0i7j4k1l8m3n6o9p2";

function signJWT(payload) {
  const header = Buffer.from(JSON.stringify({ alg: "HS256", typ: "JWT" })).toString("base64url");
  const data = Buffer.from(JSON.stringify(payload)).toString("base64url");
  const signature = crypto.createHmac("sha256", API_SECRET).update(header + "." + data).digest("base64url");
  return header + "." + data + "." + signature;
}

function verifyJWT(token) {
  try {
    const parts = token.split(".");
    if (parts.length !== 3) return null;
    const signature = crypto.createHmac("sha256", API_SECRET).update(parts[0] + "." + parts[1]).digest("base64url");
    if (signature !== parts[2]) return null;
    return JSON.parse(Buffer.from(parts[1], "base64url").toString());
  } catch(e) { return null; }
}

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url, "http://localhost");
  const path = url.pathname;
  const headers = {
    "apikey": SUPABASE_KEY,
    "Authorization": "Bearer " + SUPABASE_KEY,
    "Content-Type": "application/json",
    "Access-Control-Allow-Origin": "*"
  };

  if (req.method === "OPTIONS") {
    res.writeHead(204, { "Access-Control-Allow-Origin": "*", "Access-Control-Allow-Methods": "GET,POST,PATCH,DELETE", "Access-Control-Allow-Headers": "Content-Type,Authorization,apikey,x-api-token" });
    res.end();
    return;
  }

  if (req.method === "POST" && path === API_ROUTE) {
    const token = req.headers["x-api-token"];
    if (!token || !verifyJWT(token)) return end(res, 401, { status: "error", msg: "Unauthorized" });
    let body = "";
    req.on("data", c => body += c);
    req.on("end", async () => {
      try {
        const data = JSON.parse(body);
        const { key, hwid } = data;
        if (!key || !hwid) return end(res, 400, { status: "error", msg: "Dados incompletos" });
        let r = await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + key, { headers });
        let keys = await r.json();
        if (!keys.length) return end(res, 404, { status: "error", msg: "Key invalida" });
        if (keys[0].status === "bloqueada") return end(res, 403, { status: "error", msg: "Key bloqueada" });
        r = await fetch(SUPABASE_URL + "/rest/v1/licenses?key_code=eq." + key, { headers });
        let licenses = await r.json();
        if (licenses.length) {
          if (licenses[0].hwid !== hwid) return end(res, 409, { status: "error", msg: "Key ja vinculada" });
          const accessToken = signJWT({ hwid, key, expires: licenses[0].expires_at, type: "access" });
          return end(res, 200, { status: "ok", token: accessToken, expires: licenses[0].expires_at });
        }
        let expires = new Date(); expires.setDate(expires.getDate() + 30);
        let expStr = expires.toISOString().split("T")[0];
        const accessToken = signJWT({ hwid, key, expires: expStr, type: "access" });
        await fetch(SUPABASE_URL + "/rest/v1/licenses", { method: "POST", headers, body: JSON.stringify({ key_code: key, hwid, expires_at: expStr, token: accessToken, status: "ativa" }) });
        await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + key, { method: "PATCH", headers, body: JSON.stringify({ status: "usada" }) });
        end(res, 200, { status: "ok", token: accessToken, expires: expStr });
      } catch(e) { end(res, 500, { status: "error", msg: "Erro interno" }); }
    });
    return;
  }

  if (path === ADMIN_ROUTE && url.searchParams.get("token") !== ADMIN_PASS) {
    res.writeHead(200, { "Content-Type": "text/html;charset=utf-8" });
    res.end('<!DOCTYPE html><html><head><meta charset="UTF-8"><title>Auth</title><style>*{margin:0;padding:0;box-sizing:border-box}body{background:#0a0e14;display:flex;justify-content:center;align-items:center;height:100vh;font-family:Arial}.lb{background:#11161e;border:1px solid #1e2632;border-radius:16px;padding:48px;width:380px;text-align:center}input{background:#0a0e14;border:1px solid #1e2632;color:#fff;padding:12px;border-radius:10px;font-size:15px;width:100%;text-align:center;outline:none;margin-bottom:14px}button{background:#238636;color:#fff;border:none;padding:12px;border-radius:10px;font-size:15px;width:100%;cursor:pointer}</style></head><body><form class="lb" method="GET"><h2 style="color:#fff;margin-bottom:20px">Acesso Restrito</h2><input name="token" type="password" placeholder="Token"><button>Entrar</button></form></body></html>');
    return;
  }

  if (path === ADMIN_ROUTE + "/api") {
    const action = url.searchParams.get("action");
    if (action === "all") return end(res, 200, { keys: await (await fetch(SUPABASE_URL + "/rest/v1/keys?select=*&order=created_at.desc", { headers })).json(), licenses: await (await fetch(SUPABASE_URL + "/rest/v1/licenses?select=*&order=activated_at.desc", { headers })).json() });
    if (action === "create") { let dias = parseInt(url.searchParams.get("dias")) || 30, qtd = parseInt(url.searchParams.get("qtd")) || 1, kk = []; for (let i = 0; i < qtd; i++) { let k = crypto.randomBytes(16).toString("hex"); kk.push(k); await fetch(SUPABASE_URL + "/rest/v1/keys", { method: "POST", headers, body: JSON.stringify({ key_code: k, status: "disponivel" }) }); } return end(res, 200, { status: "ok", keys: kk, dias: dias }); }
    if (action === "delete_key") { await fetch(SUPABASE_URL + "/rest/v1/licenses?key_code=eq." + url.searchParams.get("key"), { method: "DELETE", headers }); await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + url.searchParams.get("key"), { method: "DELETE", headers }); return end(res, 200, { status: "ok" }); }
    if (action === "delete_all") { let type = url.searchParams.get("type"); if (type === "disponiveis") { let kk = await (await fetch(SUPABASE_URL + "/rest/v1/keys?status=eq.disponivel&select=key_code", { headers })).json(); for (let k of kk) { await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + k.key_code, { method: "DELETE", headers }); } } else if (type === "bloqueadas") { let kk = await (await fetch(SUPABASE_URL + "/rest/v1/keys?status=eq.bloqueada&select=key_code", { headers })).json(); for (let k of kk) { await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + k.key_code, { method: "DELETE", headers }); } } else if (type === "todas") { await fetch(SUPABASE_URL + "/rest/v1/keys", { method: "DELETE", headers }); await fetch(SUPABASE_URL + "/rest/v1/licenses", { method: "DELETE", headers }); } return end(res, 200, { status: "ok" }); }
    if (action === "reset_hwid") { await fetch(SUPABASE_URL + "/rest/v1/licenses?key_code=eq." + url.searchParams.get("key"), { method: "DELETE", headers }); await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + url.searchParams.get("key"), { method: "PATCH", headers, body: JSON.stringify({ status: "disponivel" }) }); return end(res, 200, { status: "ok" }); }
    if (action === "block_key") { await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + url.searchParams.get("key"), { method: "PATCH", headers, body: JSON.stringify({ status: "bloqueada" }) }); return end(res, 200, { status: "ok" }); }
    if (action === "unblock_key") { await fetch(SUPABASE_URL + "/rest/v1/keys?key_code=eq." + url.searchParams.get("key"), { method: "PATCH", headers, body: JSON.stringify({ status: "disponivel" }) }); return end(res, 200, { status: "ok" }); }
    if (action === "reduce") { let lic = await (await fetch(SUPABASE_URL + "/rest/v1/licenses?key_code=eq." + url.searchParams.get("key"), { headers })).json(); if (lic.length && lic[0].expires_at) { let d = new Date(lic[0].expires_at); d.setDate(d.getDate() - parseInt(url.searchParams.get("dias") || 7)); let nd = d.toISOString().split("T")[0]; await fetch(SUPABASE_URL + "/rest/v1/licenses?key_code=eq." + url.searchParams.get("key"), { method: "PATCH", headers, body: JSON.stringify({ expires_at: nd }) }); return end(res, 200, { status: "ok", expires: nd }); } return end(res, 200, { status: "error" }); }
    if (action === "add_time") { let lic = await (await fetch(SUPABASE_URL + "/rest/v1/licenses?key_code=eq." + url.searchParams.get("key"), { headers })).json(); if (lic.length && lic[0].expires_at) { let d = new Date(lic[0].expires_at); d.setDate(d.getDate() + parseInt(url.searchParams.get("dias") || 7)); let nd = d.toISOString().split("T")[0]; await fetch(SUPABASE_URL + "/rest/v1/licenses?key_code=eq." + url.searchParams.get("key"), { method: "PATCH", headers, body: JSON.stringify({ expires_at: nd }) }); return end(res, 200, { status: "ok", expires: nd }); } return end(res, 200, { status: "error" }); }
    return end(res, 400, { status: "error" });
  }

  if (path === ADMIN_ROUTE) {
    res.writeHead(200, { "Content-Type": "text/html;charset=utf-8" });
    res.end(getHTML());
    return;
  }

  res.writeHead(404, { "Content-Type": "text/plain" });
  res.end("Not Found");
});

function end(res, code, data) {
  res.writeHead(code, { "Content-Type": "application/json", "Access-Control-Allow-Origin": "*" });
  res.end(JSON.stringify(data));
}

function getHTML() {
  return `<!DOCTYPE html><html lang="pt"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Painel Admin</title>
<style>:root{--bg:#0a0e14;--surface:#11161e;--border:#1e2632;--text:#8b949e;--heading:#e6edf3;--accent:#4493f8;--green:#3fb950;--red:#f85149;--yellow:#d2991d;--purple:#a371f7}
*{margin:0;padding:0;box-sizing:border-box}
body{background:var(--bg);color:var(--text);font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;display:flex;min-height:100vh}
nav{width:240px;background:var(--surface);border-right:1px solid var(--border);padding:24px 0;position:fixed;height:100vh;overflow-y:auto;z-index:10}
nav .logo{padding:0 20px 24px;border-bottom:1px solid var(--border);margin-bottom:16px}
nav .logo h2{color:var(--heading);font-size:18px;font-weight:700}
nav .logo span{color:var(--accent);font-size:11px;text-transform:uppercase;letter-spacing:2px}
nav a{display:flex;align-items:center;gap:10px;padding:12px 20px;color:var(--text);text-decoration:none;font-size:14px;cursor:pointer;transition:all .15s;border-left:3px solid transparent}
nav a:hover{background:rgba(68,147,248,.06);color:var(--heading)}
nav a.active{background:rgba(68,147,248,.1);color:var(--accent);border-left-color:var(--accent)}
nav a .ico{font-size:18px}
main{margin-left:240px;padding:32px 40px;width:100%;max-width:1200px}
h1{color:var(--heading);font-size:26px;font-weight:700;margin-bottom:28px;display:flex;align-items:center;gap:12px}
.grid-4{display:grid;grid-template-columns:repeat(4,1fr);gap:16px;margin-bottom:32px}
.stat{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:20px 24px;transition:all .2s}
.stat:hover{border-color:var(--accent);transform:translateY(-2px)}
.stat .num{font-size:32px;font-weight:800;color:var(--heading)}
.stat .lbl{font-size:12px;color:var(--text);margin-top:6px;text-transform:uppercase;letter-spacing:1px}
.card{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:24px;margin-bottom:24px}
.card h3{color:var(--heading);font-size:17px;font-weight:600;margin-bottom:20px;display:flex;align-items:center;gap:8px}
.row{display:flex;gap:12px;align-items:flex-end;flex-wrap:wrap}
.field{display:flex;flex-direction:column;gap:6px}
.field label{font-size:11px;color:var(--text);text-transform:uppercase;letter-spacing:1px;font-weight:600}
input,select{background:var(--bg);border:1px solid var(--border);color:var(--heading);padding:10px 14px;border-radius:8px;font-size:14px;outline:none;transition:all .15s;font-family:inherit}
input:focus,select:focus{border-color:var(--accent);box-shadow:0 0 0 3px rgba(68,147,248,.12)}
input[type="text"]{width:380px}input[type="number"]{width:100px}
.btn{background:#21262d;border:1px solid #30363d;color:var(--heading);padding:10px 18px;border-radius:8px;cursor:pointer;font-size:13px;font-weight:600;transition:all .15s;white-space:nowrap;font-family:inherit;display:inline-flex;align-items:center;gap:6px}
.btn:hover{background:#30363d;transform:translateY(-1px)}
.btn-accent{background:var(--accent);border-color:var(--accent);color:#fff}.btn-accent:hover{background:#5aa3ff}
.btn-green{background:var(--green);border-color:var(--green);color:#fff}.btn-green:hover{background:#4ac95a}
.btn-red{background:var(--red);border-color:var(--red);color:#fff}.btn-red:hover{background:#ff6655}
.btn-yellow{background:var(--yellow);border-color:var(--yellow);color:#000}.btn-yellow:hover{background:#e0a820}
.btn-purple{background:var(--purple);border-color:var(--purple);color:#fff}.btn-purple:hover{background:#b580f9}
.btn-ghost{background:transparent;border:1px dashed #30363d}.btn-ghost:hover{background:rgba(255,255,255,.03)}
table{width:100%;border-collapse:collapse}
th,td{text-align:left;padding:14px 16px;border-bottom:1px solid var(--border);font-size:13px}
th{color:var(--text);font-weight:600;font-size:11px;text-transform:uppercase;letter-spacing:.8px;background:rgba(0,0,0,.2)}
tr:hover{background:rgba(68,147,248,.04)}
.mono{font-family:'JetBrains Mono','Fira Code',Consolas,monospace;font-size:12px}
.code-block{background:var(--bg);border:1px solid var(--border);border-radius:8px;padding:16px;max-height:600px;overflow-y:auto;font-family:'JetBrains Mono','Fira Code',Consolas,monospace;font-size:12px;color:var(--text);white-space:pre-wrap;line-height:1.8;word-break:break-all}
.badge{display:inline-block;padding:3px 10px;border-radius:20px;font-size:11px;font-weight:700;letter-spacing:.5px}
.badge-green{background:rgba(63,185,80,.15);color:var(--green)}
.badge-red{background:rgba(248,81,73,.15);color:var(--red)}
.badge-yellow{background:rgba(210,153,29,.15);color:var(--yellow)}
.toast{position:fixed;bottom:24px;right:24px;background:var(--surface);border:1px solid var(--border);color:var(--heading);padding:14px 22px;border-radius:10px;font-size:14px;z-index:999;display:none;box-shadow:0 8px 24px rgba(0,0,0,.5)}
.modal-overlay{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,.8);z-index:100;justify-content:center;align-items:center;backdrop-filter:blur(4px)}
.modal-overlay.show{display:flex}
.modal{background:var(--surface);border:1px solid var(--border);border-radius:16px;padding:32px;max-width:480px;width:90%}
.modal h3{color:var(--heading);font-size:20px;margin-bottom:8px}
.modal p{color:var(--text);font-size:14px;margin-bottom:20px;line-height:1.5}
.modal .actions{display:flex;gap:10px;justify-content:flex-end;margin-top:16px}
@media(max-width:768px){nav{width:56px}nav a span.nav-text,nav .logo span,nav .logo h2{display:none}main{margin-left:56px;padding:16px}.grid-4{grid-template-columns:repeat(2,1fr)}input[type="text"]{width:100%}}
</style></head><body>
<nav><div class="logo"><h2>Admin</h2><span>Painel</span></div>
<a class="active" onclick="showTab('dashboard')"><span class="ico">📊</span><span class="nav-text">Dashboard</span></a>
<a onclick="showTab('keys')"><span class="ico">🔑</span><span class="nav-text">Keys</span></a>
<a onclick="showTab('licenses')"><span class="ico">📋</span><span class="nav-text">Licenças</span></a>
<a onclick="showTab('danger')"><span class="ico">⚠️</span><span class="nav-text">Zona de Perigo</span></a></nav>
<main id="mainContent"><div style="text-align:center;padding:60px;color:var(--text)">Carregando...</div></main>
<div class="toast" id="toast"></div>
<div class="modal-overlay" id="modal"><div class="modal" id="modalContent"></div></div>
<script>
var tab='dashboard',keys=[],licenses=[];
async function api(a,p){return(await fetch(window.location.pathname+'/api?action='+a+(p||''))).json();}
async function load(){var d=await api('all');keys=d.keys||[];licenses=d.licenses||[];render();}
function showTab(t){tab=t;document.querySelectorAll('nav a').forEach(function(a,i){var txt=a.textContent.toLowerCase();a.classList.toggle('active',(t==='dashboard'&&txt.includes('dashboard'))||(t==='keys'&&txt.includes('keys'))||(t==='licenses'&&txt.includes('licen'))||(t==='danger'&&txt.includes('zona')))});render();}
function showModal(title,msg,confirmText,confirmClass,onConfirm,extra){document.getElementById('modal').classList.add('show');document.getElementById('modalContent').innerHTML='<h3>'+title+'</h3><p>'+msg+'</p>'+(extra||'')+'<div class="actions"><button class="btn btn-ghost" onclick="closeModal()">Cancelar</button><button class="btn '+confirmClass+'" id="confirmBtn">'+confirmText+'</button></div>';document.getElementById('confirmBtn').onclick=function(){closeModal();onConfirm();};}
function closeModal(){document.getElementById('modal').classList.remove('show');}
function toast(m){var t=document.getElementById('toast');t.innerText=m;t.style.display='block';setTimeout(function(){t.style.display='none'},3000);}
function render(){
  var m=document.getElementById('mainContent');
  if(tab=='dashboard'){var t=keys.length,a=licenses.filter(function(l){return l.status=='ativa'}).length,d=keys.filter(function(k){return k.status=='disponivel'}).length,b=keys.filter(function(k){return k.status=='bloqueada'}).length;m.innerHTML='<h1>Dashboard</h1><div class="grid-4"><div class="stat"><div class="num">'+t+'</div><div class="lbl">Total</div></div><div class="stat"><div class="num">'+a+'</div><div class="lbl">Ativas</div></div><div class="stat"><div class="num">'+d+'</div><div class="lbl">Disponiveis</div></div><div class="stat"><div class="num">'+b+'</div><div class="lbl">Bloqueadas</div></div></div><div class="card"><h3>Licencas Ativas</h3><table><tr><th>Key</th><th>HWID</th><th>Ativada</th><th>Expira</th><th>Status</th></tr>'+licenses.filter(function(l){return l.status=='ativa'}).slice(0,10).map(function(l){var dias=Math.ceil((new Date(l.expires_at)-new Date())/86400000);return'<tr><td class="mono" style="color:var(--accent)">'+(l.key_code||'').substring(0,20)+'...</td><td class="mono">'+(l.hwid||'').substring(0,12)+'...</td><td>'+(l.activated_at||'').split('T')[0]+'</td><td>'+l.expires_at+'</td><td><span class="badge '+(dias<7?'badge-red':dias<14?'badge-yellow':'badge-green')+'">'+dias+'d</span></td></tr>';}).join('')+'</table></div>';}
  else if(tab=='keys'){m.innerHTML='<h1>Keys</h1><div class="card"><h3>Criar Key</h3><div class="row"><div class="field"><label>Dias</label><input type="number" id="createDias" value="30"></div><div class="field"><label>Qtd</label><input type="number" id="createQtd" value="1"></div><button class="btn btn-accent" onclick="createKeys()">Criar</button></div><div id="createdKeys" class="code-block" style="display:none;margin-top:12px"></div></div><div class="card"><h3>Acoes</h3><div class="row"><input type="text" id="keyInput" placeholder="Key"><button class="btn btn-accent" onclick="viewKey()">Ver</button><button class="btn btn-yellow" onclick="resetHWID()">Resetar HWID</button><button class="btn btn-purple" onclick="addTime()">+7d</button><button class="btn" onclick="reduceTime()">-7d</button><button class="btn btn-red" onclick="blockKey()">Bloquear</button><button class="btn btn-green" onclick="unblockKey()">Desbloquear</button><button class="btn btn-red" onclick="deleteSingleKey()">Deletar</button></div><div id="keyInfo" style="display:none;margin-top:12px;padding:16px;background:var(--bg);border:1px solid var(--border);border-radius:8px;font-family:monospace;font-size:13px;color:var(--text);white-space:pre-wrap;line-height:1.6"></div></div><div class="card"><h3>Disponiveis ('+keys.filter(function(k){return k.status=='disponivel'}).length+')</h3><div class="code-block">'+(keys.filter(function(k){return k.status=='disponivel'}).slice(0,50).map(function(k){return k.key_code}).join('\\n')||'Nenhuma')+'</div></div>';}
  else if(tab=='licenses'){var ativas=licenses.filter(function(l){return l.status=='ativa'}),inativas=ativas.filter(function(l){return new Date(l.expires_at)<new Date(Date.now()-172800000)});m.innerHTML='<h1>Licencas</h1><div class="card"><h3>Ativas ('+ativas.length+')</h3><table><tr><th>Key</th><th>HWID</th><th>Ativada</th><th>Expira</th><th>Status</th></tr>'+ativas.map(function(l){var dias=Math.ceil((new Date(l.expires_at)-new Date())/86400000);return'<tr><td class="mono" style="color:var(--accent)">'+(l.key_code||'').substring(0,20)+'...</td><td class="mono">'+(l.hwid||'').substring(0,12)+'...</td><td>'+(l.activated_at||'').split('T')[0]+'</td><td>'+l.expires_at+'</td><td><span class="badge '+(dias<7?'badge-red':dias<14?'badge-yellow':'badge-green')+'">'+dias+'d</span></td></tr>';}).join('')+'</table></div><div class="card"><h3>Inativas +2d ('+inativas.length+')</h3><table><tr><th>Key</th><th>HWID</th><th>Expirou</th></tr>'+inativas.map(function(l){return'<tr><td class="mono" style="color:var(--accent)">'+(l.key_code||'').substring(0,20)+'...</td><td class="mono">'+(l.hwid||'').substring(0,12)+'...</td><td>'+l.expires_at+'</td></tr>';}).join('')+'</table></div>';}
  else if(tab=='danger'){m.innerHTML='<h1>Zona de Perigo</h1><div class="card"><h3>Deletar em Lote</h3><p style="margin-bottom:16px">Acoes irreversiveis</p><div class="row"><button class="btn btn-red" onclick="deleteAllKeys(\'todas\')">Deletar TODAS</button><button class="btn btn-yellow" onclick="deleteAllKeys(\'disponiveis\')">Deletar Disponiveis</button><button class="btn" onclick="deleteAllKeys(\'bloqueadas\')">Deletar Bloqueadas</button></div></div><div class="card"><h3>Info</h3><table><tr><td>Total</td><td><strong>'+keys.length+'</strong></td></tr><tr><td>Ativas</td><td><strong>'+licenses.filter(function(l){return l.status=='ativa'}).length+'</strong></td></tr><tr><td>Disponiveis</td><td><strong>'+keys.filter(function(k){return k.status=='disponivel'}).length+'</strong></td></tr><tr><td>Bloqueadas</td><td><strong>'+keys.filter(function(k){return k.status=='bloqueada'}).length+'</strong></td></tr></table></div>';}
}
async function createKeys(){var d=parseInt(document.getElementById('createDias').value)||30;var q=parseInt(document.getElementById('createQtd').value)||1;var r=await api('create','&dias='+d+'&qtd='+q);document.getElementById('createdKeys').style.display='block';document.getElementById('createdKeys').innerHTML='<div style="color:var(--green);margin-bottom:8px">'+r.keys.length+' key(s) criada(s) com '+d+' dias</div>'+r.keys.join('\\n');toast('Criada(s)!');load();}
async function viewKey(){var k=getKey();if(!k)return;var kd=keys.find(function(x){return x.key_code==k}),ld=licenses.find(function(x){return x.key_code==k});var t='KEY: '+k+'\\nStatus: '+(kd?kd.status:'?');if(ld)t+='\\nHWID: '+ld.hwid+'\\nAtivada: '+(ld.activated_at||'').split('T')[0]+'\\nExpira: '+ld.expires_at+'\\nDias: '+Math.ceil((new Date(ld.expires_at)-new Date())/86400000);document.getElementById('keyInfo').style.display='block';document.getElementById('keyInfo').innerText=t;}
async function resetHWID(){var k=getKey();if(!k)return;showModal('Resetar HWID','<code>'+k+'</code>','Confirmar','btn-yellow',async function(){await api('reset_hwid','&key='+k);toast('Resetado!');load();});}
async function deleteSingleKey(){var k=getKey();if(!k)return;showModal('Deletar','<b style="color:var(--red)">PERMANENTE!</b><br><code>'+k+'</code>','Deletar','btn-red',async function(){await api('delete_key','&key='+k);toast('Deletada!');load();});}
async function deleteAllKeys(type){var labels={todas:'TODAS',disponiveis:'DISPONIVEIS',bloqueadas:'BLOQUEADAS'};showModal('Deletar','<b>'+labels[type]+'</b>','Confirmar','btn-red',async function(){var pass=document.getElementById('confirmPass').value;if(pass!=='8fvh3248rvf8723rvgy98j6753u463'){toast('Senha incorreta!');return;}await api('delete_all','&type='+type);toast('Deletadas!');load();},'<div class="field" style="margin-top:12px"><label>Senha</label><input type="password" id="confirmPass" style="width:100%"></div>');}
async function blockKey(){var k=getKey();if(!k)return;await api('block_key','&key='+k);toast('Bloqueada!');load();}
async function unblockKey(){var k=getKey();if(!k)return;await api('unblock_key','&key='+k);toast('Desbloqueada!');load();}
async function reduceTime(){var k=getKey();if(!k)return;var r=await api('reduce','&key='+k);if(r.status=='ok')toast('Expira: '+r.expires);else toast('Nao ativada');load();}
async function addTime(){var k=getKey();if(!k)return;var r=await api('add_time','&key='+k);if(r.status=='ok')toast('Expira: '+r.expires);else toast('Nao ativada');load();}
function getKey(){return document.getElementById('keyInput')?.value?.trim()||'';}
load();
</script></body></html>`;
}

const PORT = process.env.PORT || 8080;
server.listen(PORT, () => {
  console.log("Painel: " + ADMIN_ROUTE);
  console.log("API: " + API_ROUTE);
});
