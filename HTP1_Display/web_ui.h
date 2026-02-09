#pragma once

#include <Arduino.h>

static const char WEB_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HTP-1 Display</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
  background:#1a1a2e;color:#e0e0e0;min-height:100vh}
.header{background:#16213e;padding:12px 20px;display:flex;justify-content:space-between;align-items:center;
  border-bottom:1px solid #0f3460}
.header h1{font-size:1.2em;color:#e94560}
.header .ver{font-size:0.8em;color:#888}
.status-bar{background:#0f3460;padding:8px 20px;display:flex;gap:20px;flex-wrap:wrap;font-size:0.85em}
.status-bar .item{display:flex;align-items:center;gap:5px}
.status-bar .dot{width:8px;height:8px;border-radius:50%;display:inline-block}
.dot.on{background:#4ecca3}.dot.off{background:#e94560}
.container{max-width:720px;margin:20px auto;padding:0 16px}
.card{background:#16213e;border-radius:8px;padding:16px 20px;margin-bottom:16px;border:1px solid #0f3460}
.card h2{font-size:1em;color:#e94560;margin-bottom:12px;border-bottom:1px solid #0f3460;padding-bottom:8px}
.field{margin-bottom:12px;display:flex;flex-wrap:wrap;align-items:center;gap:8px}
.field label{min-width:140px;font-size:0.9em;color:#aaa}
.field input[type=text],.field input[type=number],.field input[type=password],.field select{
  flex:1;min-width:160px;padding:8px 10px;background:#1a1a2e;border:1px solid #0f3460;
  border-radius:4px;color:#e0e0e0;font-size:0.9em}
.field input:focus,.field select:focus{outline:none;border-color:#e94560}
.themes{display:flex;gap:8px;flex-wrap:wrap}
.themes .swatch{width:36px;height:36px;border-radius:50%;cursor:pointer;border:3px solid transparent;
  transition:border-color 0.2s}
.themes .swatch.active{border-color:#fff}
.themes .swatch:hover{opacity:0.8}
.btn{padding:8px 20px;border:none;border-radius:4px;cursor:pointer;font-size:0.9em;
  transition:background 0.2s}
.btn-primary{background:#e94560;color:#fff}
.btn-primary:hover{background:#c73652}
.btn-secondary{background:#0f3460;color:#e0e0e0}
.btn-secondary:hover{background:#16213e}
.btn-row{display:flex;gap:10px;margin-top:8px}
.slider-wrap{flex:1;display:flex;align-items:center;gap:10px}
.slider-wrap input[type=range]{flex:1;accent-color:#e94560}
.slider-wrap .val{min-width:30px;text-align:right;font-size:0.9em;color:#4ecca3}
.toggle{position:relative;width:44px;height:24px}
.toggle input{opacity:0;width:0;height:0}
.toggle .slider{position:absolute;inset:0;background:#333;border-radius:12px;cursor:pointer;
  transition:background 0.3s}
.toggle .slider::before{content:'';position:absolute;width:18px;height:18px;left:3px;top:3px;
  background:#fff;border-radius:50%;transition:transform 0.3s}
.toggle input:checked+.slider{background:#4ecca3}
.toggle input:checked+.slider::before{transform:translateX(20px)}
.upload-zone{border:2px dashed #0f3460;border-radius:8px;padding:30px;text-align:center;
  cursor:pointer;transition:border-color 0.3s}
.upload-zone:hover{border-color:#e94560}
.upload-zone.active{border-color:#4ecca3}
.progress{width:100%;height:6px;background:#1a1a2e;border-radius:3px;margin-top:12px;display:none}
.progress .bar{height:100%;background:#4ecca3;border-radius:3px;width:0%;transition:width 0.3s}
.msg{padding:8px 12px;border-radius:4px;margin-top:8px;display:none;font-size:0.85em}
.msg.ok{display:block;background:#1a3a2a;color:#4ecca3;border:1px solid #4ecca3}
.msg.err{display:block;background:#3a1a1a;color:#e94560;border:1px solid #e94560}
.input-row{display:flex;gap:8px;align-items:center;margin-bottom:6px}
.input-row input{padding:8px 10px;background:#1a1a2e;border:1px solid #0f3460;
  border-radius:4px;color:#e0e0e0;font-size:0.9em}
.input-row input.code{width:80px}.input-row input.name{flex:1}
.input-row .del{background:none;border:none;color:#e94560;cursor:pointer;font-size:1.2em;padding:4px 8px}
@media(max-width:480px){.field{flex-direction:column;align-items:flex-start}
  .field label{min-width:auto}}
</style>
</head>
<body>
<div class="header">
  <h1>HTP-1 Display</h1>
  <span class="ver" id="fw">v--</span>
</div>
<div class="status-bar" id="statusBar">
  <div class="item"><span class="dot" id="dotWifi"></span><span id="stWifi">WiFi: --</span></div>
  <div class="item"><span class="dot" id="dotHtp"></span><span id="stHtp">HTP-1: --</span></div>
  <div class="item" id="stVolWrap">Vol: <strong id="stVol">--</strong></div>
  <div class="item">Input: <span id="stInput">--</span></div>
  <div class="item">Codec: <span id="stCodec">--</span></div>
</div>
<div class="container">

  <!-- WiFi Settings -->
  <div class="card">
    <h2>WiFi</h2>
    <div class="field"><label>SSID</label><input type="text" id="ssid" maxlength="63"></div>
    <div class="field"><label>Password</label><input type="password" id="wifipass" maxlength="63"></div>
  </div>

  <!-- HTP-1 Connection -->
  <div class="card">
    <h2>HTP-1 Connection</h2>
    <div class="field"><label>IP Address</label><input type="text" id="htp1ip" placeholder="192.168.1.x"></div>
    <div class="field"><label>Port</label><input type="number" id="htp1port" min="1" max="65535"></div>
    <div class="field"><label>Volume Offset</label><input type="number" id="voloff" min="-20" max="20"></div>
  </div>

  <!-- Input Names -->
  <div class="card">
    <h2>Input Names</h2>
    <p style="font-size:0.8em;color:#888;margin-bottom:10px">Map HTP-1 input codes (e.g. h1, h2, usb) to friendly names</p>
    <div id="inputRows"></div>
    <button class="btn btn-secondary" style="margin-top:8px" onclick="addInputRow('','')">+ Add Input</button>
  </div>

  <!-- Display Settings -->
  <div class="card">
    <h2>Display</h2>
    <div class="field"><label>Brightness</label>
      <div class="slider-wrap">
        <input type="range" id="bright" min="0" max="6" step="1">
        <span class="val" id="brightVal">3</span>
      </div>
    </div>
    <div class="field"><label>Auto-dim (sec)</label>
      <input type="number" id="dimtime" min="1" max="300"></div>
    <div class="field"><label>Dim Brightness</label>
      <div class="slider-wrap">
        <input type="range" id="dimbrt" min="1" max="60" step="1">
        <span class="val" id="dimbrtVal">7</span>
      </div>
    </div>
    <div class="field"><label>Display Mode</label>
      <select id="dmode">
        <option value="0">Volume Only</option>
        <option value="1">Volume + Source</option>
        <option value="2">Volume + Codec</option>
        <option value="3">Full Status</option>
      </select>
    </div>
    <div class="field"><label>Volume Size</label>
      <div class="slider-wrap">
        <input type="range" id="volsize" min="1" max="5" step="1">
        <span class="val" id="volsizeVal">3 (144px)</span>
      </div>
    </div>
    <div class="field"><label>Label Size</label>
      <div class="slider-wrap">
        <input type="range" id="labelsize" min="1" max="3" step="1">
        <span class="val" id="labelsizeVal">1 (26px)</span>
      </div>
    </div>
    <div class="field"><label>Color Theme</label>
      <div class="themes" id="themeRow">
        <div class="swatch" data-t="0" style="background:#fff" title="White"></div>
        <div class="swatch" data-t="1" style="background:#0f0" title="Green"></div>
        <div class="swatch" data-t="2" style="background:#ffbe00" title="Amber"></div>
        <div class="swatch" data-t="3" style="background:#00f" title="Blue"></div>
        <div class="swatch" data-t="4" style="background:#f00" title="Red"></div>
        <div class="swatch" data-t="5" style="background:#0ff" title="Cyan"></div>
      </div>
    </div>
  </div>

  <!-- Power Management -->
  <div class="card">
    <h2>Power Management</h2>
    <div class="field"><label>Sleep Enabled</label>
      <label class="toggle"><input type="checkbox" id="sleepen"><span class="slider"></span></label>
    </div>
    <div class="field"><label>Sleep Timeout (sec)</label>
      <input type="number" id="sleeptm" min="10" max="3600"></div>
  </div>

  <div class="btn-row">
    <button class="btn btn-primary" onclick="saveSettings()">Save Settings</button>
    <button class="btn btn-secondary" onclick="resetSettings()">Reset to Defaults</button>
  </div>
  <div class="msg" id="settingsMsg"></div>

  <!-- OTA Firmware Update -->
  <div class="card" style="margin-top:16px">
    <h2>Firmware Update</h2>
    <div class="upload-zone" id="uploadZone" onclick="document.getElementById('fwFile').click()">
      <p>Click or drag .bin file here</p>
      <input type="file" id="fwFile" accept=".bin" style="display:none">
    </div>
    <div class="progress" id="progWrap"><div class="bar" id="progBar"></div></div>
    <div class="msg" id="otaMsg"></div>
  </div>

</div>

<script>
let currentTheme=0;
let volSizes=[5,4,4,4];
let labelSizes=[1,2,2,1];
const $=id=>document.getElementById(id);

function setTheme(t){
  currentTheme=t;
  document.querySelectorAll('.swatch').forEach(s=>{
    s.classList.toggle('active',parseInt(s.dataset.t)===t);
  });
}
document.querySelectorAll('.swatch').forEach(s=>{
  s.addEventListener('click',()=>setTheme(parseInt(s.dataset.t)));
});

$('bright').oninput=function(){$('brightVal').textContent=this.value};
$('dimbrt').oninput=function(){$('dimbrtVal').textContent=this.value};

function volSizeLabel(v){return v+' ('+v*48+'px)'}
function labelSizeLabel(v){return v+' ('+v*26+'px)'}
function updateSizeSliders(){
  var m=parseInt($('dmode').value);
  $('volsize').value=volSizes[m];
  $('volsizeVal').textContent=volSizeLabel(volSizes[m]);
  $('labelsize').value=labelSizes[m];
  $('labelsizeVal').textContent=labelSizeLabel(labelSizes[m]);
}
$('volsize').oninput=function(){
  var m=parseInt($('dmode').value),v=parseInt(this.value);
  volSizes[m]=v;$('volsizeVal').textContent=volSizeLabel(v);
};
$('labelsize').oninput=function(){
  var m=parseInt($('dmode').value),v=parseInt(this.value);
  labelSizes[m]=v;$('labelsizeVal').textContent=labelSizeLabel(v);
};
$('dmode').addEventListener('change',updateSizeSliders);

function addInputRow(code,name){
  const row=document.createElement('div');row.className='input-row';
  row.innerHTML='<input class="code" type="text" maxlength="7" placeholder="h1" value="'+
    code.replace(/"/g,'&quot;')+'"><input class="name" type="text" maxlength="31" placeholder="Apple TV" value="'+
    name.replace(/"/g,'&quot;')+'"><button class="del" title="Remove">&times;</button>';
  row.querySelector('.del').onclick=function(){row.remove()};
  $('inputRows').appendChild(row);
}

function getInputNames(){
  const rows=document.querySelectorAll('#inputRows .input-row');
  const arr=[];
  rows.forEach(r=>{
    const c=r.querySelector('.code').value.trim();
    const n=r.querySelector('.name').value.trim();
    if(c)arr.push({code:c,name:n});
  });
  return arr;
}

function loadInputNames(inputs){
  $('inputRows').innerHTML='';
  if(inputs&&inputs.length){
    inputs.forEach(i=>addInputRow(i.code||'',i.name||''));
  }
}

function loadSettings(){
  fetch('/settings').then(r=>r.json()).then(d=>{
    $('ssid').value=d.ssid||'';
    $('wifipass').value=d.pass||'';
    $('htp1ip').value=d.htp1ip||'';
    $('htp1port').value=d.htp1port||80;
    $('voloff').value=d.voloff||7;
    $('bright').value=d.bright||3;$('brightVal').textContent=d.bright||3;
    $('dimtime').value=Math.round((d.dimtime||3000)/1000);
    $('dimbrt').value=d.dimbrt||7;$('dimbrtVal').textContent=d.dimbrt||7;
    $('dmode').value=d.dmode||0;
    if(d.volSizes)volSizes=d.volSizes.slice();
    if(d.labelSizes)labelSizes=d.labelSizes.slice();
    updateSizeSliders();
    setTheme(d.theme||0);
    $('sleepen').checked=!!d.sleepen;
    $('sleeptm').value=Math.round((d.sleeptm||60000)/1000);
    $('fw').textContent='v'+(d.fw||'?');
    loadInputNames(d.inputs);
  }).catch(()=>{});
}

function saveSettings(){
  const body=JSON.stringify({
    ssid:$('ssid').value,
    pass:$('wifipass').value,
    htp1ip:$('htp1ip').value,
    htp1port:parseInt($('htp1port').value),
    voloff:parseInt($('voloff').value),
    bright:parseInt($('bright').value),
    dimtime:parseInt($('dimtime').value)*1000,
    dimbrt:parseInt($('dimbrt').value),
    dmode:parseInt($('dmode').value),
    volSizes:volSizes,
    labelSizes:labelSizes,
    theme:currentTheme,
    sleepen:$('sleepen').checked,
    sleeptm:parseInt($('sleeptm').value)*1000,
    inputs:getInputNames()
  });
  fetch('/settings',{method:'POST',headers:{'Content-Type':'application/json'},body})
    .then(r=>r.json()).then(d=>{
      const m=$('settingsMsg');
      if(d.ok){m.className='msg ok';m.textContent='Settings saved. Some changes require reboot.';}
      else{m.className='msg err';m.textContent='Error saving settings.';}
    }).catch(()=>{
      const m=$('settingsMsg');m.className='msg err';m.textContent='Connection error.';
    });
}

function resetSettings(){
  if(!confirm('Reset all settings to factory defaults?'))return;
  fetch('/settings',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({reset:true})}).then(()=>loadSettings());
}

function pollStatus(){
  fetch('/status').then(r=>r.json()).then(d=>{
    $('dotWifi').className='dot '+(d.wifi?'on':'off');
    $('stWifi').textContent='WiFi: '+(d.wifi?d.ip+' ('+d.rssi+'dBm)':'disconnected');
    $('dotHtp').className='dot '+(d.htp1?'on':'off');
    $('stHtp').textContent='HTP-1: '+(d.htp1?'connected':'disconnected');
    $('stVol').textContent=d.muted?'MUTE':d.vol;
    $('stInput').textContent=d.input||'--';
    $('stCodec').textContent=d.codec||'--';
  }).catch(()=>{});
}

// OTA Upload
const zone=$('uploadZone'),fwFile=$('fwFile');
['dragenter','dragover'].forEach(e=>zone.addEventListener(e,ev=>{ev.preventDefault();zone.classList.add('active')}));
['dragleave','drop'].forEach(e=>zone.addEventListener(e,ev=>{ev.preventDefault();zone.classList.remove('active')}));
zone.addEventListener('drop',ev=>{if(ev.dataTransfer.files.length)uploadFW(ev.dataTransfer.files[0])});
fwFile.addEventListener('change',()=>{if(fwFile.files.length)uploadFW(fwFile.files[0])});

function uploadFW(file){
  if(!file.name.endsWith('.bin')){
    const m=$('otaMsg');m.className='msg err';m.textContent='Please select a .bin file.';return;
  }
  const xhr=new XMLHttpRequest();
  const prog=$('progWrap'),bar=$('progBar'),msg=$('otaMsg');
  prog.style.display='block';msg.style.display='none';bar.style.width='0%';

  xhr.open('POST','/update');
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable)bar.style.width=Math.round(e.loaded/e.total*100)+'%';
  };
  xhr.onload=function(){
    if(xhr.status===200){
      msg.className='msg ok';msg.textContent='Update successful! Rebooting...';
      setTimeout(()=>location.reload(),8000);
    }else{
      msg.className='msg err';msg.textContent='Update failed: '+xhr.responseText;
    }
  };
  xhr.onerror=function(){msg.className='msg err';msg.textContent='Upload error.';};

  const fd=new FormData();fd.append('firmware',file);
  xhr.send(fd);
}

loadSettings();
setInterval(pollStatus,2000);
pollStatus();
</script>
</body>
</html>)rawliteral";
