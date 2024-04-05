'use strict';
import { Component, h, html, render, useEffect, useState, useRef } from './preact/preact.min.js';

const MaxMetricsDataPoints = 50;

// This simple publish/subscribe is used to pass notifications that were
// received from the server, to all child components of the app.
var PubSub = (function () {
  var handlers = {}, id = 0;
  return {
    subscribe: function (fn) {
      handlers[id++] = fn;
    },
    unsubscribe: function (id) {
      delete handlers[id];
    },
    publish: function (data) {
      for (var k in handlers) handlers[k](data);
    }
  };
})();

const Nav = props => html`
<div style="background: #333; padding: 0.5em; color: #fff;">
  <div class="container d-flex">
    <div style="flex: 1 1 auto; display: flex; align-items: center;">
      <b>Your Product</b>
    </div>
    <div style="display: flex; align-items: center; flex: 0 0 auto; ">
      <span>Logged in as:</span>
      <span style="padding: 0 0.5em;"><img src="user.png" height="22" /></span>
      <span>${props.user}</span>
      <a class="btn" onclick=${props.logout}
        style="margin-left: 1em; font-size: 0.8em; background: #8aa;">logout</a>
    </div>
  </div>
</div>`;


const Hero = props => html`
<div class="section">
<div style="margin-top: 1em; background: #eee; padding: 1em; border-radius: 0.5em; color: #777; ">
  <h1 style="margin: 0.2em 0;">Interactive Device Dashboard</h1>

  <p>
  This device dashboard is developed using the modern and compact Preact framework,
  in order to fit on very small devices. This is
  a <a href="https://mongoose.ws/tutorials/http-server/">hybrid server</a> which
  provides both static and dynamic content.  Static files, like CSS/JS/HTML
  or images, are compiled into the server binary.

  This UI uses the REST API implemented by the device, which you can examine
  using  <code>curl</code> command-line utility:
  </p>

  <div><code>curl -u admin:pass0 localhost:8000/api/config/get</code> </div>
  <div><code>curl -u admin:pass0 localhost:8000/api/config/set -d 'pub=mg/topic'</code> </div>
  <div><code>curl -u admin:pass0 localhost:8000/api/message/send -d 'message=hello'</code> </div>

  <p>
  The device can send notifications to this dashboard at anytime. Notifications
  are sent over WebSocket at URI <code>/api/watch</code> as JSON strings: <code>{"name": "..", "data": ...}</code>
  <div>Try <code>wscat --auth user1:pass1 --connect ws://localhost:8000/api/watch</code></div>
  </p>
</div>
</div>`;

const Login = function (props) {
  const [user, setUser] = useState('');
  const [pass, setPass] = useState('');
  const login = ev =>
    fetch(
      '/api/login',
      { headers: { Authorization: 'Basic ' + btoa(user + ':' + pass) } })
      .then(r => r.json())
      .then(r => r && props.login(r))
      .catch(err => err);
  return html`
<div class="rounded border" style="max-width: 480px; margin: 0 auto; margin-top: 5em; background: #eee; ">
  <div style="padding: 2em; ">
    <h1 style="color: #666;">Device Dashboard Login </h1>
    <div style="margin: 0.5em 0;">
      <input type='text' placeholder='Name' style="width: 100%;"
        oninput=${ev => setUser(ev.target.value)} value=${user} />
    </div>
    <div style="margin: 0.5em 0;">
      <input type="password" placeholder="Password" style="width: 100%;"
        oninput=${ev => setPass(ev.target.value)} value=${pass}
        onchange=${login} />
    </div>
    <div style="margin: 1em 0;">
      <button class="btn" style="width: 100%; background: #8aa;"
        disabled=${!user || !pass} onclick=${login}> Login </button>
    </div>
    <div style="color: #777; margin-top: 2em;">
      Valid logins: admin:pass0, user1:pass1, user2:pass2
    </div>
  </div>
</div>`;
};


const Configuration = function (props) {
  const [url, setUrl] = useState(props.config.url || '');
  const [pub, setPub] = useState(props.config.pub || '');
  const [sub, setSub] = useState(props.config.sub || '');

  useEffect(() => {
    setUrl(props.config.url);
    setPub(props.config.pub);
    setSub(props.config.sub);
  }, [props.config]);

  const update = (name, val) => fetch('/api/config/set', {
    method: 'post',
    body: `${name}=${encodeURIComponent(val)}`
  }).catch(err => err);
  const updateurl = ev => update('url', url);
  const updatepub = ev => update('pub', pub);
  const updatesub = ev => update('sub', sub);

  // console.log(props, [url, pub, sub]);
  return html`
<div class="section">
  <h3 style="background: #c03434; color: #fff; padding: 0.4em;">
    Device Configuration</h3>
  <div style="margin: 0.5em 0; display: flex;">
    <span class="addon nowrap">MQTT server:</span>
    <input type="text" style="flex: 1 100%;"
          value=${url} onchange=${updateurl}
          oninput=${ev => setUrl(ev.target.value)} />
    <button class="btn" disabled=${!url} onclick=${updateurl}
      style="margin-left: 1em; background: #8aa;">Update</button>
  </div>
  <div style="margin: 0.5em 0; display: flex; ">
    <span class="addon nowrap">Subscribe topic:</span>
    <input type="text" style="flex: 1 100%;"
        value=${sub} onchange=${updatesub}
        oninput=${ev => setSub(ev.target.value)} />
    <button class="btn" disabled=${!sub} onclick=${updatesub}
      style="margin-left: 1em; background: #8aa;">Update</button>
  </div>
  <div style="margin: 0.5em 0; display: flex;">
    <span class="addon nowrap">Publish topic:</span>
    <input type="text" style="flex: 1 100%;"
          value=${pub} onchange=${updatepub}
          oninput=${ev => setPub(ev.target.value)} />
    <button class="btn" disabled=${!pub} onclick=${updatepub}
      style="margin-left: 1em; background: #8aa;">Update</button>
  </div>
  <div>
    You can use <a href="http://www.hivemq.com/demos/websocket-client/">
    HiveMQ Websocket web client</a> to send messages to this console.
  </div>
  <div class="msg">
    The device keeps a persistent connection to the configured MQTT server.
    Changes to this configuration are propagated to all dashboards: try
    changing them in this dashboard and observe changes in other opened
    dashboards.
  </div><div class="msg">
    Note: administrators can see this section and can change device
    configuration, whilst users cannot.
  </div>
</div>`;
};


