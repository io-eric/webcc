import http.server
import socketserver
import os
import json
import subprocess
import sys
import threading
import time
import webbrowser
from urllib.parse import urlparse

PORT = 8000
BENCHMARK_FRAMES = 500
RESULTS = {}
EXPECTED_REPORTS = 2
server_instance = None

def kill_process_on_port(port):
    """Kill any process using the specified port."""
    try:
        # Find PID using the port
        result = subprocess.run(['lsof', '-ti', f':{port}'], 
                              capture_output=True, text=True, timeout=5)
        if result.returncode == 0 and result.stdout.strip():
            pids = result.stdout.strip().split('\n')
            for pid in pids:
                print(f"Killing process {pid} using port {port}")
                subprocess.run(['kill', '-9', pid], check=True, timeout=5)
            time.sleep(2)  # Wait for port to be freed
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
        print("Could not find/kill existing process on port (lsof not available or no process found)")
        pass

class BenchmarkHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/report':
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            data = json.loads(post_data.decode('utf-8'))
            
            name = data.get('name', 'unknown')
            print(f"Received report from {name}: {data}")
            RESULTS[name] = data
            
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b'OK')
            
            if len(RESULTS) >= EXPECTED_REPORTS:
                print("All reports received. Shutting down server...")
                threading.Thread(target=self.server.shutdown).start()
        else:
            self.send_error(404)

    def do_GET(self):
        # Map /webcc to webcc/dist and /emscripten to emscripten/dist
        if self.path.startswith('/webcc/'):
            self.path = self.path.replace('/webcc/', '/webcc/dist/')
        elif self.path.startswith('/emscripten/'):
            self.path = self.path.replace('/emscripten/', '/emscripten/dist/')
        
        return http.server.SimpleHTTPRequestHandler.do_GET(self)

def build_projects():
    print("Building projects...")
    try:
        subprocess.check_call(['./run.sh'], cwd=os.path.dirname(os.path.abspath(__file__)))
    except subprocess.CalledProcessError as e:
        print(f"Build failed: {e}")
        sys.exit(1)

def get_file_sizes():
    sizes = {}
    
    # WebCC
    webcc_wasm = os.path.join('webcc', 'dist', 'app.wasm')
    webcc_js = os.path.join('webcc', 'dist', 'app.js')
    if os.path.exists(webcc_wasm):
        sizes['webcc_wasm'] = os.path.getsize(webcc_wasm)
    if os.path.exists(webcc_js):
        sizes['webcc_js'] = os.path.getsize(webcc_js)
        
    # Emscripten
    em_wasm = os.path.join('emscripten', 'dist', 'index.wasm')
    em_js = os.path.join('emscripten', 'dist', 'index.js')
    if os.path.exists(em_wasm):
        sizes['emscripten_wasm'] = os.path.getsize(em_wasm)
    if os.path.exists(em_js):
        sizes['emscripten_js'] = os.path.getsize(em_js)
        
    return sizes

def run_server():
    global server_instance, PORT
    max_attempts = 5
    for attempt in range(max_attempts):
        try:
            with socketserver.TCPServer(("", PORT), BenchmarkHandler) as httpd:
                server_instance = httpd
                print(f"Serving at port {PORT}")
                httpd.serve_forever()
            break  # Success
        except OSError as e:
            if attempt < max_attempts - 1:
                print(f"Port {PORT} in use, trying {PORT + 1}")
                PORT += 1
            else:
                print(f"Failed to start server after {max_attempts} attempts: {e}")
                sys.exit(1)

def wait_for_result(name, timeout=60):
    print(f"Waiting for {name} results (timeout {timeout}s)...")
    start_wait = time.time()
    while name not in RESULTS:
        if time.time() - start_wait > timeout:
            print(f"Timeout waiting for {name}.")
            return False
        time.sleep(1)
    return True

def parse_browser(ua):
    if 'Chrome' in ua:
        parts = ua.split('Chrome/')
        if len(parts) > 1:
            ver = parts[1].split(' ')[0]
            return f"Chrome {ver}"
    if 'Firefox' in ua:
        parts = ua.split('Firefox/')
        if len(parts) > 1:
            ver = parts[1].split(' ')[0]
            return f"Firefox {ver}"
    if 'Safari' in ua and 'Version' in ua:
        parts = ua.split('Version/')
        if len(parts) > 1:
            ver = parts[1].split(' ')[0]
            return f"Safari {ver}"
    return "Unknown Browser"

