var statusElement = document.getElementById('status');
var progressElement = document.getElementById('progress');
var spinnerElement = document.getElementById('spinner');

var Module = {
  preRun: [],
  postRun: [],

  print: (function() {
    var element = document.getElementById('output');
    if (element) element.value = ''; // clear browser cache
    return function(text) {
      if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
      // These replacements are necessary if you render to raw HTML
      //text = text.replace(/&/g, "&amp;");
      //text = text.replace(/</g, "&lt;");
      //text = text.replace(/>/g, "&gt;");
      //text = text.replace('\n', '<br>', 'g');
      console.log(text);
      if (element) {
        element.value += text + "\n";
        element.scrollTop = element.scrollHeight; // focus on bottom
      }
    };
  })(),

  canvas: (function() {
    var canvas = document.getElementById('canvas');

    // As a default initial behavior, pop up an alert when webgl context is lost. To make your
    // application robust, you may want to override this behavior before shipping!
    // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
    canvas.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);

    return canvas;
  })(),

  setStatus: function(text) {
    if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
    if (text === Module.setStatus.last.text) return;
    var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    var now = Date.now();
    if (m && now - Module.setStatus.last.time < 30) return; // if this is a progress update, skip it if too soon
    Module.setStatus.last.time = now;
    Module.setStatus.last.text = text;
    if (m) {
      text = m[1];
      progressElement.value = parseInt(m[2])*100;
      progressElement.max = parseInt(m[4])*100;
      progressElement.hidden = false;
      spinnerElement.hidden = false;
    } else {
      progressElement.value = null;
      progressElement.max = null;
      progressElement.hidden = true;
      if (!text) spinnerElement.style.display = 'none';
    }
    statusElement.innerHTML = text;
  },

  totalDependencies: 0,

  monitorRunDependencies: function(left) {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
  }

};


let searchParams = new URLSearchParams(location.search);
let machineConfig = null;

let fps = 0;
if (searchParams.has('fixedfps')) {
  fps = searchParams.get('fixedfps');
  if (fps != null) {
    fps = parseInt(fps);
  } else {
    fps = 60;
  }
  console.log('UI: Fixing frame rate to ' + fps + ' FPS');
}


if (searchParams.has('disc')) {
  Module.postRun.push(function() {
    let discUrl = searchParams.get('disc')
    console.log('UI: postRun - load disc URL ' + discUrl);
    loadDisc(discUrl);
  });
}

if (searchParams.has('ff')) {
  Module.postRun.push(function() {
    let ff_ms = parseInt(searchParams.get('ff'));
    console.log(`UI: postRun - fast forward to ${ff_ms} ms`);
    ccall('arc_fast_forward', null, ['number'], [ff_ms]);
  });
}

if (searchParams.has('autoboot')) {
  Module.preRun.push(function() {
    let autoboot = searchParams.get('autoboot');
    console.log('UI: preRun - create !boot:' + autoboot);
    FS.createDataFile('/hostfs', '!boot,feb', autoboot, true, true);
  }); 
  machineConfig = 'A3010_autoboot';
} else if (searchParams.has('basic')) {
  let prog = searchParams.get('basic');
  
  
  Module.preRun.push(function() {
    ENV.SDL_EMSCRIPTEN_KEYBOARD_ELEMENT ="#canvas";
    console.log('UI: preRun - create !boot and prog');
    FS.createDataFile('/hostfs', '!boot,ffe', 'KEY1 *!boot |M\r\nbasic\r\n' + prog + "\r\nRUN\r\n", true, true);
  });
  Module.postRun.push(function() {
    //ccall('arc_fast_forward', null, ['number'], [6000]);
  });
  document.getElementById('prog-container').style.display = 'block';
  document.getElementById('prog').value = prog;
  machineConfig = 'A3010_autoboot';
}

Module.arguments = [fps.toString()];
if (machineConfig) {
  console.log('UI: Using machine config ' + machineConfig);
  Module.arguments.push(machineConfig);
}

Module.setStatus('Downloading...');

window.onerror = function(event) {
    // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
    Module.setStatus('Exception thrown, see JavaScript console');
    spinnerElement.style.display = 'none';
    Module.setStatus = function(text) {
    if (text) Module.printErr('[post-exception status] ' + text);
    };
};

let currentDiscFile = null;


// Disc image extensions that Arculator handles (see loaders struct in disc.c)
let validDiscExts = ['.ssd','.dsd','.adf','.adl', '.fdi', '.apd', '.hfe'];


async function loadDisc(url) {
    if (url == "") return;
    let response = await fetch(url, {mode:'cors'});
    let buf = await response.arrayBuffer();
    let data = new Uint8Array(buf);
    let discFilename = url.substr(url.lastIndexOf('/')+1);
    var preamble = new TextDecoder().decode(data.slice(0, 2));

    if (preamble == 'PK') {
      console.log("Extracting ZIP file to find disc image");
      let unzip = new JSUnzip();
      unzip.open(data);
      let zipDiscFile = null;
      for (const n in unzip.files) {
        for (let ext of validDiscExts) {
          if (n.toLowerCase().endsWith(ext))
            zipDiscFile = n;
        }
        if (zipDiscFile)
          break;
      }
      if (!zipDiscFile) {
        console.warn("ZIP file did not contain a disc image with a valid extension");
        return;
      }
      console.log("Extracting " + zipDiscFile);
      let result = unzip.readBinary(zipDiscFile);
      if (!result.status) {
        console.error("failed to extract file: " + result.error)
      }
      data = result.data;

      if (zipDiscFile.indexOf('/') >= 0)
        zipDiscFile = zipDiscFile.substr(zipDiscFile.lastIndexOf('/')+1);
      discFilename = zipDiscFile;
      
    }
    if (currentDiscFile) {
      ccall('arc_disc_eject', null, ['number'], [0]);
      FS.unlink(currentDiscFile);
    }
    currentDiscFile = discFilename;
    FS.createDataFile("/", currentDiscFile, data, true, true);
    ccall('arc_disc_change', null, ['number', 'string'], [0, '/' + currentDiscFile]);
}