const Message = m => html`<div style="margin: 0.5em 0;">
  <span class="qos">qos: ${m.message.qos} </span>
  <span class="topic">topic: ${m.message.topic} </span>
  <span class="data">data: ${m.message.data}</span>
</div>`;

const Messages = function (props) {
  const [messages, setMessages] = useState([]);
  const [txt, setTxt] = useState('');

  useEffect(() => {
    const id = PubSub.subscribe(function (msg) {
      if (msg.name == 'message') setMessages(x => x.concat([msg.data]));
    });
    return PubSub.unsubscribe(id);
  }, []);

  const sendmessage = ev => fetch('/api/message/send', {
    method: 'post',
    body: `message=${encodeURIComponent(txt)}`
  }).then(r => setTxt(''));
  const connstatus = props.config.connected ? 'connected' : 'disconnected';
  return html`
<div class="section">
  <h3 style="background: #30c040; color: #fff; padding: 0.4em;">MQTT messages</h3>
  <div>
    MQTT server status: <b>${connstatus}</b>
  </div>
  <div style="height: 10em; overflow: auto; padding: 0.5em; " class="border">
    ${messages.map(message => h(Message, { message }))}
  </div>
  <div style="margin: 0.5em 0; display: flex">
    <span class="addon nowrap">Publish message:</span>
    <input placeholder="type and press enter..." style="flex: 1 100%;"
      value=${txt} onchange=${sendmessage}
      oninput=${ev => setTxt(ev.target.value)} />
  </div>
  <div class="msg">
    The message gets passed to the device via REST. Then the device sends it to
    the MQTT server over MQTT. All MQTT messages on a subscribed topic
    received by the device, are propagated to this dashboard via /api/watch.
  </div>
</div>`;
};

// Expected arguments:
// data: timeseries, e.g. [ [1654361352, 19], [1654361353, 18], ... ]
// width, height, yticks, xticks, ymin, ymax, xmin, xmax
const SVG = function (props) {
  //            w
  //   +---------------------+
  //   |        h1           |
  //   |    +-----------+    |
  //   |    |           |    |  h
  //   | w1 |           | w2 |
  //   |    +-----------+    |
  //   |         h2          |
  //   +---------------------+
  //
  let w = props.width, h = props.height, w1 = 30, w2 = 0, h1 = 8, h2 = 18;
  let yticks = props.yticks || 4, xticks = props.xticks || 5;
  let data = props.data || [];
  let ymin = props.ymin || 0;
  // let ymax = props.ymax || Math.max.apply(null, data.map(p => p[1]));
  let ymax = props.ymax;
  let xmin = props.xmin || Math.min.apply(null, data.map(p => p[0]));
  let xmax = props.xmax || Math.max.apply(null, data.map(p => p[0]));

  // Y-axis tick lines and labels
  let yta = (new Array(yticks + 1)).fill(0).map((_, i) => i);  // indices
  let yti = i => h - h2 - (h - h1 - h2) * i / yticks;          // index's Y
  let ytv = i => (ymax - ymin) * i / yticks;
  let ytl = y => html`<line x1=${w1} y1=${y} x2=${w} y2=${y} class="tick"/>`;
  let ytt = (y, v) => html`<text x=0 y=${y + 5} class="label">${v}</text>`;

  // X-axis tick lines and labels
  let datefmt = unix => (new Date(unix * 1000)).toISOString().substr(14, 5);
  let xta = (new Array(xticks + 1)).fill(0).map((_, i) => i);  // indices
  let xti = i => w1 + (w - w1 - w2) * i / xticks;              // index's X
  let xtv = i => datefmt(xmin + (xmax - xmin) * i / xticks);
  let xtl = x => html`<path d="M ${x},${h1} L ${x},${h - h2}" class="tick"/>`;
  let xtt = (x, v) =>
    html`<text x=${x - 15} y=${h - 2} class="label">${v}</text>`;

  // Transform data points array into coordinate
  let dx = v => w1 + (v - xmin) / ((xmax - xmin) || 1) * (w - w1 - w2);
  let dy = v => h - h2 - (v - ymin) / ((ymax - ymin) || 1) * (h - h1 - h2);
  let dd = data.map(p => [Math.round(dx(p[0])), Math.round(dy(p[1]))]);
  let ddl = dd.length;
  // And plot the data as <path> element
  let begin0 = ddl ? `M ${dd[0][0]},${dd[0][1]}` : `M 0,0`;
  let begin = `M ${w1},${h - h2}`;  // Initial point
  let end = ddl ? `L ${dd[ddl - 1][0]},${h - h2}` : `L ${w1},${h - h2}`;
  let series = ddl ? dd.map(p => `L ${p[0]} ${p[1]}`) : [];

  return html`
<svg viewBox="0 0 ${w} ${h}">
  <style>
    .axis { stroke: #aaa; fill: none; }
    .label { stroke: #aaa; font-size: 13px; }
    .tick { stroke: #ccc; fill: none; stroke-dasharray: 5; }
    .seriesbg { stroke: none; fill: rgba(200,225,255, 0.25)}
    .series { stroke: #255a; fill: none; }
  </style>
  ${yta.map(i => ytl(yti(i)))}
  ${yta.map(i => ytt(yti(i), ytv(i)))}
  ${xta.map(i => xtl(xti(i)))}
  ${data.length ? xta.map(i => xtt(xti(i), xtv(i))) : ''}
  <path d="${begin} ${series.join(' ')} ${end}" class="seriesbg" />
  <path d="${begin0} ${series.join(' ')}" class="series" />
</svg>`;
};

