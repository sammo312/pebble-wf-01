var DEFAULTS = {
  timeColor: 0xFFFFFF,
  asciiColor: 0x555555,
  faceBgColor: 0x000000,
  densityIndex: 0,
  cellSizeIndex: 1,
  touchIntensity: 1,
  touchDuration: 2,
  ambientTimeout: 0
};

var TIME_PRESETS = [0xFFFFFF, 0x00CC44, 0xFFCC00, 0x00CCCC, 0xFF8800, 0xFF44AA];
var ASCII_PRESETS = [0x555555, 0x333333, 0x008844, 0x884400, 0x004488, 0x880044];
var BG_PRESETS    = [0x000000, 0x111122, 0x002211, 0x221100, 0x110022, 0x222222];

var DENSITY_LABELS = [".-+*#@", ".,oO@", "/|+*#", ":;io8", "<>=#%"];
var CELL_LABELS = ["Small", "Medium", "Large"];
var TOUCH_INT_LABELS = ["Subtle", "Normal", "Strong"];
var TOUCH_DUR_LABELS = ["1s", "3s", "6s", "10s"];
var AMBIENT_LABELS = ["Always", "30s", "1m", "5m", "10m"];

function toHex(n) {
  var s = n.toString(16);
  while (s.length < 6) s = "0" + s;
  return "#" + s;
}

