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
    loadDisk(discUrl);
  });
}

if (searchParams.has('autoboot')) {
  Module.preRun.push(function() {
    let autoboot = searchParams.get('autoboot');
    console.log('UI: preRun - create !Boot:' + autoboot);
    FS.createDataFile('/hostfs', '!boot,feb', autoboot, true, true);
  }); 
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

let currentDiskFile = null;


// Disc image extensions that Arculator handles
let validDiskExts = ['.ssd','.dsd','.adf','.adl', '.fdi', '.apd', '.hfe'];

async function loadDisk(url) {
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
        for (let ext of validDiskExts) {
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
    if (currentDiskFile) {
      ccall('arc_disc_eject', null, ['number'], [0]);
      FS.unlink(currentDiskFile);
    }
    currentDiskFile = discFilename;
    FS.createDataFile("/", currentDiskFile, data, true, true);
    ccall('arc_disc_change', null, ['number', 'string'], [0, '/' + currentDiskFile]);
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

async function setupDisks() {
    let dropdown = document.getElementById('disks');
    for await (let diskUrl of makeTextFileLineIterator('disk_index.txt')) {
        let diskName = diskUrl.substr(diskUrl.lastIndexOf('/')+1);
        dropdown.add(new Option(diskName, diskUrl));
    }
}

setupDisks();