const SVGx = function (props) {
  //            w
  //   +---------------------+
  //   |        h1           |
  //   |    +-----------+    |
  //   |    |           |    |  h
  //   | w1 |           | w2 |
  //   |    +-----------+    |
  //   |         h2          |
  //   +---------------------+
  //
  let w = props.width, h = props.height, w1 = 30, w2 = 0, h1 = 8, h2 = 18;
  let yticks = props.yticks || 4, xticks = props.xticks || 5;
  let data = props.data || [];
  let ymin = 0;
  // let ymax = props.ymax || Math.max.apply(null, data.map(p => p[1]));
  let ymax = 100;
  let xmin = props.xmin || Math.min.apply(null, data.map(p => p[0]));
  let xmax = props.xmax || Math.max.apply(null, data.map(p => p[0]));

  // Y-axis tick lines and labels
  let yta = (new Array(yticks + 1)).fill(0).map((_, i) => i);  // indices
  let yti = i => h - h2 - (h - h1 - h2) * i / yticks;          // index's Y
  let ytv = i => (ymax - ymin) * i / yticks;
  let ytl = y => html`<line x1=${w1} y1=${y} x2=${w} y2=${y} class="tick"/>`;
  let ytt = (y, v) => html`<text x=0 y=${y + 5} class="label">${v}</text>`;

  // X-axis tick lines and labels
  let datefmt = unix => (new Date(unix * 1000)).toISOString().substr(14, 5);
  let xta = (new Array(xticks + 1)).fill(0).map((_, i) => i);  // indices
  let xti = i => w1 + (w - w1 - w2) * i / xticks;              // index's X
  let xtv = i => datefmt(xmin + (xmax - xmin) * i / xticks);
  let xtl = x => html`<path d="M ${x},${h1} L ${x},${h - h2}" class="tick"/>`;
  let xtt = (x, v) =>
    html`<text x=${x - 15} y=${h - 2} class="label">${v}</text>`;

  // Transform data points array into coordinate
  let dx = v => w1 + (v - xmin) / ((xmax - xmin) || 1) * (w - w1 - w2);
  let dy = v => h - h2 - (v - ymin) / ((ymax - ymin) || 1) * (h - h1 - h2);
  let dd = data.map(p => [Math.round(dx(p[0])), Math.round(dy(p[1]))]);
  // let dd = data.map(p => [Math.round(p[0])-xmin+w1, Math.round(dy(p[1]))]);

  // let dd = data.map(p => [Math.round(p[0])-tmpp0+w1, Math.round(p[1])*(h2-h1)/100]);
  let ddl = dd.length;
  // And plot the data as <path> element
  let begin0 = ddl ? `M ${dd[0][0]},${dd[0][1]}` : `M 0,0`;
  let begin = `M ${w1},${h - h2}`;  // Initial point
  let end = ddl ? `L ${dd[ddl - 1][0]},${h - h2}` : `L ${w1},${h - h2}`;
  let series = ddl ? dd.map(p => `L ${p[0]} ${p[1]}`) : [];
  // console.log("series:",series.join(' '));
  // <path d="${begin} ${series.join(' ')} ${end}" class="seriesbg" />
  // <path d="${begin0} ${series.join(' ')}" class="series" />
  return html`
<svg viewBox="0 0 ${w} ${h}">
  <style>
    .axis { stroke: #aaa; fill: none; }
    .label { stroke: #aaa; font-size: 13px; }
    .tick { stroke: #ccc; fill: none; stroke-dasharray: 5; }
    .seriesbg { stroke: none; fill: rgba(200,225,255, 0.25)}
    .series { stroke: #255; fill: none; }
  </style>
  ${yta.map(i => ytl(yti(i)))}
  ${yta.map(i => ytt(yti(i), ytv(i)))}
  ${xta.map(i => xtl(xti(i)))}
  ${data.length ? xta.map(i => xtt(xti(i), xtv(i))) : ''}
  <path d="${begin} ${series.join(' ')} ${end}" class="seriesbg" />
  <path d="${begin0} ${series.join(' ')}" class="series" />
</svg>`;
};

const Chart = function (props) {
  const [data, setData] = useState([]);
  const [cpu, set_cpu] = useState([]);
  const [mem, set_mem] = useState([]);

  useEffect(() => {
    const id = PubSub.subscribe(function (msg) {
      if (msg.name != 'cpumem') return;
      set_cpu(x => x.concat([msg.cpu]).splice(-MaxMetricsDataPoints));
      set_mem(x => x.concat([msg.mem]).splice(-MaxMetricsDataPoints));

    });
    return PubSub.unsubscribe(id);
  }, []);

  let xmax = 0, missing = MaxMetricsDataPoints - cpu.length;
  if (missing > 0) xmax = Math.round(Date.now() / 1000) + missing;
  if (xmax = 0) xmax = cpu.length;
  if (props.page != "1") return ``;
  return html`
  <div class="item">
    <div class="section">
      <h3 style="background: #ec3; color: #fff; padding: 0.4em;">CPU and MEMERY</h3>
        <div style="overflow: auto; padding: 0.5em;" class="">
          <${SVG} height=100 width=600 ymin=0 ymax=100 xmax=${xmax} data=${cpu} />
          <${SVGx} height=100 width=600 ymin=0 ymax=100 xmax=${xmax} data=${mem} />
        </div>
      </div>
  </div>`;
};



