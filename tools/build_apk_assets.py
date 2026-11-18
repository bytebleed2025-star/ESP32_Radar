#!/usr/bin/env python3
"""Generate the Android WebView assets from web/index.html.

Injects a Bluetooth transport shim so the existing radar UI works over
Bluetooth Classic SPP instead of WebSocket/WiFi.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "web"
ASSETS = ROOT / "android_app" / "app" / "src" / "main" / "assets"

SHIM = """<script>
window.__LACAK_LIVE = true;
(function(){
  if (!window.AndroidBT) return;

  function BTWebSocket(url){
    this.url = url;
    this.readyState = 1;
    this.onopen = null;
    this.onmessage = null;
    this.onclose = null;
    this.onerror = null;
    window.__btWS = this;
    var self = this;
    setTimeout(function(){ if (self.onopen) self.onopen(); }, 60);
  }
  BTWebSocket.OPEN = 1;
  BTWebSocket.CLOSED = 3;
  BTWebSocket.prototype.send = function(d){
    try { AndroidBT.send(String(d)); } catch(e) {}
  };
  BTWebSocket.prototype.close = function(){
    this.readyState = 3;
    if (this.onclose) this.onclose();
  };
  window.WebSocket = BTWebSocket;

  window.__btDeliver = function(line){
    var ws = window.__btWS;
    if (ws && ws.onmessage) ws.onmessage({ data: line });
  };

  document.addEventListener("DOMContentLoaded", function(){
    var tabs = document.querySelectorAll(".tab");
    for (var i = 0; i < tabs.length; i++){
      if ((tabs[i].textContent || "").trim().toUpperCase() === "CONFIG"){
        tabs[i].style.display = "none";
      }
    }
    var cfg = document.getElementById("view-config");
    if (cfg) cfg.style.display = "none";

    var wifi = document.getElementById("pill-wifi");
    if (wifi){
      var label = wifi.querySelector("b");
      var value = wifi.querySelector("em");
      if (label) label.textContent = "BLUETOOTH";
      if (value) value.textContent = "SPP";
    }
  });
})();
</script>
"""


def main() -> int:
    html = (WEB / "index.html").read_text(encoding="utf-8")
    if "</head>" not in html:
        print("ERROR: </head> not found in web/index.html")
        return 1
    if "AndroidBT" in html:
        print("ERROR: web/index.html already contains a Bluetooth shim")
        return 1

    html = html.replace("</head>", SHIM + "\n</head>", 1)
    ASSETS.mkdir(parents=True, exist_ok=True)
    (ASSETS / "index.html").write_text(html, encoding="utf-8", newline="\n")

    for name in ("logo.jpg", "bg.jpg"):
        (ASSETS / name).write_bytes((WEB / name).read_bytes())

    print(f"Wrote {ASSETS / 'index.html'}")
    for name in ("logo.jpg", "bg.jpg"):
        print(f"Copied {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
