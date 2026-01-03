#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

const char *const VERSION = "1.12.2";

// --- HTML Content ---

// index_html removed - merged into dashboard_html

const char dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Device Dashboard</title>
  <style>
    * { box-sizing: border-box; }
    body { font-family: 'Segoe UI', sans-serif; background: #222; color: #fff; margin: 0; padding: 0; text-align: center; }
    
    /* Navigation */
    nav { background: #333; overflow: hidden; display: flex; justify-content: center; border-bottom: 2px solid #444; }
    nav button { background: inherit; border: none; outline: none; cursor: pointer; padding: 14px 20px; transition: 0.3s; font-size: 17px; color: #aaa; }
    nav button:hover { background: #444; color: #fff; }
    nav button.active { background: #444; color: #00dbde; border-bottom: 3px solid #00dbde; }

    /* Content */
    .tab-content { display: none; padding: 20px; animation: fadeEffect 0.5s; }
    @keyframes fadeEffect { from {opacity: 0;} to {opacity: 1;} }

    /* Cards */
    .card { background: #333; padding: 20px; border-radius: 10px; display: block; text-align: left; width: 90%; max-width: 400px; margin: 20px auto; box-shadow: 0 4px 8px rgba(0,0,0,0.3); }

    /* Stats */
    .stat { margin: 15px 0; border-bottom: 1px solid #444; padding-bottom: 5px; }
    .stat:last-child { border: none; }
    .label { color: #aaa; font-size: 0.9em; margin-bottom: 2px; }
    .value { font-size: 1.2em; font-weight: bold; color: #00dbde; }

    /* Buttons */
    .button-row { width: 90%; max-width: 400px; margin: 20px auto 0; display: flex; justify-content: space-between; }
    .action-btn { background: #007bff; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; width: 48%; }
    .action-btn:hover { background: #0056b3; }
    .reset { background: #dc3545; }
    .reset:hover { background: #a71d2a; }
    .restart { background: #28a745; }
    .restart:hover { background: #218838; }

    /* Icons */
    .wifi-icon { position: relative; display: inline-block; width: 30px; height: 30px; margin-left: 10px; vertical-align: middle; }
    .bar { position: absolute; bottom: 0; width: 6px; background: #555; border-radius: 2px; transition: background 0.3s; }
    .bar-1 { left: 0; height: 6px; }
    .bar-2 { left: 8px; height: 14px; }
    .bar-3 { left: 16px; height: 22px; }
    .bar-4 { left: 24px; height: 30px; }
    .signal-1 .bar-1 { background: #00dbde; }
    .signal-2 .bar-1, .signal-2 .bar-2 { background: #00dbde; }
    .signal-3 .bar-1, .signal-3 .bar-2, .signal-3 .bar-3 { background: #00dbde; }
    .signal-4 .bar-1, .signal-4 .bar-2, .signal-4 .bar-3, .signal-4 .bar-4 { background: #00dbde; }
    .flex-row { display: flex; align-items: center; }
    
    .nav-batt { display: flex; align-items: center; margin-left: 10px; font-size: 0.9em; font-weight: bold; }
    .batt-container { width: 34px; height: 18px; border: 2px solid #fff; border-radius: 3px; position: relative; display: flex; align-items: center; justify-content: center; }
    .batt-fill { position: absolute; left: 0; top: 0; height: 100%; width: 0%; background: #27ae60; transition: width 0.3s, background 0.3s; z-index: 1; }
    .batt-cap { width: 3px; height: 8px; background: #fff; position: absolute; right: -5px; top: 3px; border-radius: 0 2px 2px 0; }
    .batt-text { position: relative; top: 1px; z-index: 2; font-size: 10px; color: #fff; font-weight: bold; text-shadow: 0 0 2px rgba(0,0,0,0.8); }
    .batt-low { background: #e67e22; }
    .batt-critical { background: #e74c3c; }

    /* AP Mode Specific */
    input { padding: 10px; border-radius: 5px; border: none; width: 100%; margin: 5px 0; color: #000; }
    #networks ul { list-style: none; padding: 0; }
    #networks li { display: flex; justify-content: space-between; align-items: center; background: #444; margin: 5px 0; padding: 10px; cursor: pointer; border-radius: 5px; }
    #networks li:hover { background: #555; }
    .wifi-bars { display: flex; align-items: flex-end; height: 16px; width: 24px; gap: 2px; }
    .wifi-bars div { background: #555; width: 4px; border-radius: 1px; }
    .wifi-bars .bar1 { height: 4px; }
    .wifi-bars .bar2 { height: 8px; }
    .wifi-bars .bar3 { height: 12px; }
    .wifi-bars .bar4 { height: 16px; }
    .wifi-bars div.active { background: #00dbde; }
  </style>
</head>
<body>

  <nav>
    <button class="tab-link active" onclick="openTab(event, 'Status')">Status</button>
    <button class="tab-link" onclick="openTab(event, 'Settings')">Settings</button>
    <button class="tab-link" onclick="openTab(event, 'About')">About</button>
    <div class="nav-batt">
      <div class="batt-container">
        <div id="nav-batt-fill" class="batt-fill"></div>
        <span id="nav-batt-val" class="batt-text">--%</span>
        <div class="batt-cap"></div>
      </div>
    </div>
  </nav>

  <!-- STATUS TAB -->
  <div id="Status" class="tab-content" style="display: block;">
    
    <!-- STATION STATUS (Visible in STA mode) -->
    <div id="station-status" style="display:none;">
      <div class="card">
        <div class="stat"><div class="label">Connected Network</div><div class="value">%SSID%</div></div>
        <div class="stat"><div class="label">IP Address</div><div class="value">%IP%</div></div>
        <div class="stat">
          <div class="label">Signal Strength</div>
          <div class="flex-row">
            <span class="value" id="rssi-val">%RSSI%</span> <span class="value"> dBm</span>
            <div id="wifi-icon" class="wifi-icon signal-0">
              <div class="bar bar-1"></div>
              <div class="bar bar-2"></div>
              <div class="bar bar-3"></div>
              <div class="bar bar-4"></div>
            </div>
          </div>
        </div>
        <div class="stat"><div class="label">Device MAC</div><div class="value">%MAC%</div></div>
        <div class="stat">
          <div class="label">Battery Voltage</div>
          <div class="value"><span id="batt-volt">--</span> V</div>
        </div>
      </div>
      <div class="button-row">
        <button class="action-btn restart" onclick="restartDev()">Restart</button>
        <button class="action-btn reset" onclick="resetConfig()">Reset</button>
      </div>
    </div>

    <!-- AP CONFIG (Visible in AP mode) -->
    <div id="ap-config" style="display:none;">
      <div class="card">
        <h3>WiFi Configuration</h3>
        <div class="stat"><div class="label">AP IP Address</div><div class="value">%IP%</div></div>
        <div class="stat"><div class="label">Current SSID</div><div class="value" id="ap-status-ssid">--</div></div>
        <div class="stat"><div class="label">Current Password</div><div class="value" id="ap-status-pass">--</div></div>
        <button class="action-btn" onclick="scanNetworks()" style="width:100%; margin-bottom:10px;">Scan Networks</button>
        <div id="networks"></div>
        <br>
        <input type="text" id="ssid" placeholder="Selected SSID">
        <input type="password" id="password" placeholder="WiFi Password">
        <button class="action-btn" onclick="saveConfig()" style="width:100%; margin-top:10px;">Save & Connect</button>
      </div>
      <div class="button-row">
        <button class="action-btn restart" onclick="restartDev()">Restart</button>
        <button class="action-btn reset" onclick="resetConfig()">Reset</button>
      </div>
    </div>

  </div>

  <!-- SETTINGS TAB -->
  <div id="Settings" class="tab-content">
    <div class="card" id="settings-ap-config" style="display:none;">
      <h3>AP Mode Config</h3>
      <input type="text" id="ap_ssid" placeholder="AP SSID (e.g. ODROID-GO)">
      <input type="password" id="ap_pass" placeholder="AP Password (min 8 chars)">
      <button class="action-btn" onclick="saveAP()" style="width:100%; margin-top:10px;">Save AP Settings</button>
    </div>

    <div class="card">
      <h3>Display Settings</h3>
      <div class="stat">
        <div class="label">Brightness</div>
        <div style="display:flex; align-items:center;">
          <input type="range" id="brightness" min="10" max="255" value="255" oninput="updateBrightness(this.value)" style="flex-grow:1; margin-right:10px;">
          <span id="brightness-val" class="value">100%</span>
        </div>
      </div>
      <button class="action-btn" onclick="saveDisplay()" style="width:100%; margin-top:10px;">Save Display</button>
    </div>

    <div class="card">
      <h3>Transmission Config</h3>
      <input type="text" id="t_host" placeholder="Host / IP">
      <input type="number" id="t_port" placeholder="Port (9091)">
      <input type="text" id="t_path" placeholder="Path (/transmission/rpc)">
      <input type="text" id="t_user" placeholder="Username">
      <input type="password" id="t_pass" placeholder="Password">

      <div style="display:flex; justify-content:space-between; margin-top:10px;">
        <button class="action-btn" onclick="saveTrans()" style="width:48%;">Save</button>
        <button class="action-btn" onclick="testTrans(this)" style="background:#e67e22; width:48%;">Test</button>
      </div>
      <p id="test_status" style="text-align:center; margin-top:10px; font-weight:bold;"></p>
    </div>

    <div class="card">
      <h3>Firmware Update</h3>
      <input type="file" id="firmware_file" accept=".bin" style="margin:10px 0; color:white;">
      <button id="upload_btn" class="action-btn" onclick="uploadFirmware()" style="width:100%; background:#8e44ad;">Upload Firmware</button>
    </div>
  </div>

  <!-- ABOUT TAB -->
  <div id="About" class="tab-content">
    <div class="card">
      <h3>About Device</h3>
      <div class="stat"><div class="label">Firmware Version</div><div class="value">%VERSION%</div></div>
      <div class="stat"><div class="label">Build Date</div><div class="value">%BUILD_DATE%</div></div>
      <div class="stat"><div class="label">Project</div><div class="value"><a href="https://github.com/Adamyno/odroid_transmission_monitor" target="_blank" style="color:#00dbde; text-decoration:none;">GitHub</a></div></div>
      <br>
      <p style="color:#aaa; font-size:0.9em;">Transmission Monitor for ODROID-GO<br>Created by Adamyno</p>
    </div>


  <script>
    const SYSTEM_MODE = "%MODE%"; // "AP" or "STA"

    function init() {
      console.log("Init Mode:", SYSTEM_MODE);
      try {
        var apStatus = document.getElementById('ap-config');
        var staStatus = document.getElementById('station-status');
        var apSettings = document.getElementById('settings-ap-config');

        if(SYSTEM_MODE === "AP") {
            // AP Mode: Show AP Status, Hide STA Status, Show AP Settings
            if(apStatus) apStatus.style.display = 'block';
            if(staStatus) staStatus.style.display = 'none';
            if(apSettings) apSettings.style.display = 'block';
        } else {
            // STA Mode: Hide AP Status, Show STA Status, Hide AP Settings
            if(apStatus) apStatus.style.display = 'none';
            if(staStatus) staStatus.style.display = 'block';
            if(apSettings) apSettings.style.display = 'none';
        }

        
        // Start updates for both modes
        setInterval(updateSignal, 2000);
        updateSignal();
        // Always update battery in both modes
        setInterval(updateBattery, 5000);
        updateBattery();
        loadTrans(); 
      } catch(e) {
        console.error("Init Error:", e);
      }
    }
    
    function updateBattery() {
      fetch('/status').then(function(r) { return r.json(); }).then(function(data) {
        var volt = data.batt || 0;
        var pct = Math.round((volt - 3.4) / (4.2 - 3.4) * 100);
        if(pct > 100) pct = 100;
        if(pct < 0) pct = 0;

        var battVal = document.getElementById('nav-batt-val');
        var battFill = document.getElementById('nav-batt-fill');

        if(battVal) battVal.innerText = pct + '%';
        if(battFill) {
            battFill.style.width = pct + '%';
            battFill.classList.remove('batt-low', 'batt-critical');
            if(pct < 10) battFill.classList.add('batt-critical');
            else if(pct < 25) battFill.classList.add('batt-low');
        }
      }).catch(function(e) { });
    }

    function openTab(evt, tabName) {
      var i, tabcontent, tablinks;
      tabcontent = document.getElementsByClassName("tab-content");
      for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
      }
      tablinks = document.getElementsByClassName("tab-link");
      for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
      }
      document.getElementById(tabName).style.display = "block";
      evt.currentTarget.className += " active";
    }

    // AP Mode Functions
    function scanNetworks() {
      var netDiv = document.getElementById('networks');
      netDiv.innerHTML = "Scanning...";
      fetch('/scan_wifi')
        .then(function(res) { return res.json(); })
        .then(function(data) {
          var html = "<ul>";
          data.forEach(function(net) {
            var bars = 1;
            if (net.rssi >= -60) bars = 4;
            else if (net.rssi >= -70) bars = 3;
            else if (net.rssi >= -80) bars = 2;
            
            var active1 = (bars>=1) ? 'active' : '';
            var active2 = (bars>=2) ? 'active' : '';
            var active3 = (bars>=3) ? 'active' : '';
            var active4 = (bars>=4) ? 'active' : '';

            var barsHtml = '<div class="wifi-bars">' +
              '<div class="bar1 ' + active1 + '"></div>' +
              '<div class="bar2 ' + active2 + '"></div>' +
              '<div class="bar3 ' + active3 + '"></div>' +
              '<div class="bar4 ' + active4 + '"></div>' +
            '</div>';
            
            // Clean SSID to prevent quote breaking
            var safeSSID = net.ssid.replace(/'/g, "\\'"); 
            
            html += '<li onclick="selectNetwork(\'' + safeSSID + '\')">' +
              '<span>' + net.ssid + '</span>' +
              barsHtml +
            '</li>';
          });
          html += "</ul>";
          netDiv.innerHTML = html;
        })
        .catch(function(e) { console.log(e); });
    }
    
    function selectNetwork(ssid) {
      document.getElementById('ssid').value = ssid;
      document.getElementById('password').focus();
    }

    function saveConfig() {
      var ssid = document.getElementById('ssid').value;
      var pass = document.getElementById('password').value;
      if(!ssid) return alert("SSID required!");
      
      var params = new URLSearchParams();
      params.append("ssid", ssid);
      params.append("password", pass);

      fetch('/save', { method: 'POST', body: params })
        .then(function(res) { return res.text(); })
        .then(function(msg) { alert(msg); setTimeout(function() { location.reload(); }, 2000); })
        .catch(function(e) { alert("Error saving"); });
    }

    // STA Mode & Common Functions
    function updateSignal() {
        fetch('/status').then(function(r) { return r.json(); }).then(function(data) {
            // Signal
            if(data.mode === "AP") {
                if(document.getElementById('ap-status-ssid')) document.getElementById('ap-status-ssid').innerText = data.ap_ssid;
                if(document.getElementById('ap-status-pass')) document.getElementById('ap-status-pass').innerText = (data.ap_password && data.ap_password.length > 0) ? data.ap_password : "(Open)";
            } else if(data.rssi) {
                var val = document.getElementById('rssi-val');
                var icon = document.getElementById('wifi-icon');
                if(val) val.innerText = data.rssi;
                if(icon) {
                    icon.className = 'wifi-icon';
                    if(data.rssi >= -60) icon.classList.add('signal-4');
                    else if(data.rssi >= -70) icon.classList.add('signal-3');
                    else if(data.rssi >= -80) icon.classList.add('signal-2');
                    else if(data.rssi >= -90) icon.classList.add('signal-1');
                    else icon.classList.add('signal-0');
                }
            }

            // Battery
            var volt = data.batt;
            var pct = Math.round((volt - 3.4) / (4.2 - 3.4) * 100);
            if(pct > 100) pct = 100;
            if(pct < 0) pct = 0;

            var battVal = document.getElementById('nav-batt-val');
            var battFill = document.getElementById('nav-batt-fill');
            var battVolt = document.getElementById('batt-volt');

            if(battVal) battVal.innerText = pct + '%';
            if(battVolt) battVolt.innerText = volt.toFixed(2);
            if(battFill) {
                battFill.style.width = pct + '%';
                battFill.classList.remove('batt-low', 'batt-critical');
                if(pct < 10) battFill.classList.add('batt-critical');
                else if(pct < 25) battFill.classList.add('batt-low');
            }
        }).catch(function(e) { console.log(e); });
    }

    function loadTrans() {
      fetch('/get_params').then(function(res) { return res.json(); }).then(function(data) {
        document.getElementById('t_host').value = data.host || "";
        document.getElementById('t_port').value = data.port || "9091";
        document.getElementById('t_path').value = data.path || "/transmission/rpc";
        document.getElementById('t_user').value = data.user || "";
        document.getElementById('t_pass').value = data.pass || "";
        
        if(document.getElementById('ap_ssid')) document.getElementById('ap_ssid').value = data.ap_ssid || "";
        if(document.getElementById('ap_pass')) document.getElementById('ap_pass').value = data.ap_password || "";
        
        if(document.getElementById('brightness')) {
            var br = data.brightness || 255;
            document.getElementById('brightness').value = br;
            var pct = Math.round((br / 255) * 100);
            document.getElementById('brightness-val').innerText = pct + '%';
        }
      }).catch(function(e) { console.log("No params loaded"); });
    }

    function updateBrightness(val) {
        var pct = Math.round((val / 255) * 100);
        document.getElementById('brightness-val').innerText = pct + '%';
    }

    function saveDisplay() {
      var btn = event.target;
      var oldText = btn.innerText;
      btn.innerText = "Saving...";
      btn.disabled = true;
      btn.style.opacity = "0.7";

      var params = new URLSearchParams();
      params.append("brightness", document.getElementById('brightness').value);

      fetch('/save_params', { method: 'POST', body: params })
        .then(function(res) { return res.text(); })
        .then(function(msg) {
          btn.innerText = msg;
          btn.style.background = "#27ae60";
          setTimeout(function() { 
            btn.innerText = oldText;
            btn.style.background = "";
            btn.disabled = false;
            btn.style.opacity = "1";
          }, 2000);
        })
        .catch(function(e) {
          btn.innerText = "Error!";
          btn.style.background = "#e74c3c";
          setTimeout(function() { 
            btn.innerText = oldText;
            btn.style.background = "";
            btn.disabled = false;
            btn.style.opacity = "1";
          }, 2000);
        });
    }

    function saveAP() {
      var btn = event.target;
      var oldText = btn.innerText;
      btn.innerText = "Saving...";
      btn.disabled = true;
      btn.style.opacity = "0.7";

      var params = new URLSearchParams();
      params.append("ap_ssid", document.getElementById('ap_ssid').value);
      params.append("ap_password", document.getElementById('ap_pass').value);

      fetch('/save_params', { method: 'POST', body: params })
        .then(function(res) { return res.text(); })
        .then(function(msg) {
          btn.innerText = msg;
          btn.style.background = "#27ae60";
          setTimeout(function() { 
            btn.innerText = oldText;
            btn.style.background = "";
            btn.disabled = false;
            btn.style.opacity = "1";
          }, 2000);
        })
        .catch(function(e) {
          btn.innerText = "Error!";
          btn.style.background = "#e74c3c";
          setTimeout(function() { 
            btn.innerText = oldText;
            btn.style.background = "";
            btn.disabled = false;
            btn.style.opacity = "1";
          }, 2000);
        });
    }

    function saveTrans() {
      var btn = event.target;
      var oldText = btn.innerText;
      btn.innerText = "Saving...";
      btn.disabled = true;
      btn.style.opacity = "0.7";

      var params = new URLSearchParams();
      params.append("host", document.getElementById('t_host').value);
      params.append("port", document.getElementById('t_port').value);
      params.append("path", document.getElementById('t_path').value);
      params.append("user", document.getElementById('t_user').value);
      params.append("pass", document.getElementById('t_pass').value);

      fetch('/save_params', { method: 'POST', body: params })
        .then(function(res) { return res.text(); })
        .then(function(msg) {
          btn.innerText = msg;
          btn.style.background = "#27ae60";
          setTimeout(function() { 
            btn.innerText = oldText;
            btn.style.background = "";
            btn.disabled = false;
            btn.style.opacity = "1";
          }, 2000);
        })
        .catch(function(e) {
          btn.innerText = "Error!";
          btn.style.background = "#e74c3c";
          setTimeout(function() { 
            btn.innerText = oldText;
            btn.style.background = "";
            btn.disabled = false;
            btn.style.opacity = "1";
          }, 2000);
        });
    }

    function testTrans(btn) {
      var oldText = btn.innerText;
      var status = document.getElementById('test_status');
      
      btn.innerText = "Testing...";
      btn.disabled = true;
      btn.style.opacity = "0.5";
      status.innerText = ""; 

      var params = new URLSearchParams();
      params.append("host", document.getElementById('t_host').value);
      params.append("port", document.getElementById('t_port').value);
      params.append("path", document.getElementById('t_path').value);
      params.append("user", document.getElementById('t_user').value);
      params.append("pass", document.getElementById('t_pass').value);

      fetch('/test_transmission', { method: 'POST', body: params })
        .then(function(res) { return res.text(); })
        .then(function(msg) {
          if(msg.includes("Success")) {
            status.style.color = "#2ecc71"; // Emerald Green
            status.innerText = msg;
          } else {
            status.style.color = "#e74c3c"; // Alizarin Red
            status.innerText = msg;
          }
        })
        .catch(function(e) {
            status.style.color = "#e74c3c";
            status.innerText = "Network Error";
        })
        .finally(function() {
          btn.innerText = oldText;
          btn.disabled = false;
          btn.style.opacity = "1";
        });
    }

    function uploadFirmware() {
      var fileInput = document.getElementById('firmware_file');
      if(fileInput.files.length === 0) return alert("Select file first!");
      
      var file = fileInput.files[0];
      var formData = new FormData();
      formData.append("update", file);

      var btn = document.getElementById('upload_btn');
      var oldText = btn.innerText;
      
      btn.disabled = true;
      btn.style.backgroundColor = '#2980b9'; // Blue color

      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/update', true);

      xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
          var percent = Math.round((e.loaded / e.total) * 100);
          btn.innerText = '[' + percent + '%] Uploading...';
        }
      };

      xhr.onload = function() {
        if (xhr.status === 200) {
          var msg = xhr.responseText;
          alert(msg);
          if (msg.includes("Success")) setTimeout(function() { location.reload(); }, 5000);
        } else {
          alert('Upload failed!');
        }
        btn.innerText = oldText;
        btn.disabled = false;
        btn.style.backgroundColor = '#8e44ad'; 
      };

      xhr.onerror = function() {
        alert('Network Error during upload');
        btn.innerText = oldText;
        btn.disabled = false;
        btn.style.backgroundColor = '#8e44ad';
      };

      xhr.send(formData);
    }

    function restartDev() {
      var btn = event.target;
      var oldText = btn.innerText;
      btn.innerText = "Restarting...";
      btn.disabled = true;
      btn.style.opacity = "0.7";
      fetch('/restart', { method: 'POST' })
        .then(function(res) { return res.text(); })
        .then(function(msg) {
          btn.innerText = msg;
          btn.style.background = "#27ae60";
          setTimeout(function() { location.reload(); }, 3000);
        })
        .catch(function(e) {
          btn.innerText = "Error!";
          btn.style.background = "#e74c3c";
        });
    }
    function resetConfig() {
      var btn = event.target;
      var oldText = btn.innerText;
      btn.innerText = "Resetting...";
      btn.disabled = true;
      btn.style.opacity = "0.7";
      fetch('/reset', { method: 'POST' })
        .then(function(res) { return res.text(); })
        .then(function(msg) {
          btn.innerText = msg;
          btn.style.background = "#27ae60";
          setTimeout(function() { location.reload(); }, 3000);
        })
        .catch(function(e) {
          btn.innerText = "Error!";
          btn.style.background = "#e74c3c";
        });
    }

    // Initialize
    window.onload = init;
  </script>
</body>
</html>
)rawliteral";

#endif