var Nav_cnt = 0;
// const App_1 = function(props) {
const Navigation = function (props) {
  const [url, setUrl] = useState();

  const update = (name, val) => {
    fetch('/api/Navigation', {
      method: 'post',
      body: `${name}=${encodeURIComponent(val)}`
    }).catch(err => err);
  }
  // const updateurl = ()=> {setNavIndex(1); console.log("System Status");
  // }
  const [sysSta, set_sysSta] = useState("active");
  const [Comconfig, set_Comconfig] = useState("disactive");
  const [OcdConfig, set_OcdConfig] = useState("disactive");
  const [settingConfig, set_settingConfig] = useState("disactive");
  const [showWs, set_showWs] = useState("...");

  const click_sysSta = () => {
    update("page", "click_sysSta");
  };

  const click_Comconfig = () => {
    update("page", "click_Comconfig");
  };
  const click_OcdConfig = () => {
    update("page", "click_OcdConfig");
  };
  const click_settingConfig = () => {
    update("page", "click_settingConfig");
  }

  var imgPulseId = "imgPulseId";
  var pulseFile = "pulse3.gif";
  function updatePulse() {
    // console.log("updatePulse ---->");
    var e = document.getElementById(imgPulseId);
    e.src=pulseFile;
    // console.log("updatePulse gitttttt",e);
  };
  useEffect(() => {
    const id = PubSub.subscribe(function (msg) {
      // console.log("Navigation:",msg);
      if (msg.page) {
        if (msg.page == 'sysSta') {
          set_sysSta("active");
          set_Comconfig("disactive");
          set_OcdConfig("disactive");
          set_settingConfig("disactive");

        }
        else if (msg.page == 'Comconfig') {
          set_sysSta("disactive");
          set_Comconfig("active");
          set_OcdConfig("disactive");
          set_settingConfig("disactive");
        }
        else if (msg.page == 'OcdConfig') {
          set_sysSta("disactive");
          set_Comconfig("disactive");
          set_OcdConfig("active");
          set_settingConfig("disactive");
        }
        else if (msg.page == 'settingConfig') {
          set_sysSta("disactive");
          set_Comconfig("disactive");
          set_OcdConfig("disactive");
          set_settingConfig("active");
        }
        console.log("before run updatePulse function");
        updatePulse();
      }
      
      if (msg.interface) { //for heart beats, loop show text
        set_showWs(".");
        updatePulse();
      }
      if (msg.OCD0) {
        set_showWs("..");
        updatePulse();
      }
      if (msg.OCD1) {
        set_showWs("...");
        updatePulse();
      }
    });
    return PubSub.unsubscribe(id);
  }, []);
  Nav_cnt++;
  // console.log("Navigation function running...", Nav_cnt++);
  return html`
  
    <div class="top_navbar">
      <div class="hamburger">
          <div class="one"></div>
          <div class="two"></div>
          <div class="three"></div>
      </div>
      <div class="top_menu">
          <div class="logo">netOcd</div>

          <span style="padding: 0 0.5em;"><img id=${imgPulseId} src=${pulseFile} height="40" /></span> 
          
          <div style="display: flex; align-items: center; flex: 0 0 auto; ">
          <span>Logged in as:</span>
          <span style="padding: 0 0.5em;"><img src="user.png" height="22" /></span>
          <span>${props.user}</span>
          <a class="btn" onclick=${props.logout}
            style="margin-left: 1em; font-size: 0.8em; background: #8aa;">logout</a>
        </div>
      </div>
    </div>

    <div class="sidebar">
      <ul>
          <li><a href="#"  class=${sysSta} onclick=${click_sysSta} >
                  <span class="icon"><i class="fas fa-book"></i></span>
                  <span class="title">System Status</span></a></li>
          <li><a href="#" class=${Comconfig} onclick=${click_Comconfig}>
                  <span class="icon"><i class="fas fa-file-video"></i></span>
                  <span class="title">Comconfig</span>
              </a></li>
          <li><a href="#" class=${OcdConfig} onclick=${click_OcdConfig}>
                  <span class="icon"><i class="fas fa-volleyball-ball"></i></span>
                  <span class="title">Ocd</span>
              </a></li>
          <li><a href="#" class=${settingConfig} onclick=${click_settingConfig}>
                  <span class="icon"><i class="fas fa-blog"></i></span>
                  <span class="title">setting</span>
              </a></li>
      </ul>
    </div>
 
  `;
};