def generate_svg_report(results, file_sizes, browser_name):
    w_wasm = file_sizes.get('webcc_wasm', 0) / 1024
    e_wasm = file_sizes.get('emscripten_wasm', 0) / 1024
    w_js = file_sizes.get('webcc_js', 0) / 1024
    e_js = file_sizes.get('emscripten_js', 0) / 1024
    w_fps = results.get('webcc', {}).get('fps', 0)
    e_fps = results.get('emscripten', {}).get('fps', 0)
    w_mem = results.get('webcc', {}).get('memory_used_mb', 0)
    e_mem = results.get('emscripten', {}).get('memory_used_mb', 0)
    w_wasm_mem = results.get('webcc', {}).get('wasm_heap_mb', 0)
    e_wasm_mem = results.get('emscripten', {}).get('wasm_heap_mb', 0)

    # (Name, WebCC Value, Emscripten Value, Lower is Better, Unit)
    metrics = [
        ("FPS", w_fps, e_fps, False, "FPS"),
        ("WASM Size", w_wasm, e_wasm, True, "KB"),
        ("JS Size", w_js, e_js, True, "KB"),
        ("JS Heap", w_mem, e_mem, True, "MB"),
        ("WASM Heap", w_wasm_mem, e_wasm_mem, True, "MB")
    ]

    width = 900
    height = 700
    
    # Modern Color Palette
    bg_color = "#f8f9fa"
    text_main = "#212529"
    text_sub = "#6c757d"
    
    # WebCC Colors (Teal/Green)
    c_webcc = "#20c997"
    
    # Emscripten Colors (Indigo/Blue)
    c_em = "#5c7cfa"

    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" style="font-family: \'Segoe UI\', Roboto, Helvetica, Arial, sans-serif; background: {bg_color};">']
    
    # Background
    svg.append(f'<rect width="100%" height="100%" fill="{bg_color}"/>')
    
    # Header
    svg.append(f'<text x="{width/2}" y="60" text-anchor="middle" fill="{text_main}" font-size="32" font-weight="bold" letter-spacing="-0.5">WebCC vs Emscripten</text>')
    svg.append(f'<text x="{width/2}" y="95" text-anchor="middle" fill="{text_sub}" font-size="18">Canvas 2D • 10,000 Rectangles • {browser_name}</text>')

    # Legend
    legend_y = 130
    # WebCC Legend
    svg.append(f'<rect x="{width/2 - 120}" y="{legend_y}" width="20" height="20" fill="{c_webcc}" rx="4"/>')
    svg.append(f'<text x="{width/2 - 90}" y="{legend_y + 15}" fill="{text_main}" font-size="16" font-weight="600">WebCC</text>')
    # Emscripten Legend
    svg.append(f'<rect x="{width/2 + 20}" y="{legend_y}" width="20" height="20" fill="{c_em}" rx="4"/>')
    svg.append(f'<text x="{width/2 + 50}" y="{legend_y + 15}" fill="{text_main}" font-size="16" font-weight="600">Emscripten</text>')

    # Charts
    start_y = 190
    row_height = 90
    bar_height = 28
    max_bar_width = 450
    chart_start_x = 200

    for i, (name, w_val, e_val, lower_better, unit) in enumerate(metrics):
        y = start_y + i * row_height
        
        # Metric Label
        svg.append(f'<text x="{chart_start_x - 20}" y="{y + bar_height}" text-anchor="end" fill="{text_main}" font-size="16" font-weight="bold">{name}</text>')
        
        # Calculate widths
        max_val = max(w_val, e_val) if max(w_val, e_val) > 0 else 1
        w_width = (w_val / max_val) * max_bar_width
        e_width = (e_val / max_val) * max_bar_width
        
        # Minimum width for visibility
        w_width = max(w_width, 2)
        e_width = max(e_width, 2)

        # WebCC Bar
        svg.append(f'<rect x="{chart_start_x}" y="{y}" width="{w_width}" height="{bar_height}" fill="{c_webcc}" rx="4"/>')
        w_text = f"{w_val:.2f} {unit}"
        svg.append(f'<text x="{chart_start_x + w_width + 10}" y="{y + 19}" fill="{text_main}" font-size="14">{w_text}</text>')
        
        # Emscripten Bar
        svg.append(f'<rect x="{chart_start_x}" y="{y + bar_height + 8}" width="{e_width}" height="{bar_height}" fill="{c_em}" rx="4"/>')
        e_text = f"{e_val:.2f} {unit}"
        svg.append(f'<text x="{chart_start_x + e_width + 10}" y="{y + bar_height + 8 + 19}" fill="{text_main}" font-size="14">{e_text}</text>')

        # Winner Indicator (Dot) & Percentage
        is_w_better = (w_val < e_val) if lower_better else (w_val > e_val)
        
        # Calculate percentage
        val_1 = w_val
        val_2 = e_val
        
        if lower_better:
            # For size/memory, calculate reduction: (Big - Small) / Big
            worst = max(val_1, val_2)
            best = min(val_1, val_2)
            if worst > 0:
                pct = ((worst - best) / worst) * 100
                diff_text = f"{pct:.1f}% smaller"
            else:
                diff_text = ""
        else:
            # For FPS, calculate increase: (Big - Small) / Small
            worst = min(val_1, val_2)
            best = max(val_1, val_2)
            if worst > 0:
                pct = ((best - worst) / worst) * 100
                diff_text = f"{pct:.1f}% faster"
            else:
                diff_text = ""
        
        # Position percentage text further to the right to avoid overlap
        pct_x = chart_start_x + max_bar_width + 100

        if is_w_better:
             svg.append(f'<circle cx="{chart_start_x - 10}" cy="{y + 14}" r="4" fill="{c_webcc}"/>')
             svg.append(f'<text x="{pct_x}" y="{y + 19}" fill="{c_webcc}" font-size="14" font-weight="bold">{diff_text}</text>')
        else:
             svg.append(f'<circle cx="{chart_start_x - 10}" cy="{y + bar_height + 8 + 14}" r="4" fill="{c_em}"/>')
             svg.append(f'<text x="{pct_x}" y="{y + bar_height + 8 + 19}" fill="{c_em}" font-size="14" font-weight="bold">{diff_text}</text>')

    svg.append('</svg>')
    
    with open('benchmark_results.svg', 'w') as f:
        f.write('\n'.join(svg))
    print("Generated benchmark_results.svg")