function buildConfigPage(current) {
  var html = [
    "<!DOCTYPE html><html><head>",
    '<meta name="viewport" content="width=device-width,initial-scale=1">',
    "<style>",
    "body{background:#0a0a0a;color:#eee;font-family:-apple-system,sans-serif;padding:0 16px 16px;margin:0;font-size:15px}",
    "h2{margin:14px 0 6px;font-size:13px;text-transform:uppercase;letter-spacing:.06em;color:#888;font-weight:600}",
    ".preview{position:sticky;top:0;background:#0a0a0a;padding:14px 0 10px;z-index:10;border-bottom:1px solid #1a1a1a;margin-bottom:6px}",
    ".pv-frame{width:200px;height:228px;margin:0 auto;border-radius:14px;overflow:hidden;font-family:Menlo,Consolas,monospace;font-size:9px;line-height:9px;letter-spacing:0;white-space:pre;text-align:center;padding:6px 0;box-sizing:border-box}",
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(60px,1fr));gap:6px}",
    ".colorgrid{display:grid;grid-template-columns:repeat(6,1fr) 1.4fr;gap:6px;align-items:center}",
    "button.opt{padding:14px 8px;border:1px solid #333;background:#181818;color:#eee;border-radius:8px;font:inherit;cursor:pointer;text-align:center}",
    "button.opt.sel{background:#fff;color:#000;border-color:#fff}",
    "button.swatch{width:100%;height:42px;border:2px solid #333;border-radius:8px;cursor:pointer;padding:0}",
    "button.swatch.sel{border-color:#fff;border-width:3px}",
    ".picker{display:flex;align-items:center;gap:6px;padding:6px 8px;border:1px solid #333;border-radius:8px;background:#181818;height:42px;box-sizing:border-box}",
    ".picker input[type=color]{width:30px;height:28px;border:0;background:transparent;padding:0;cursor:pointer}",
    ".picker span{font-size:12px;color:#aaa;font-family:Menlo,monospace}",
    ".save{width:100%;padding:16px;margin-top:24px;background:#fff;color:#000;border:0;border-radius:8px;font-size:16px;font-weight:700;cursor:pointer}",
    ".cancel{width:100%;padding:12px;margin-top:8px;background:transparent;color:#888;border:1px solid #333;border-radius:8px;font-size:14px;cursor:pointer}",
    "</style></head><body>",

    '<div class="preview"><div class="pv-frame" id="pv"></div></div>',

    "<h2>Time Color</h2>",
    colorRow("timeColor", TIME_PRESETS),

    "<h2>ASCII Field Color</h2>",
    colorRow("asciiColor", ASCII_PRESETS),

    "<h2>Face Background</h2>",
    colorRow("faceBgColor", BG_PRESETS),

    "<h2>Character Style</h2>",
    optRow("density", DENSITY_LABELS),

    "<h2>Cell Size</h2>",
    optRow("cellSize", CELL_LABELS),

    "<h2>Touch Intensity</h2>",
    optRow("touchIntensity", TOUCH_INT_LABELS),

    "<h2>Touch Trail Duration</h2>",
    optRow("touchDuration", TOUCH_DUR_LABELS),

    "<h2>Ambient Animation Timeout</h2>",
    optRow("ambientTimeout", AMBIENT_LABELS),

    '<button class="save" onclick="save()">Save</button>',
    '<button class="cancel" onclick="cancel()">Cancel</button>',

    "<script>",
    "var current=" + JSON.stringify(current) + ";",
    "var optKeys={density:'densityIndex',cellSize:'cellSizeIndex',touchIntensity:'touchIntensity',touchDuration:'touchDuration',ambientTimeout:'ambientTimeout'};",
    "var colorKeys=['timeColor','asciiColor','faceBgColor'];",
    "var densityRamps=" + JSON.stringify(DENSITY_LABELS) + ";",

    "function toHex(n){var s=n.toString(16);while(s.length<6)s='0'+s;return '#'+s;}",

    "function paint(){",
    "  Object.keys(optKeys).forEach(function(k){",
    "    var sec=document.getElementById(k);",
    "    var val=current[optKeys[k]];",
    "    Array.prototype.forEach.call(sec.querySelectorAll('button.opt'),function(b){",
    "      var v=parseInt(b.getAttribute('data-v'),10);",
    "      if(v===val)b.classList.add('sel');else b.classList.remove('sel');",
    "    });",
    "  });",
    "  colorKeys.forEach(function(k){",
    "    var sec=document.getElementById(k);",
    "    var val=current[k];",
    "    Array.prototype.forEach.call(sec.querySelectorAll('button.swatch'),function(b){",
    "      var v=parseInt(b.getAttribute('data-v'),10);",
    "      if(v===val)b.classList.add('sel');else b.classList.remove('sel');",
    "    });",
    "    var picker=sec.querySelector('input[type=color]');",
    "    var label=sec.querySelector('.picker span');",
    "    picker.value=toHex(val);",
    "    label.textContent=toHex(val).toUpperCase();",
    "  });",
    "  renderPreview();",
    "}",

    "var DIGITS={'0':['111','101','101','101','111'],'1':['010','110','010','010','111'],'2':['110','001','010','100','111'],'3':['111','001','011','001','111'],'4':['101','101','111','001','001'],'5':['111','100','111','001','111'],'6':['111','100','111','101','111'],'7':['111','001','010','010','010'],'8':['111','101','111','101','111'],'9':['111','101','111','001','111']};",
    "var pvTick=0;",
    "function fsin(a){return Math.sin((a&255)/256*2*Math.PI)*127;}",
    "function fcos(a){return Math.cos((a&255)/256*2*Math.PI)*127;}",
    "function fieldBrightness(col,row,t){",
    "  var a=(col*14+t*3)&255;",
    "  var b=(row*12-t*2)&255;",
    "  var c=((col+row)*9+t*4)&255;",
    "  var d=((col-row)*11+t*1)&255;",
    "  var v=fsin(a)+fcos(b)+fsin(c)+fcos(d);",
    "  v=v+512;",
    "  if(v<0)v=0;if(v>1023)v=1023;",
    "  return v;",
    "}",
    "function renderPreview(){",
    "  var pv=document.getElementById('pv');",
    "  var ramp=densityRamps[current.densityIndex]||'.-+*#@';",
    "  var rampLen=ramp.length;",
    "  var cols=22,rows=24;",
    "  var d=new Date();",
    "  var hh=('0'+d.getHours()).slice(-2);",
    "  var mm=('0'+d.getMinutes()).slice(-2);",
    "  var digits=[hh.charAt(0),hh.charAt(1),mm.charAt(0),mm.charAt(1)];",
    "  pv.style.background=toHex(current.faceBgColor);",
    "  var blockW=7,blockH=11;",
    "  var ox=Math.floor((cols-blockW)/2),oy=Math.floor((rows-blockH)/2);",
    "  var timeHex=toHex(current.timeColor),asciiHex=toHex(current.asciiColor);",
    "  var topChar=ramp.charAt(rampLen-1);",
    "  var html='';",
    "  for(var r=0;r<rows;r++){",
    "    for(var c=0;c<cols;c++){",
    "      var lit=0;",
    "      var lr=r-oy,lc=c-ox;",
    "      if(lr>=0&&lr<blockH&&lc>=0&&lc<blockW){",
    "        var line=-1,dy=0,side=-1,dx=0;",
    "        if(lr<5){line=0;dy=lr;}else if(lr>5){line=1;dy=lr-6;}",
    "        if(lc<3){side=0;dx=lc;}else if(lc>3){side=1;dx=lc-4;}",
    "        if(line>=0&&side>=0){",
    "          var dchar=digits[line*2+side];",
    "          var pat=DIGITS[dchar][dy];",
    "          if(pat&&pat.charAt(dx)==='1')lit=1;",
    "        }",
    "      }",
    "      if(lit){",
    "        html+='<span style=\"color:'+timeHex+'\">'+topChar+'</span>';",
    "      }else{",
    "        var v=fieldBrightness(c,r,pvTick);",
    "        var idx=Math.floor((v*rampLen)/1024);",
    "        if(idx>=rampLen)idx=rampLen-1;",
    "        if(idx<=0){html+=' ';}else{html+='<span style=\"color:'+asciiHex+'\">'+ramp.charAt(idx)+'</span>';}",
    "      }",
    "    }",
    "    html+='\\n';",
    "  }",
    "  pv.innerHTML=html;",
    "}",
    "setInterval(function(){pvTick++;renderPreview();},100);",

    "Object.keys(optKeys).forEach(function(k){",
    "  var sec=document.getElementById(k);",
    "  sec.addEventListener('click',function(e){",
    "    var b=e.target.closest('button.opt');if(!b)return;",
    "    current[optKeys[k]]=parseInt(b.getAttribute('data-v'),10);paint();",
    "  });",
    "});",

    "colorKeys.forEach(function(k){",
    "  var sec=document.getElementById(k);",
    "  sec.addEventListener('click',function(e){",
    "    var b=e.target.closest('button.swatch');if(!b)return;",
    "    current[k]=parseInt(b.getAttribute('data-v'),10);paint();",
    "  });",
    "  var picker=sec.querySelector('input[type=color]');",
    "  picker.addEventListener('input',function(e){",
    "    current[k]=parseInt(e.target.value.slice(1),16);paint();",
    "  });",
    "});",

    "paint();",

    "function save(){location.href='pebblejs://close#'+encodeURIComponent(JSON.stringify(current));}",
    "function cancel(){location.href='pebblejs://close#';}",
    "</script></body></html>"
  ].join("");
  return html;
}

