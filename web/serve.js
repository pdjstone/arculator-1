/* Simple web server to rebuild and test Arculator's Emscripten functionality.
 * call with e.g. `node serve.js localhost 3020` to bind to localhost:3020.
 *
 * It will run a make command before serving any of the build files, 
 * and redirect you to the right place from the root.
 */
const { execSync } = require("child_process");
const http = require("http")
const fs = require("fs")

const addr = process.argv[2] || 'localhost'
const port = process.argv[3] || '3020'
const root = process.cwd()
const MAKE_WASM = "make -j8 DEBUG=1 wasm"

var mimeTypes = {
    '.html': 'text/html',
    '.js': 'text/javascript',
    '.wasm': 'application/wasm',
    '.css': 'text/css',
    '.png': 'image/png',
    '.jpg': 'image/jpg',
    '.ico': 'image/x-icon',
    '.svg': 'image/svg+xml',
    '.json': 'application/json',
    '.txt': 'text/plain',
    '.mp3': 'audio/mpeg',
    '.wav': 'audio/wav',
    '.zip': 'application/zip',
}

function serveFile(req, res) {
    var filename = root + req.url;
    console.log(filename);
    try {
        data = fs.readFileSync(filename);
        if (data) {
            var ext = filename.substring(filename.lastIndexOf("."));
            var mimeType = mimeTypes[ext] || "application/octet-stream";
            res.writeHead(200, { "Content-Type": mimeType });
            res.write(data);
        }
    } catch (ex) {
        if (ex.code == "ENOENT") {
            res.writeHead(404, { "Content-Type": "text/plain" });
            res.end("Not found");
        } else {
            throw ex;
        }
    }
    res.end();
}

function handle(req, res) {
    if (req.url == "/" || req.url.startsWith("/build/wasm/arculator.html")) {
        var now = new Date();
        try {
            var output = execSync(MAKE_WASM)
            // don't care about output if it worked
        }
        catch (error) {
            res.writeHead(500, { "Content-Type": "text/plain" });
            res.end(error.message);
            console.log(`error: ${error.message}`);
            return;
        }
        var duration = new Date() - now;
        console.log(`Build took ${duration}ms`)
    }

    if (req.url == "/") {
        res.writeHead(302, { Location: "/build/wasm/arculator.html" });
        res.end();
        return;
    }

    serveFile(req, res);
}

const server = http.createServer(function (req, res) {
    try {
        handle(req, res)
    }
    catch (ex) {
        console.log("Uncaught exception handling request for "+req.url+": "+ex)
        res.writeHead(500, { "Content-Type": "text/plain" });
        res.end("Internal server error");
    }
});

server.listen(port, addr, () => {
  console.log(`🌰 Arculator is available at http://${addr}:${port}`);
});