var sysStaViewCnt =0;
const sysStaView = function (props) {
  var titleColor = "#999999"
  var cancleColor = "#bfbfbf"
  var enabeColor = "#4dff4d"
  var disableColor = "#b3b3cc"
  const [ocd0C, set_ocd0C] = useState(disableColor);
  const [ocd1C, set_ocd1C] = useState(disableColor);
  const [uart0C, set_uart0C] = useState(disableColor);
  const [uart1C, set_uart1C] = useState(disableColor);
  const [rs4220C, set_rs4220C] = useState(disableColor);
  const [rs4221C, set_rs4221C] = useState(disableColor);
  const [rs4850C, set_rs4850C] = useState(disableColor);
  const [rs4851C, set_rs4851C] = useState(disableColor);
  const [can0C, set_can0C] = useState(disableColor);
  const [can1C, set_can1C] = useState(disableColor);

  const [ocd0T, set_ocd0T] = useState("");
  const [ocd1T, set_ocd1T] = useState("");
  const [uart0T, set_uart0T] = useState("");
  const [uart1T, set_uart1T] = useState("");
  const [rs4220T, set_rs4220T] = useState("");
  const [rs4221T, set_rs4221T] = useState("");
  const [rs4850T, set_rs4850T] = useState("");
  const [rs4851T, set_rs4851T] = useState("");
  const [can0T, set_can0T] = useState("");
  const [can1T, set_can1T] = useState("");

  
  useEffect(() => {
    const id = PubSub.subscribe(function (msg) {
      if (msg.interface) {
        if (msg.interface.UART0 && msg.interface.UART0.sta == "enable") {
          set_uart0C(enabeColor);
          set_uart0T(msg.interface.UART0.bps);
        } else {
          set_uart0C(disableColor);
          set_uart0T("");
        }
        //uart1
        if (msg.interface.UART1 && msg.interface.UART1.sta == "enable") {
          set_uart1C(enabeColor);
          set_uart1T(msg.interface.UART1.bps);
        } else {
          set_uart1C(disableColor);
          set_uart1T("");
        }
        //rs4220
        if (msg.interface.Rs4220 && msg.interface.Rs4220.sta == "enable") {
          set_rs4220C(enabeColor);
          set_rs4220T(msg.interface.Rs4220.bps);
        } else {
          set_rs4220C(disableColor);
          set_rs4220T("");
        }
        //rs4221
        if (msg.interface.Rs4221 && msg.interface.Rs4221.sta == "enable") {
          set_rs4221C(enabeColor);
          set_rs4221T(msg.interface.Rs4221.bps);
        } else {
          set_rs4221C(disableColor);
          set_rs4221T("");
        }
        //rs4850
        if (msg.interface.Rs4850 && msg.interface.Rs4850.sta == "enable") {
          set_rs4850C(enabeColor);
          set_rs4850T(msg.interface.Rs4850.bps);
        } else {
          set_rs4850C(disableColor);
          set_rs4850T("");
        }
        //rs4851
        if (msg.interface.Rs4851 && msg.interface.Rs4851.sta == "enable") {
          set_rs4851C(enabeColor);
          set_rs4851T(msg.interface.Rs4851.bps);
        } else {
          set_rs4851C(disableColor);
          set_rs4851T("");
        }
        //can0
        if (msg.interface.CAN0 && msg.interface.CAN0.sta == "enable") {
          set_can0C(enabeColor);
          set_can0T(msg.interface.CAN0.bps);
        } else {
          set_can0C(disableColor);
          set_can0T("");
        }
        //can1
        if (msg.interface.CAN1 && msg.interface.CAN1.sta == "enable") {
          set_can1C(enabeColor);
          set_can1T(msg.interface.CAN1.bps);
        } else {
          set_can1C(disableColor);
          set_can1T("");
        }
      }
      if (msg.OCD0) {
        if (msg.OCD0.sta == "enable") {
          set_ocd0C(enabeColor);
          set_ocd0T(msg.OCD0.cfg);
        } else {
          set_ocd0C(disableColor);
          set_ocd0T("");
        }
      }
      if (msg.OCD1) {
        if (msg.OCD1.sta == "enable") {
          set_ocd1C(enabeColor);
          set_ocd1T(msg.OCD1.cfg);
        } else {
          set_ocd1C(disableColor);
          set_ocd1T("");
        }
      }
      
    });
    return PubSub.unsubscribe(id);
  }, []);
  
  // console.log("systme status::::", ocd0T, ocd1T,sysStaViewCnt);
  if (props.page != "1") return ``;
  return html`
  <div class="item">
  <table>
  <tr>
  <th></th>  
    <th style="color:${titleColor}; width:30%;">OCD</th>
    <th style="color:${titleColor}; width:12%;">UART</th>
    <th style="color:${titleColor}; width:15%;">Rs422</th>
    <th style="color:${titleColor}; width:7%;">Rs485</th>
    <th style="color:${titleColor}; width:15%;">CAN</th>
    <th style="color:${titleColor}; width:12%;">POWEROUT</th>
    <th style="color:${titleColor}; width:7%;">POWERIN</th>
  </tr>
  <tr>
  	<td style="color:${titleColor};">1</td>
    <td style="border-radius: 10px; background-color:${ocd1C}; ">${ocd1T}</td> <!--ocd1-->
    <td style="border-radius: 10px; background-color:${uart1C}; ">${uart1T}</td> <!-- UART1-->
    <td style="border-radius: 10px; background-color:${rs4221C}; ">${rs4221T}</td> <!--rs4221-->
    <td style="border-radius: 10px; background-color:${rs4851C}; ">${rs4851T}</td> <!--rs4851-->
    <td style="border-radius: 10px; background-color:${can1C}; ">${can1T}</td> <!--can1-->
    <td style="border-radius: 10px; background-color:${cancleColor}; ">3V 5V</td> <!--powerout-->
    
  </tr>
  <tr>
  <td style="color:${titleColor};">0</td>
    <td style="border-radius: 10px; background-color:${ocd0C}; ">${ocd0T}</td> <!--ocd0-->
    <td style="border-radius: 10px; background-color:${uart0C}; ">${uart0T}</td> <!-- UART0-->
    <td style="border-radius: 10px; background-color:${rs4220C}; ">${rs4220T}</td> <!--rs4220-->
    <td style="border-radius: 10px; background-color:${rs4850C}; ">${rs4850T}</td> <!--rs4850-->
    <td style="border-radius: 10px; background-color:${can0C}; ">${can0T}</td> <!--can0-->
    <td style="border-radius: 10px; background-color:${cancleColor}; ">3V 5V</td> <!--powerout-->
    <td style="border-radius: 10px; background-color:${cancleColor}; ">9V</td> <!--powerin-->
  </tr>
</table>
</div>
  `;
};

const oneInterfaceConfig_bps = m => html`
<option value=${m.bps} >${m.bps}</option>
`;

const oneInterfaceConfig = props => {
  // "disabled" or others the button is disable, only "" the button is enble;
  const [enabledSta, set_enabledSta] = useState("disable");
  const [disabledSta, set_disabledSta] = useState("disable");
  const [bgroundColor, set_bgroundColor] = useState("#adbbe5");

  if (props.realstat.sta == "enable") {
    set_bgroundColor("adbbe5");
    set_enabledSta("disable");
    set_disabledSta("");
  } else if (props.realstat.sta == "disable") {
    set_bgroundColor("#a6a6a6");
    set_enabledSta("");
    set_disabledSta("disable");
  }
  // console.log("p:",props.inface, "r:",props.realstat.sta, "en:",enabledSta,"dis:",disabledSta,);
  // console.log("p:",props.inface,"bps:",props.realstat.bps,"sta",props.realstat.sta);
  const clickEnable = (action, bps) => {
    fetch('/api/interface', {
      method: 'post',
      // body: `${name}=${encodeURIComponent(bps)}&${props.inface}=${encodeURIComponent(bps)}&port=${props.port}`
      body: `interface=${props.inface}&bps=${encodeURIComponent(bps)}&port=${props.port}&action=${action}`
    }
    ).catch(err => err);
  };
  const getBspId = ev => {
    var e = document.getElementById(props.inface);
    var bps = e.value;
    console.log("enable get:", bps, enabledSta);
    clickEnable("enable", bps);
  };
  const getBspId_disable = ev => {
    var e = document.getElementById(props.inface);
    var bps = e.value;
    console.log("disable get:", bps, disabledSta);
    clickEnable("disable", bps);
  };

  var enableBtnId = props.inface + "enableButton";
  function dis_allButton() {
    console.log("oneInterfaceConfig->>>>>>", enableBtnId);
    var e = document.getElementById(enableBtnId);
    e.disabled = true;

  };
  useEffect(() => {
    const enBtnEle = document.getElementById(enableBtnId);

    enBtnEle.addEventListener("click", dis_allButton);
    console.log("interface COM BUTTON!!!!");
    return () => {
      enBtnEle.removeEventListener("click", dis_allButton);
    }
  }, []);
  console.log("interface com html ...");
  return html`
<div class="col col-5-5">
  <div class="item" style="background: ${bgroundColor}">
    <h3 style="background: #0DA0EE; color: #fff; padding: 0.4em;">${props.inface}  port:${props.port}</h3>
      <div style="margin: 0.5em 0; display: flex;">
      <span class="info" >Baudrate:</span>
      <select id=${props.inface} value=${props.realstat.bps}>
      ${props.bps.map(bps => h(oneInterfaceConfig_bps, { bps }))}
      </select>

      <button id=${enableBtnId} disabled=${enabledSta} class="btn" 
      style="margin-left: 1em; background: #9DEE0D;" onclick=${getBspId} >Enable</button>

      <button id=${props.inface + "disableButton"} disabled=${disabledSta} class="btn" 
      style="margin-left: 1em; background: #8aa;" onclick=${getBspId_disable} >Disable</button>
      </div>
  </div>
</div>
`};