function colorRow(key, presets) {
  var parts = ['<section class="colorgrid" id="' + key + '">'];
  for (var i = 0; i < presets.length; i++) {
    parts.push('<button class="swatch" data-v="' + presets[i] + '" style="background:' + toHex(presets[i]) + '"></button>');
  }
  parts.push('<label class="picker"><input type="color" value="#000000"><span>#000000</span></label>');
  parts.push("</section>");
  return parts.join("");
}

function optRow(key, labels) {
  var parts = ['<section class="grid" id="' + key + '">'];
  for (var i = 0; i < labels.length; i++) {
    parts.push('<button class="opt" data-v="' + i + '">' + labels[i] + "</button>");
  }
  parts.push("</section>");
  return parts.join("");
}

function readStored() {
  try {
    var raw = localStorage.getItem("config");
    if (!raw) return cloneDefaults();
    var parsed = JSON.parse(raw);
    var out = cloneDefaults();
    Object.keys(out).forEach(function(k) {
      if (typeof parsed[k] === "number") out[k] = parsed[k];
    });
    return out;
  } catch (e) {
    return cloneDefaults();
  }
}

function cloneDefaults() {
  var out = {};
  Object.keys(DEFAULTS).forEach(function(k) { out[k] = DEFAULTS[k]; });
  return out;
}

Pebble.addEventListener("ready", function() {
  console.log("JS ready");
});

Pebble.addEventListener("showConfiguration", function() {
  var current = readStored();
  var html = buildConfigPage(current);
  Pebble.openURL("data:text/html;charset=utf-8," + encodeURIComponent(html));
});

Pebble.addEventListener("webviewclosed", function(e) {
  if (!e.response) return;
  try {
    var config = JSON.parse(decodeURIComponent(e.response));
    localStorage.setItem("config", JSON.stringify(config));
    Pebble.sendAppMessage(
      {
        TIME_COLOR: config.timeColor,
        ASCII_COLOR: config.asciiColor,
        FACE_BG_COLOR: config.faceBgColor,
        DENSITY_INDEX: config.densityIndex,
        CELL_SIZE_INDEX: config.cellSizeIndex,
        TOUCH_INTENSITY: config.touchIntensity,
        TOUCH_DURATION: config.touchDuration,
        AMBIENT_TIMEOUT: config.ambientTimeout
      },
      function() { console.log("Settings sent"); },
      function(err) { console.log("Settings error: " + JSON.stringify(err)); }
    );
  } catch (ex) {
    console.log("Config parse error: " + ex);
  }
});