function arc_set_display_mode(display_mode) {
  ccall('arc_set_display_mode', null, ['number'], display_mode);
}

function arc_set_dblscan(dbl_scan) {
  ccall('arc_set_dblscan', null, ['number'], dbl_scan);
}

function arc_enter_fullscreen() {
  ccall('arc_enter_fullscreen', null, []);
}

function arc_renderer_reset() {
  ccall('arc_renderer_reset', null, []);
}

function arc_do_reset() {
  ccall('arc_do_reset', null, []);
}

async function* makeTextFileLineIterator(fileURL) {
    const utf8Decoder = new TextDecoder('utf-8');
    const response = await fetch(fileURL);
    const reader = response.body.getReader();
    let { value: chunk, done: readerDone } = await reader.read();
    chunk = chunk ? utf8Decoder.decode(chunk) : '';

    const re = /\n|\r|\r\n/gm;
    let startIndex = 0;
    let result;

    for (;;) {
        let result = re.exec(chunk);
        if (!result) {
        if (readerDone) {
            break;
        }
        let remainder = chunk.substr(startIndex);
        ({ value: chunk, done: readerDone } = await reader.read());
        chunk = remainder + (chunk ? utf8Decoder.decode(chunk) : '');
        startIndex = re.lastIndex = 0;
        continue;
        }
        yield chunk.substring(startIndex, result.index);
        startIndex = re.lastIndex;
    }
    if (startIndex < chunk.length) {
        // last line didn't end in a newline char
        yield chunk.substr(startIndex);
    }
}

async function setupDiscPicker() {
    let dropdown = document.getElementById('discs');
    for await (let discUrl of makeTextFileLineIterator('disc_index.txt')) {
        let discName = discUrl.substr(discUrl.lastIndexOf('/')+1);
        dropdown.add(new Option(discName, discUrl));
    }
}

function simulateKeyEvent(eventType, keyCode, charCode) {
  var e = document.createEventObject ? document.createEventObject() : document.createEvent("Events");
  if (e.initEvent) e.initEvent(eventType, true, true);

  e.keyCode = keyCode;
  e.which = keyCode;
  e.charCode = charCode;

  // Dispatch directly to Emscripten's html5.h API (use this if page uses emscripten/html5.h event handling):
  if (typeof JSEvents !== 'undefined' && JSEvents.eventHandlers && JSEvents.eventHandlers.length > 0) {
    for(var i = 0; i < JSEvents.eventHandlers.length; ++i) {
      if ((JSEvents.eventHandlers[i].target == Module['canvas'] || JSEvents.eventHandlers[i].target == window)
       && JSEvents.eventHandlers[i].eventTypeString == eventType) {
         JSEvents.eventHandlers[i].handlerFunc(e);
      }
    }
  } else {
    // Dispatch to browser for real (use this if page uses SDL or something else for event handling):
    Module['canvas'].dispatchEvent ? Module['canvas'].dispatchEvent(e) : Module['canvas'].fireEvent("on" + eventType, e);
  }
}

function sendKeyCode(keycode) {
  simulateKeyEvent('keydown', keycode, 0);
  setTimeout(() => simulateKeyEvent('keyup', keycode, 0), 100);
}

function rerunProg() {
  sendKeyCode(KEY_ESCAPE);
  setTimeout(() => sendKeyCode(KEY_F1), 200);
  setTimeout(() => document.getElementById('prog').focus(), 200);
}

function runProgram() {
  let prog = document.getElementById('prog').value;
  FS.unlink('/hostfs/!boot,ffe');
  FS.createDataFile('/hostfs', '!boot,ffe', 'KEY1 *!boot |M\r\nbasic\r\n' + prog + "\r\nRUN\r\n", true, true);
  rerunProg();
}

KEY_ESCAPE = 27;
KEY_F1 = 112;

setupDiscPicker();

function setClipboard(text) {
  var type = "text/plain";
  var blob = new Blob([text], { type });
  var data = [new ClipboardItem({ [type]: blob })];

  navigator.clipboard.write(data).then(
      function () {
      /* success */
      },
      function () {
      /* failure */
      }
  );
}



function getBasicShareUrl() {
  let prog = document.getElementById('prog').value;
  return location.protocol + '//' + location.host + location.pathname + '?basic=' + encodeURIComponent(prog);
}
function showShareBox() {
  document.getElementById('sharebox').style.display = 'block';
  document.getElementById('shareurl').value = getBasicShareUrl();
}

function tweetProg() {
  let prog = document.getElementById('prog').value;
  let url = "https://twitter.com/intent/tweet?screen_name=ARM2bot&text=" + encodeURIComponent(prog);
  window.open(url);
  document.getElementById('sharebox').style.display = 'none';

}
function copyProgAsURL() {
  navigator.clipboard.writeText(getBasicShareUrl()).then(function() {
    console.log('clipboard write ok');
  }, function() {
    console.log('clipboard write failed');
  });
  document.getElementById('sharebox').style.display = 'none';
}