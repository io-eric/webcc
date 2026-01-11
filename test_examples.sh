#!/bin/bash
set -e

# 1. Build the compiler
echo "Building WebCC compiler..."
./build.sh && WEBCC_REBUILT=0 || WEBCC_REBUILT=$?

# Exit code 2 means webcc was rebuilt - clear example caches to force recompile
if [ $WEBCC_REBUILT -eq 2 ]; then
    echo "WebCC was rebuilt, clearing example caches..."
    rm -rf examples/*/.webcc_cache
elif [ $WEBCC_REBUILT -ne 0 ]; then
    echo "WebCC build failed with exit code $WEBCC_REBUILT"
    exit $WEBCC_REBUILT
fi

# 2. Build all examples
EXAMPLES=(
    "webcc_audio"
    "webcc_canvas"
    "webcc_dom"
    "webcc_types"
    "webcc_webgl"
    "webcc_webgl_waves"
    "webcc_webgpu"
)

# Stats overlay HTML (minified)
STATS_HTML='<div id="stats" style="position:fixed;top:0;left:0;background:rgba(0,0,0,0.7);color:lime;font-family:monospace;padding:5px;pointer-events:none;z-index:9999;">Loading stats...</div><script>window.addEventListener("load",function(){const s=document.getElementById("stats");function f(b){return(b/1024).toFixed(1)+" KB"}Promise.all([fetch("app.wasm",{method:"HEAD"}).then(r=>parseInt(r.headers.get("Content-Length")||0)),fetch("app.js",{method:"HEAD"}).then(r=>parseInt(r.headers.get("Content-Length")||0)),fetch(location.href,{method:"HEAD"}).then(r=>parseInt(r.headers.get("Content-Length")||0))]).then(([w,j,h])=>{s.innerHTML='\''<div style="display:grid;grid-template-columns:1fr auto;gap:0 10px"><div>WASM:</div><div style="text-align:right">'\''+f(w)+'\''</div><div>JS:</div><div style="text-align:right">'\''+f(j)+'\''</div><div>HTML:</div><div style="text-align:right">'\''+f(h)+'\''</div><div style="border-top:1px solid lime;margin-top:2px;padding-top:2px">TOTAL:</div><div style="border-top:1px solid lime;margin-top:2px;padding-top:2px;text-align:right">'\''+f(w+j+h)+'\''</div></div>'\''})});</script>'

for ex in "${EXAMPLES[@]}"; do
    echo "Building example: $ex"
    rm -rf examples/$ex/dist
    ./examples/$ex/build.sh

    # Inject stats overlay into generated index.html
    if [ -f "examples/$ex/dist/index.html" ]; then
        echo "Injecting stats overlay into $ex..."
        sed -i "s~</body>~$STATS_HTML</body>~" "examples/$ex/dist/index.html"
    else
        echo "Warning: index.html not found for $ex"
    fi
done

# 3. Start Python server in background
echo "Starting HTTP server on port 8000..."
python3 -m http.server 8000 &
SERVER_PID=$!

# Function to kill server on exit
cleanup() {
    echo "Stopping server..."
    kill $SERVER_PID
}
trap cleanup EXIT

# 4. Open examples in browser
echo "Opening examples in browser..."
BASE_URL="http://localhost:8000/examples"

# Wait a moment for server to start
sleep 1

# Try to detect the open command
OPEN_CMD=""
if command -v xdg-open &> /dev/null; then
    OPEN_CMD="xdg-open"
elif command -v open &> /dev/null; then
    OPEN_CMD="open" # macOS
elif command -v start &> /dev/null; then
    OPEN_CMD="start" # Windows (Git Bash)
fi

if [ -n "$OPEN_CMD" ]; then
    # Open all URLs (xdg-open only accepts one URL at a time)
    for ex in "${EXAMPLES[@]}"; do
        $OPEN_CMD "$BASE_URL/$ex/dist/index.html" &
    done
else
    echo "Could not detect a command to open the browser."
    echo "Please manually visit:"
    for ex in "${EXAMPLES[@]}"; do
        echo "  $BASE_URL/$ex/dist/index.html"
    done
fi

echo "Server is running. Press Ctrl+C to stop."
wait $SERVER_PID