def main():
    # 1. Build
    if "--no-build" not in sys.argv:
        build_projects()
    
    # 2. Get static stats
    file_sizes = get_file_sizes()
    
    # 3. Kill any existing server on port
    kill_process_on_port(PORT)
    
    # 4. Start Server
    server_thread = threading.Thread(target=run_server)
    server_thread.start()
    
    # 5. Run Benchmarks Sequentially
    
    # WebCC
    print("Running WebCC Benchmark...")
    webbrowser.open(f'http://localhost:{PORT}/webcc/index.html')
    wait_for_result('webcc')
    
    # Emscripten
    print("Running Emscripten Benchmark...")
    webbrowser.open(f'http://localhost:{PORT}/emscripten/index.html')
    wait_for_result('emscripten')
    
    # Shutdown server
    if server_instance:
        threading.Thread(target=server_instance.shutdown).start()
    server_thread.join()
    
    # 6. Save and Print Report
    # Extract browser info (assume same for both)
    browser_ua = RESULTS.get('webcc', {}).get('browser', '') or RESULTS.get('emscripten', {}).get('browser', '')
    browser_name = parse_browser(browser_ua)
    
    # Remove browser from individual results to clean up JSON
    if 'browser' in RESULTS.get('webcc', {}):
        del RESULTS['webcc']['browser']
    if 'browser' in RESULTS.get('emscripten', {}):
        del RESULTS['emscripten']['browser']

    final_report = {
        'browser': browser_ua,
        'file_sizes': file_sizes,
        'runtime_stats': RESULTS
    }
    
    with open('benchmark_results.json', 'w') as f:
        json.dump(final_report, f, indent=4)
        
    generate_svg_report(RESULTS, file_sizes, browser_name)
        
    print("\n=== BENCHMARK RESULTS ===")
    print(f"Browser: {browser_name}")
    print(f"{'Metric':<20} | {'WebCC':<15} | {'Emscripten':<15}")
    print("-" * 56)
    
    # File Sizes
    w_wasm = file_sizes.get('webcc_wasm', 0) / 1024
    e_wasm = file_sizes.get('emscripten_wasm', 0) / 1024
    print(f"{'WASM Size (KB)':<20} | {w_wasm:<15.2f} | {e_wasm:<15.2f}")
    
    w_js = file_sizes.get('webcc_js', 0) / 1024
    e_js = file_sizes.get('emscripten_js', 0) / 1024
    print(f"{'JS Size (KB)':<20} | {w_js:<15.2f} | {e_js:<15.2f}")
    
    # Runtime
    w_fps = RESULTS.get('webcc', {}).get('fps', 0)
    e_fps = RESULTS.get('emscripten', {}).get('fps', 0)
    print(f"{'FPS':<20} | {w_fps:<15.2f} | {e_fps:<15.2f}")

    w_mem = RESULTS.get('webcc', {}).get('memory_used_mb', 0)
    e_mem = RESULTS.get('emscripten', {}).get('memory_used_mb', 0)
    print(f"{'JS Heap (MB)':<20} | {w_mem:<15.2f} | {e_mem:<15.2f}")

    w_wasm_mem = RESULTS.get('webcc', {}).get('wasm_heap_mb', 0)
    e_wasm_mem = RESULTS.get('emscripten', {}).get('wasm_heap_mb', 0)
    print(f"{'WASM Heap (MB)':<20} | {w_wasm_mem:<15.2f} | {e_wasm_mem:<15.2f}")

    # 7. Generate SVG Report
    generate_svg_report(RESULTS, file_sizes, parse_browser(browser_ua))

if __name__ == "__main__":
    main()