const ttyBaudRate = ["4800", "9600", "19200", "57600", "115200"];
const canBaudRate = ["125k", "250k", "500k", "750k", "1000k"];

const ComconfigPage = function (props) {
  //if there is no cnt, oneInterfaceConfig can't run
  const [UART0, set_uart0] = useState({ bps: "4800", sta: "disable" });
  const [UART1, set_uart1] = useState({ bps: "4800", sta: "disable" });
  const [Rs4220, set_Rs4220] = useState({ bps: "4800", sta: "disable" });
  const [Rs4221, set_Rs4221] = useState({ bps: "4800", sta: "disable" });
  const [Rs4850, set_Rs4850] = useState({ bps: "4800", sta: "disable" });
  const [Rs4851, set_Rs4851] = useState({ bps: "4800", sta: "disable" });
  const [can0, set_can0] = useState({ bps: "125k", sta: "disable" });
  const [can1, set_can1] = useState({ bps: "125k", sta: "disable" });

  //console.log("in ComconfigPage State ", UART0, UART1, Rs4220, Rs4221, Rs4850, Rs4851);
  useEffect(() => {
    const id = PubSub.subscribe(function (msg) {
      //  console.log("ComconfigPage:",msg);
      if (msg.interface) {
        // console.log("ComconfigPage:",msg.interface);
        // console.log("ComconfigPage:",msg.interface.UART0);
        if (msg.interface.UART0) {
          set_uart0({ sta: msg.interface.UART0.sta, bps: msg.interface.UART0.bps });
          // set_uart0({sta:msg.interface.UART0.sta});
          //console.log(msg.interface.UART0.bps);
          //console.log("useState:uart0: ",uart0);
        }
        if (msg.interface.UART1) {
          set_uart1({ bps: msg.interface.UART1.bps, sta: msg.interface.UART1.sta });
          // set_uart1({sta:msg.interface.UART1.sta});
          // console.log("get uart0 sta");
          //console.log("useState:uart0: ",uart0);
        }
        if (msg.interface.Rs4220) {
          set_Rs4220({ bps: msg.interface.Rs4220.bps, sta: msg.interface.Rs4220.sta });
          // set_Rs4220({sta:msg.interface.Rs4220.sta});
          // console.log("get uart0 sta");
          //console.log("useState:uart0: ",uart0);
        }
        if (msg.interface.Rs4221) {
          set_Rs4221({ bps: msg.interface.Rs4221.bps, sta: msg.interface.Rs4221.sta });
          // set_Rs4221({sta:msg.interface.Rs4221.sta});
          // console.log("get uart0 sta");
          //console.log("useState:uart0: ",uart0);
        }
        if (msg.interface.Rs4850) {
          set_Rs4850({ bps: msg.interface.Rs4850.bps, sta: msg.interface.Rs4850.sta });
          // set_Rs4850({sta:msg.interface.Rs4850.sta});
          // console.log("get uart0 sta");
          //console.log("useState:uart0: ",uart0);
        }
        if (msg.interface.Rs4851) {
          set_Rs4851({ bps: msg.interface.Rs4851.bps, sta: msg.interface.Rs4851.sta });
          // set_Rs4851({sta:msg.interface.Rs4851.sta});
          // console.log("get uart0 sta");
          //console.log("useState:uart0: ",uart0);
        }
        if (msg.interface.CAN0) {
          set_can0({ bps: msg.interface.CAN0.bps, sta: msg.interface.CAN0.sta });
        }
        if (msg.interface.CAN1) {
          set_can1({ bps: msg.interface.CAN1.bps, sta: msg.interface.CAN1.sta });
        }
      }

    });
    return PubSub.unsubscribe(id);
  }, []);

  var cnt = 0;
  if (props.page != 2) return ``;
  return html`
    <div class="row">
    <${oneInterfaceConfig} inface="UART0" port=${6100} bps=${ttyBaudRate} realstat=${UART0}/> 
    <div class="col col-0"></div>
    <${oneInterfaceConfig} inface="UART1" port=${6101} bps=${ttyBaudRate} realstat=${UART1}/>
    </div>

    <div class="row">
    <${oneInterfaceConfig} inface="Rs4220" port=${6200} bps=${ttyBaudRate} realstat=${Rs4220}/> 
    <div class="col col-0"></div>
    <${oneInterfaceConfig} inface="Rs4221" port=${6201} bps=${ttyBaudRate} realstat=${Rs4221}/>
    </div>

    <div class="row">
    <${oneInterfaceConfig} inface="Rs4850" port=${6300} bps=${ttyBaudRate} realstat=${Rs4850}/> 
    <div class="col col-0"></div>
    <${oneInterfaceConfig} inface="Rs4851" port=${6301} bps=${ttyBaudRate} realstat=${Rs4851}/>
    </div>

    <div class="row">
    <${oneInterfaceConfig} inface="CAN0" port=${6400} bps=${canBaudRate} realstat=${can0}/> 
    <div class="col col-0"></div>
    <${oneInterfaceConfig} inface="CAN1" port=${6401} bps=${canBaudRate} realstat=${can1}/>
    </div>    

`;
};

const oneOcdCfg = m => html`
<option value=${m.cfg} >${m.cfg}</option>`;

const showLonLine = m => {
  if (m.log.Info) {
    return html`<div style="margin: 0.3em 0;">
  <span class="logInfo" >${m.log.Info} </span>
  </div>`;
  }
  else if (m.log.Error) {
    return html`<div style="margin: 0.3em 0;">
  <span class="logError">${m.log.Error} </span>
  </div>`;
  } else if (m.log.Warn) {
    return html`<div style="margin: 0.3em 0;">
  <span class="logWarn">${m.log.Warn} </span>
  </div>`
  } else {
    return html`<div style="margin: 0.3em 0;">
    <span class="logOther">${m.log.Other} </span>
    </div>`;
  }
  ;
}

// inface="OCD0" port=${6400} cfgList=${cfgList} cfg=${ocd0cfg} sta=${ocd0Status} log=${ocd0Log}/> 
const oneOcdConfig = props => {
  const [bgroundColor, set_bgroundColor] = useState("#adbbe5");
  const [enabledSta, set_enableSta] = useState("");
  const [disabledSta, set_disabledSta] = useState("");

  //const [slectCfg, set_slectCfg] = useState("");
  // console.log(props.inface,"log0", log);
  if (props.sta == 'disable') {
    set_enableSta('');
    set_disabledSta("disable");
    set_bgroundColor("#a6a6a6");
  } else if (props.sta == 'enable') {
    set_enableSta('disable');
    set_disabledSta("");
    set_bgroundColor("#adbbe5");
    // set_log(x=>x.concat([props.log]));
  } else { //halt status 
    set_enableSta('disable');
    set_disabledSta("disable");
  }
  var useTip = "not working";
  if (props.sta == 'enable') {
    if (props.inface == "OCD0") {
      useTip = "telnet:4440 gdb:3330"
    } else if (props.inface == "OCD1") {
      useTip = "telnet:4450 gdb:3340"
    }
  }


  const pushMsg = (action, cfg) => {
    fetch('/api/ocd', {
      method: 'post',
      body: `ocd=${props.inface}&cfg=${cfg}&action=${action}`
    }
    ).catch(err => err);
  }



  const enableButton = ev => {
    var e = document.getElementById(props.inface);
    var cfgName = e.value;
    //console.log(cfgName);
    pushMsg("start", cfgName);
  }
  const disableButton = ev => {
    var e = document.getElementById(props.inface);
    var cfgName = e.value;
    //console.log(cfgName);
    pushMsg("stop", cfgName);
  }

  var enableBtnId = props.inface + "enableButton";
  function dis_allButton() {
    console.log("oneOCDOCDConfig->>>>>>", enableBtnId);
    var e = document.getElementById(enableBtnId);
    e.disabled = true;

  };
  useEffect(() => {
    const enBtnEle = document.getElementById(enableBtnId);
    enBtnEle.addEventListener("click", dis_allButton);
    console.log("interface ocdocd BUTTON!!!!");
    return () => {
      enBtnEle.removeEventListener("click", dis_allButton);
    }
  }, []);

  var disableBtnId = props.inface + "disableButton";
  function dis_disableButton() {
    console.log("oneOCDOCDConfig->disable>>>>>", disableBtnId);
    var e = document.getElementById(disableBtnId);
    e.disabled = true;
  };
  useEffect(() => {
    const enBtnEle = document.getElementById(disableBtnId);
    enBtnEle.addEventListener("click", dis_disableButton);
    console.log("interface ocdocd disable BUTTON!!!!");
    return () => {
      enBtnEle.removeEventListener("click", dis_disableButton);
    }
  }, []);

  var logId = props.inface + "log";
  function scrollLog() {
    console.log("scrollLog function is run ...");
    var e = document.getElementById(logId);
    e.scrollTop += 100;
  }
  useEffect(() => {
    const logEle = document.getElementById(logId);
    //this "DOMNodeInserted" is very important, I find one day
    logEle.addEventListener("DOMNodeInserted", scrollLog);
    return () => {
      logEle.removeEventListener("DOMNodeInserted", scrollLog);
    }
  }, [])
  return html`
  <div class="col col-5-5">
  <div class="item" style="background: ${bgroundColor}">
      <h3 style="background: #0DA0EE; color: #fff; padding: 0.4em;">${props.inface} ${useTip}</h3>
      <div style="margin: 0.5em 0; display: flex;">
      <select id=${props.inface} value=${props.cfg}>
      ${props.cfgList.map(cfg => h(oneOcdCfg, { cfg }))}
      </select>

      <button id=${enableBtnId} disabled=${enabledSta} class="btn" 
      style="margin-left: 1em; background: #9DEE0D;" onclick=${enableButton} >Enable</button>

      <button id=${disableBtnId} disabled=${disabledSta} class="btn" 
      style="margin-left: 1em; background: #8aa;"  onclick=${disableButton}>Disable</button>
      </div>

      <div id=${logId} style="height: 30em; overflow: auto; padding: 0.5em; " class="border">
          ${props.log.map(log => h(showLonLine, { log }))}
      </div>
  </div>
  </div>
  `;
};
// ${log.map(log => h(showLonLine, {log}))}
const oneResetButton = props => {
  const [bgroundColor, set_bgroundColor] = useState("#adbbe5");
  const resetPinExec = (action) =>{
    fetch('/api/trstExec', {
    method: 'post',
    body: `&resetPin=${props.resetPin}&action=${action}`
    }).catch(err => err);
    console.log("oneResetButton click oneResetButton",props.resetPin);
  };
  console.log("oneResetButton run ...");
  return html`
  <div class="col col-5-5">
  <div class="item" style="background: ${bgroundColor}">
    <h3 style="background: #0DA0EE; color: #fff; padding: 0.4em;">${props.resetPin} pin keep low for one second    
      <button  disabled= class="btn" 
      style="margin-left: 1em; background: #9DEE0D;" onclick=${resetPinExec} >exec</button>
     </h3>
  </div>
  </div>
  `
};

const OcdConfigPage = function (props) {
  const [cfgList, set_cfgList] = useState([]);
  const [ocd0Log, set_ocd0Log] = useState([]);
  const [ocd1Log, set_ocd1Log] = useState([]);

  const [ocd0Status, set_ocd0Status] = useState("disable");
  const [ocd1Status, set_ocd1Status] = useState("disable");

  const [ocd0cfg, set_ocd0cfg] = useState();
  const [ocd1cfg, set_ocd1cfg] = useState();

  useEffect(() => {
    const id = PubSub.subscribe(function (msg) {
      if (msg.cfgConfig) {
        set_cfgList(msg.list);
        //console.log("OcdConfigPage:subscribe:", msg.list);
      }
      if (msg.OCD0) {
        if (msg.OCD0.sta == 'disable') {
          set_ocd0Status("disable");
        } else if (msg.OCD0.sta == 'enable') {
          set_ocd0Status("enable");
          set_ocd0cfg(msg.OCD0.cfg);
        } else {
          set_ocd0Status(msg.OCD0.sta); //for halt sta
        }
        if (msg.OCD0.log.length > 0) {
          set_ocd0Log(x => x.concat(msg.OCD0.log));
        }
      }

      if (msg.OCD1) {
        if (msg.OCD1.sta == 'disable') {
          set_ocd1Status("disable");
        } else if (msg.OCD1.sta == 'enable') {
          set_ocd1Status("enable");
          set_ocd1cfg(msg.OCD1.cfg);
        } else {
          set_ocd1Status(msg.OCD1.sta); //for halt sta
        }
        if (msg.OCD1.log.length > 0) {
          set_ocd1Log(x => x.concat(msg.OCD1.log));
        }
      }
    });
    return PubSub.unsubscribe(id);
  }, []);
  // console.log("OcdConfigPage:", cfgList);
  if (props.page != 3) return ``;
  if (cfgList.length == 0) {
    console.log("cfgList is null");
    return '';
  }

  return html`
  <div class="row">
    <${oneOcdConfig} inface="OCD0" port=${6400} cfgList=${cfgList} cfg=${ocd0cfg} sta=${ocd0Status} log=${ocd0Log}/> 
    <div class="col col-0"></div>
    <${oneOcdConfig} inface="OCD1" port=${6401} cfgList=${cfgList} cfg=${ocd1cfg} sta=${ocd1Status} log=${ocd1Log}/>
    </div> 
    <div class="row">
    <${oneResetButton} resetPin="trst0" />
    <div class="col col-0"></div>
    <${oneResetButton} resetPin="trst1" />
    </div>
  `;
};

const SettingPage = function (props) {
  var l = window.location;
  var hostIp = l.host.split(':')[0];
  if (props.page != 4) return ``;
  return html `
  <div class="row">
    <div class="col col-5-5">
    <div class="item" >
    <div style="margin-top: 1em; background: #eee; padding: 1em; border-radius: 0.5em; color: #777; ">
    <h1 style="margin: 0.2em 0;">Interface usage:</h1>
      
      <p>This device maps both UART RS422 RS485 and CAN interface to <code>${hostIp}</code> socket port.</p>
      <p>You can access data via these ports through tools such as nc putty etc or programs written by yourself.</p>
      <p>For example, you can send or recieve data via UART0 by following command</p>
      <code><div>
        nc ${hostIp} 6100
      </div></code>
      <p>and so on ...</p>
    </div>
    </div>
    </div>
    <div class="col col-0"></div>
    <div class="col col-5-5">
    <div class="item" >
    <div style="margin-top: 1em; background: #eee; padding: 1em; border-radius: 0.5em; color: #777; ">
    <h1 style="margin: 0.2em 0;">OCD usage:</h1>
      
      <p>This device provides two jtag interfaces. They were named OCD0 and OCD1, and provide TCK TMS TDO and TDI signal</p>
      <p>You can control your targets via telnet, for example </p>
      <div>
      <code>  telnet ${hostIp} 4440 </code>
      <code>  telnet ${hostIp} 4441 </code>
      </div>
      <p>Commands include <code>targets halt resume</code> etc commonly used in telnet .</p>
      <p>And, this device provides gdb service for debugging the targets </p>
      <p>you can run cross-gdb to access your targets, for example</p>
      <div><code>
        <p> arm-linux-gdb your-program.elf -tui </p>
        <p> target extend ${hostIp}:3330/3331 </p>
      </code></div>
      <p>For more information and demos</p>
      <p> Please visit <a href="http://github.com/cahill123/netOcd" target="_blank">mannul</a> </p>
    </div>
    </div>
    </div>
  </div>
  `;
};

const App_1 = function (props) {
  const [user, setUser] = useState('');

  // Watch for notifications. As soon as a notification arrives, pass it on
  // to all subscribed components
  const watch = function () {
    var l = window.location, proto = l.protocol.replace('http', 'ws');
    var tid, wsURI = proto + '//' + l.host + '/api/watch'
    var reconnect = function () {
      var ws = new WebSocket(wsURI);
      // ws.onopen = () => console.log('ws connected');
      ws.onmessage = function (ev) {
        try {
          var msg = JSON.parse(ev.data);
          PubSub.publish(msg);
          // console.log(msg);
          // if (msg.name != 'metrics') console.log('ws->', msg);
        } catch (e) {
          //console.log('Invalid ws frame:', ev.data);  // eslint-disable-line
        }
      };
      ws.onclose = function () {
        clearTimeout(tid);
        tid = setTimeout(reconnect, 1000);
        console.log('ws disconnected');
      };
    };
    reconnect();
  };


  const login = function (u) {
    document.cookie = `access_token=${u.token}; Secure, HttpOnly; SameSite=Lax; path=/; max-age=3600`;
    setUser(u.user);
    watch();
    return getconfig();
  };

  const logout = ev => {
    console.log("logout ...");
    document.cookie = `access_token=; Secure, HttpOnly; SameSite=Lax; path=/; max-age=0`;
    setUser('');
  };
  const [pageIndex, set_pageIndex] = useState(1);
  useEffect(() => {
    const id = PubSub.subscribe(function (msg) {
      if (msg.page == 'sysSta') {
        set_pageIndex(1);
      }
      else if (msg.page == 'Comconfig') {
        set_pageIndex(2);
      }
      else if (msg.page == 'OcdConfig') {
        set_pageIndex(3);
      }
      else if (msg.page == 'settingConfig') {
        set_pageIndex(4);
      }
    });
  }, []);

  if (!user) return html`<${Login} login=${login} />`;
  return html`
    <div class="wrapper">
      <${Navigation} user=${user} logout=${logout}/>
      <div class="main_container">
        <${Chart} page=${pageIndex}/>
        <${sysStaView} page=${pageIndex}/>
        <${ComconfigPage} page=${pageIndex}/>
        <${OcdConfigPage} page=${pageIndex}/>
        <${SettingPage} page=${pageIndex}/>
      </div>
    </div>
    `;
};

window.onload = () => render(h(App_1), document.body);
