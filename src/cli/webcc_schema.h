// GENERATED FILE - DO NOT EDIT
#pragma once
#include <cstdint>

namespace webcc {

struct SchemaParam {
    const char* type;
    const char* name;
};

struct SchemaCommand {
    const char* ns;
    const char* name;
    uint8_t opcode;
    const char* func_name;
    const char* return_type;
    const char* action;
    int num_params;
    SchemaParam params[8];
};

struct SchemaEvent {
    const char* ns;
    const char* name;
    uint8_t opcode;
    int num_params;
    SchemaParam params[8];
};

static const SchemaCommand SCHEMA_COMMANDS[] = {
    { "dom", "GET_BODY", 1, "get_body", "int32", R"JS_ACTION({ if(!elements[0]) elements[0] = document.body; return 0; })JS_ACTION", 0, {} },
    { "dom", "CREATE_ELEMENT", 2, "create_element", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const el = document.createElement(tag); elements[handle] = el; return handle; })JS_ACTION", 1, {{ "string", "tag" }} },
    { "dom", "SET_ATTRIBUTE", 3, "set_attribute", "", R"JS_ACTION({ const el = elements[handle]; if(!el){ console.warn('set_attribute: unknown element handle', handle); continue; } el.setAttribute(name, value); })JS_ACTION", 3, {{ "int32", "handle" }, { "string", "name" }, { "string", "value" }} },
    { "dom", "GET_ATTRIBUTE", 4, "get_attribute", "", R"JS_ACTION({ /* TODO: Return values not supported yet */ })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "name" }} },
    { "dom", "APPEND_CHILD", 5, "append_child", "", R"JS_ACTION({ const parent = elements[parent_handle]; const child = elements[child_handle]; if(!parent || !child){ console.warn('append_child: unknown handles', parent_handle, child_handle); continue; } parent.appendChild(child); })JS_ACTION", 2, {{ "int32", "parent_handle" }, { "int32", "child_handle" }} },
    { "dom", "REMOVE_ELEMENT", 6, "remove_element", "", R"JS_ACTION({ const el = elements[handle]; if(!el){ console.warn('remove_element: unknown element handle', handle); continue; } el.remove(); elements[handle] = undefined; })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "dom", "SET_INNER_HTML", 7, "set_inner_html", "", R"JS_ACTION({ const el = elements[handle]; if(el) el.innerHTML = html; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "html" }} },
    { "dom", "SET_INNER_TEXT", 8, "set_inner_text", "", R"JS_ACTION({ const el = elements[handle]; if(el) el.innerText = text; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "text" }} },
    { "dom", "ADD_CLASS", 9, "add_class", "", R"JS_ACTION({ const el = elements[handle]; if(el) el.classList.add(cls); })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "cls" }} },
    { "dom", "REMOVE_CLASS", 10, "remove_class", "", R"JS_ACTION({ const el = elements[handle]; if(el) el.classList.remove(cls); })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "cls" }} },
    { "canvas", "CREATE_CANVAS", 11, "create_canvas", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const c = document.createElement('canvas'); c.id = dom_id; c.width = width; c.height = height; elements[dom_id] = c; elements[handle] = c; return handle; })JS_ACTION", 3, {{ "string", "dom_id" }, { "float32", "width" }, { "float32", "height" }} },
    { "canvas", "GET_CONTEXT", 12, "get_context", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const c = elements[canvas_handle]; if(!c) { console.warn('get_context: unknown canvas', canvas_handle); return 0; } contexts[handle] = c.getContext(context_type); return handle; })JS_ACTION", 2, {{ "int32", "canvas_handle" }, { "string", "context_type" }} },
    { "canvas", "SET_SIZE", 13, "set_size", "", R"JS_ACTION({ const c = elements[handle]; if(c) { c.width = width; c.height = height; } })JS_ACTION", 3, {{ "int32", "handle" }, { "float32", "width" }, { "float32", "height" }} },
    { "canvas", "SET_FILL_STYLE", 14, "set_fill_style", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('set_fill_style: unknown context', handle); continue; } ctx.fillStyle = `rgb(${r},${g},${b})`; })JS_ACTION", 4, {{ "int32", "handle" }, { "uint8", "r" }, { "uint8", "g" }, { "uint8", "b" }} },
    { "canvas", "SET_FILL_STYLE_STR", 15, "set_fill_style_str", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.fillStyle = color; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "color" }} },
    { "canvas", "FILL_RECT", 16, "fill_rect", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('fill_rect: unknown context', handle); continue; } ctx.fillRect(x, y, w, h); })JS_ACTION", 5, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }, { "float32", "w" }, { "float32", "h" }} },
    { "canvas", "CLEAR_RECT", 17, "clear_rect", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('clear_canvas: unknown context', handle); continue; } ctx.clearRect(x, y, w, h); })JS_ACTION", 5, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }, { "float32", "w" }, { "float32", "h" }} },
    { "canvas", "STROKE_RECT", 18, "stroke_rect", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('stroke_rect: unknown context', handle); continue; } ctx.strokeRect(x, y, w, h); })JS_ACTION", 5, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }, { "float32", "w" }, { "float32", "h" }} },
    { "canvas", "SET_STROKE_STYLE", 19, "set_stroke_style", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('set_stroke_style: unknown context', handle); continue; } ctx.strokeStyle = `rgb(${r},${g},${b})`; })JS_ACTION", 4, {{ "int32", "handle" }, { "uint8", "r" }, { "uint8", "g" }, { "uint8", "b" }} },
    { "canvas", "SET_STROKE_STYLE_STR", 20, "set_stroke_style_str", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.strokeStyle = color; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "color" }} },
    { "canvas", "SET_LINE_WIDTH", 21, "set_line_width", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.lineWidth = width; })JS_ACTION", 2, {{ "int32", "handle" }, { "float32", "width" }} },
    { "canvas", "BEGIN_PATH", 22, "begin_path", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('begin_path: unknown context', handle); continue; } ctx.beginPath(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "canvas", "CLOSE_PATH", 23, "close_path", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.closePath(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "canvas", "MOVE_TO", 24, "move_to", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('move_to: unknown context', handle); continue; } ctx.moveTo(x, y); })JS_ACTION", 3, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "LINE_TO", 25, "line_to", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('line_to: unknown context', handle); continue; } ctx.lineTo(x, y); })JS_ACTION", 3, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "STROKE", 26, "stroke", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('stroke: unknown context', handle); continue; } ctx.stroke(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "canvas", "FILL", 27, "fill", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('fill: unknown context', handle); continue; } ctx.fill(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "canvas", "ARC", 28, "arc", "", R"JS_ACTION({ const ctx = contexts[handle]; if(!ctx){ console.warn('arc: unknown context', handle); continue; } ctx.arc(x, y, radius, start_angle, end_angle); })JS_ACTION", 6, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }, { "float32", "radius" }, { "float32", "start_angle" }, { "float32", "end_angle" }} },
    { "canvas", "FILL_TEXT", 29, "fill_text", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.fillText(text, x, y); })JS_ACTION", 4, {{ "int32", "handle" }, { "string", "text" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "FILL_TEXT_F", 30, "fill_text_f", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.fillText(fmt.replace('%f', val.toFixed(2)), x, y); })JS_ACTION", 5, {{ "int32", "handle" }, { "string", "fmt" }, { "float32", "val" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "FILL_TEXT_I", 31, "fill_text_i", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.fillText(fmt.replace('%d', val), x, y); })JS_ACTION", 5, {{ "int32", "handle" }, { "string", "fmt" }, { "int32", "val" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "SET_FONT", 32, "set_font", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.font = font; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "font" }} },
    { "canvas", "SET_TEXT_ALIGN", 33, "set_text_align", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.textAlign = align; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "align" }} },
    { "canvas", "DRAW_IMAGE", 34, "draw_image", "", R"JS_ACTION({ const ctx = contexts[handle]; const img = images[img_handle]; if(ctx && img) ctx.drawImage(img, x, y); })JS_ACTION", 4, {{ "int32", "handle" }, { "int32", "img_handle" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "TRANSLATE", 35, "translate", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.translate(x, y); })JS_ACTION", 3, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "ROTATE", 36, "rotate", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.rotate(angle); })JS_ACTION", 2, {{ "int32", "handle" }, { "float32", "angle" }} },
    { "canvas", "SCALE", 37, "scale", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.scale(x, y); })JS_ACTION", 3, {{ "int32", "handle" }, { "float32", "x" }, { "float32", "y" }} },
    { "canvas", "SAVE", 38, "save", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.save(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "canvas", "RESTORE", 39, "restore", "", R"JS_ACTION({ const ctx = contexts[handle]; if(ctx) ctx.restore(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "canvas", "LOG_CANVAS_INFO", 40, "log_canvas_info", "", R"JS_ACTION({ const cv = elements[handle]; if(!cv){ console.warn('log_canvas_info: unknown canvas handle', handle); continue; } console.log('Canvas', handle, 'size:', cv.width, 'x', cv.height); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "canvas", "SET_GLOBAL_ALPHA", 41, "set_global_alpha", "", R"JS_ACTION({ const cv = elements[handle]; if(cv) cv.getContext('2d').globalAlpha = alpha; })JS_ACTION", 2, {{ "int32", "handle" }, { "float32", "alpha" }} },
    { "canvas", "SET_LINE_CAP", 42, "set_line_cap", "", R"JS_ACTION({ const cv = elements[handle]; if(cv) cv.getContext('2d').lineCap = cap; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "cap" }} },
    { "canvas", "SET_LINE_JOIN", 43, "set_line_join", "", R"JS_ACTION({ const cv = elements[handle]; if(cv) cv.getContext('2d').lineJoin = join; })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "join" }} },
    { "canvas", "SET_SHADOW", 44, "set_shadow", "", R"JS_ACTION({ const cv = elements[handle]; if(cv) { const ctx = cv.getContext('2d'); ctx.shadowBlur = blur; ctx.shadowOffsetX = off_x; ctx.shadowOffsetY = off_y; ctx.shadowColor = color; } })JS_ACTION", 5, {{ "int32", "handle" }, { "float32", "blur" }, { "float32", "off_x" }, { "float32", "off_y" }, { "string", "color" }} },
    { "input", "INIT_KEYBOARD", 45, "init_keyboard", "", R"JS_ACTION({ window.addEventListener('keydown', e => push_event_input_KEY_DOWN(e.keyCode)); window.addEventListener('keyup', e => push_event_input_KEY_UP(e.keyCode)); })JS_ACTION", 0, {} },
    { "input", "INIT_MOUSE", 46, "init_mouse", "", R"JS_ACTION({ const el = elements[handle] || document; el.addEventListener('mousedown', e => push_event_input_MOUSE_DOWN(e.button, e.offsetX, e.offsetY)); el.addEventListener('mouseup', e => push_event_input_MOUSE_UP(e.button, e.offsetX, e.offsetY)); el.addEventListener('mousemove', e => push_event_input_MOUSE_MOVE(e.offsetX, e.offsetY)); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "input", "REQUEST_POINTER_LOCK", 47, "request_pointer_lock", "", R"JS_ACTION({ const el = elements[handle] || document.body; el.requestPointerLock(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "input", "EXIT_POINTER_LOCK", 48, "exit_pointer_lock", "", R"JS_ACTION({ document.exitPointerLock(); })JS_ACTION", 0, {} },
    { "system", "LOG", 49, "log", "", R"JS_ACTION({ console.log(msg); })JS_ACTION", 1, {{ "string", "msg" }} },
    { "system", "WARN", 50, "warn", "", R"JS_ACTION({ console.warn(msg); })JS_ACTION", 1, {{ "string", "msg" }} },
    { "system", "ERROR", 51, "error", "", R"JS_ACTION({ console.error(msg); })JS_ACTION", 1, {{ "string", "msg" }} },
    { "system", "SET_MAIN_LOOP", 52, "set_main_loop", "", R"JS_ACTION({ const fn = table.get(func); if(!fn){ console.error('set_main_loop: function not found in table', func); continue; } const loop = (t) => { fn(t); requestAnimationFrame(loop); }; requestAnimationFrame(loop); })JS_ACTION", 1, {{ "func_ptr", "func" }} },
    { "system", "SET_TITLE", 53, "set_title", "", R"JS_ACTION({ document.title = title; })JS_ACTION", 1, {{ "string", "title" }} },
    { "system", "RELOAD", 54, "reload", "", R"JS_ACTION({ location.reload(); })JS_ACTION", 0, {} },
    { "system", "OPEN_URL", 55, "open_url", "", R"JS_ACTION({ window.open(url, '_blank'); })JS_ACTION", 1, {{ "string", "url" }} },
    { "system", "REQUEST_FULLSCREEN", 56, "request_fullscreen", "", R"JS_ACTION({ const el = elements[handle] || document.body; el.requestFullscreen().catch(console.error); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "storage", "SET_ITEM", 57, "set_item", "", R"JS_ACTION({ localStorage.setItem(key, value); })JS_ACTION", 2, {{ "string", "key" }, { "string", "value" }} },
    { "storage", "REMOVE_ITEM", 58, "remove_item", "", R"JS_ACTION({ localStorage.removeItem(key); })JS_ACTION", 1, {{ "string", "key" }} },
    { "storage", "CLEAR", 59, "clear", "", R"JS_ACTION({ localStorage.clear(); })JS_ACTION", 0, {} },
    { "audio", "CREATE_AUDIO", 60, "create_audio", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const a = new Audio(src); audios[handle] = a; elements[handle] = a; return handle; })JS_ACTION", 1, {{ "string", "src" }} },
    { "audio", "PLAY", 61, "play", "", R"JS_ACTION({ const a = audios[handle]; if(a) a.play().catch(e => console.warn(e)); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "audio", "PAUSE", 62, "pause", "", R"JS_ACTION({ const a = audios[handle]; if(a) a.pause(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "audio", "SET_VOLUME", 63, "set_volume", "", R"JS_ACTION({ const a = audios[handle]; if(a) a.volume = vol; })JS_ACTION", 2, {{ "int32", "handle" }, { "float32", "vol" }} },
    { "audio", "SET_LOOP", 64, "set_loop", "", R"JS_ACTION({ const a = audios[handle]; if(a) a.loop = (loop !== 0); })JS_ACTION", 2, {{ "int32", "handle" }, { "uint8", "loop" }} },
    { "audio", "GET_CURRENT_TIME", 65, "get_current_time", "float", R"JS_ACTION({ const a = audios[handle]; return (a ? (a.currentTime || 0) : 0); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "audio", "GET_DURATION", 66, "get_duration", "float", R"JS_ACTION({ const a = audios[handle]; return (a ? (a.duration || 0) : 0); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "websocket", "CREATE", 67, "create", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const ws = new WebSocket(url); websockets[handle] = ws; if(events & 1) ws.onmessage = (e) => push_event_websocket_MESSAGE(handle, e.data); if(events & 2) ws.onopen = () => push_event_websocket_OPEN(handle); if(events & 4) ws.onclose = () => push_event_websocket_CLOSE(handle); if(events & 8) ws.onerror = () => push_event_websocket_ERROR(handle); return handle; })JS_ACTION", 2, {{ "string", "url" }, { "uint32", "events" }} },
    { "websocket", "SEND", 68, "send", "", R"JS_ACTION({ const ws = websockets[handle]; if(ws && ws.readyState === 1) ws.send(msg); })JS_ACTION", 2, {{ "int32", "handle" }, { "string", "msg" }} },
    { "websocket", "CLOSE", 69, "close", "", R"JS_ACTION({ const ws = websockets[handle]; if(ws) ws.close(); })JS_ACTION", 1, {{ "int32", "handle" }} },
    { "fetch", "GET", 70, "get", "int32", R"JS_ACTION({ const id = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); fetch(url).then(r => { if(!r.ok) throw new Error(r.status + ' ' + r.statusText); return r.text(); }).then(d => push_event_fetch_SUCCESS(id, d)).catch(e => push_event_fetch_ERROR(id, e.toString())); return id; })JS_ACTION", 1, {{ "string", "url" }} },
    { "fetch", "POST", 71, "post", "int32", R"JS_ACTION({ const id = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); fetch(url, { method: 'POST', body: body }).then(r => { if(!r.ok) throw new Error(r.status + ' ' + r.statusText); return r.text(); }).then(d => push_event_fetch_SUCCESS(id, d)).catch(e => push_event_fetch_ERROR(id, e.toString())); return id; })JS_ACTION", 2, {{ "string", "url" }, { "string", "body" }} },
    { "image", "LOAD", 72, "load", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const img = new Image(); img.src = src; images[handle] = img; elements[handle] = img; return handle; })JS_ACTION", 1, {{ "string", "src" }} },
    { "webgl", "VIEWPORT", 73, "viewport", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) gl.viewport(x, y, width, height); })JS_ACTION", 5, {{ "int32", "ctx_handle" }, { "int32", "x" }, { "int32", "y" }, { "int32", "width" }, { "int32", "height" }} },
    { "webgl", "CLEAR_COLOR", 74, "clear_color", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) gl.clearColor(r, g, b, a); })JS_ACTION", 5, {{ "int32", "ctx_handle" }, { "float32", "r" }, { "float32", "g" }, { "float32", "b" }, { "float32", "a" }} },
    { "webgl", "CLEAR", 75, "clear", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) gl.clear(mask); })JS_ACTION", 2, {{ "int32", "ctx_handle" }, { "uint32", "mask" }} },
    { "webgl", "CREATE_SHADER", 76, "create_shader", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const gl = contexts[ctx_handle]; if(gl) { const s = gl.createShader(type); gl.shaderSource(s, source); gl.compileShader(s); if(!gl.getShaderParameter(s, gl.COMPILE_STATUS)) console.error(gl.getShaderInfoLog(s)); webgl_shaders[handle] = s; } return handle; })JS_ACTION", 3, {{ "int32", "ctx_handle" }, { "uint32", "type" }, { "string", "source" }} },
    { "webgl", "CREATE_PROGRAM", 77, "create_program", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const gl = contexts[ctx_handle]; if(gl) { const p = gl.createProgram(); webgl_programs[handle] = p; } return handle; })JS_ACTION", 1, {{ "int32", "ctx_handle" }} },
    { "webgl", "ATTACH_SHADER", 78, "attach_shader", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; const p = webgl_programs[prog_handle]; const s = webgl_shaders[shader_handle]; if(gl && p && s) gl.attachShader(p, s); })JS_ACTION", 3, {{ "int32", "ctx_handle" }, { "int32", "prog_handle" }, { "int32", "shader_handle" }} },
    { "webgl", "LINK_PROGRAM", 79, "link_program", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; const p = webgl_programs[prog_handle]; if(gl && p) { gl.linkProgram(p); if(!gl.getProgramParameter(p, gl.LINK_STATUS)) console.error(gl.getProgramInfoLog(p)); } })JS_ACTION", 2, {{ "int32", "ctx_handle" }, { "int32", "prog_handle" }} },
    { "webgl", "BIND_ATTRIB_LOCATION", 80, "bind_attrib_location", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; const p = webgl_programs[prog_handle]; if(gl && p) gl.bindAttribLocation(p, index, name); })JS_ACTION", 4, {{ "int32", "ctx_handle" }, { "int32", "prog_handle" }, { "uint32", "index" }, { "string", "name" }} },
    { "webgl", "USE_PROGRAM", 81, "use_program", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; const p = webgl_programs[prog_handle]; if(gl && p) gl.useProgram(p); })JS_ACTION", 2, {{ "int32", "ctx_handle" }, { "int32", "prog_handle" }} },
    { "webgl", "CREATE_BUFFER", 82, "create_buffer", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const gl = contexts[ctx_handle]; if(gl) { const b = gl.createBuffer(); webgl_buffers[handle] = b; } return handle; })JS_ACTION", 1, {{ "int32", "ctx_handle" }} },
    { "webgl", "BIND_BUFFER", 83, "bind_buffer", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; const b = webgl_buffers[buf_handle]; if(gl && b) gl.bindBuffer(target, b); })JS_ACTION", 3, {{ "int32", "ctx_handle" }, { "uint32", "target" }, { "int32", "buf_handle" }} },
    { "webgl", "BUFFER_DATA", 84, "buffer_data", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) { const data = new Uint8Array(memory.buffer, data_ptr, data_len); gl.bufferData(target, data, usage); } })JS_ACTION", 5, {{ "int32", "ctx_handle" }, { "uint32", "target" }, { "uint32", "data_ptr" }, { "uint32", "data_len" }, { "uint32", "usage" }} },
    { "webgl", "ENABLE_VERTEX_ATTRIB_ARRAY", 85, "enable_vertex_attrib_array", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) gl.enableVertexAttribArray(index); })JS_ACTION", 2, {{ "int32", "ctx_handle" }, { "uint32", "index" }} },
    { "webgl", "ENABLE", 86, "enable", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) gl.enable(cap); })JS_ACTION", 2, {{ "int32", "ctx_handle" }, { "uint32", "cap" }} },
    { "webgl", "GET_UNIFORM_LOCATION", 87, "get_uniform_location", "int32", R"JS_ACTION({ const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const gl = contexts[ctx_handle]; const p = webgl_programs[prog_handle]; if(gl && p) { const loc = gl.getUniformLocation(p, name); if(!loc) console.warn('getUniformLocation failed:', name); webgl_uniforms[handle] = loc; } return handle; })JS_ACTION", 3, {{ "int32", "ctx_handle" }, { "int32", "prog_handle" }, { "string", "name" }} },
    { "webgl", "UNIFORM_1F", 88, "uniform_1f", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; const loc = webgl_uniforms[loc_handle]; if(loc === undefined) console.warn('uniform_1f: loc undefined', loc_handle); if(gl && loc !== undefined) gl.uniform1f(loc, val); })JS_ACTION", 3, {{ "int32", "ctx_handle" }, { "int32", "loc_handle" }, { "float32", "val" }} },
    { "webgl", "VERTEX_ATTRIB_POINTER", 89, "vertex_attrib_pointer", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) gl.vertexAttribPointer(index, size, type, normalized !== 0, stride, offset); })JS_ACTION", 7, {{ "int32", "ctx_handle" }, { "uint32", "index" }, { "int32", "size" }, { "uint32", "type" }, { "uint8", "normalized" }, { "int32", "stride" }, { "int32", "offset" }} },
    { "webgl", "DRAW_ARRAYS", 90, "draw_arrays", "", R"JS_ACTION({ const gl = contexts[ctx_handle]; if(gl) gl.drawArrays(mode, first, count); })JS_ACTION", 4, {{ "int32", "ctx_handle" }, { "uint32", "mode" }, { "int32", "first" }, { "int32", "count" }} },
    { "wgpu", "REQUEST_ADAPTER", 91, "request_adapter", "", R"JS_ACTION({ if (!navigator.gpu) { console.warn('NO: navigator.gpu is undefined — WebGPU not available'); push_event_wgpu_ADAPTER_READY(0); return; } console.log('navigator.gpu OK'); navigator.gpu.requestAdapter({ powerPreference: 'high-performance' }).then(a => a || navigator.gpu.requestAdapter()).then(adapter => { if (!adapter) { console.warn('NO: requestAdapter returned null — no usable adapter'); push_event_wgpu_ADAPTER_READY(0); return; } console.log('Adapter:', adapter); console.log('Features:', Array.from(adapter.features || [])); console.log('Limits:', adapter.limits || {}); const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_adapters[h] = adapter; push_event_wgpu_ADAPTER_READY(h); }).catch(e => { console.error('requestAdapter failed:', e); push_event_wgpu_ADAPTER_READY(0); }); })JS_ACTION", 0, {} },
    { "wgpu", "REQUEST_DEVICE", 92, "request_device", "", R"JS_ACTION({ const a = webgpu_adapters[adapter_handle]; if(a) a.requestDevice().then(d => { const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_devices[h] = d; webgpu_queues[h] = d.queue; push_event_wgpu_DEVICE_READY(h); }).catch(e => console.error("WebGPU: requestDevice failed", e)); })JS_ACTION", 1, {{ "int32", "adapter_handle" }} },
    { "wgpu", "GET_QUEUE", 93, "get_queue", "int32", R"JS_ACTION({ const d = webgpu_devices[device_handle]; if(!d) return 0; const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_queues[h] = d.queue; return h; })JS_ACTION", 1, {{ "int32", "device_handle" }} },
    { "wgpu", "CREATE_SHADER_MODULE", 94, "create_shader_module", "int32", R"JS_ACTION({ const d = webgpu_devices[device_handle]; if(!d) return 0; const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_shaders[h] = d.createShaderModule({ code: code }); return h; })JS_ACTION", 2, {{ "int32", "device_handle" }, { "string", "code" }} },
    { "wgpu", "CREATE_COMMAND_ENCODER", 95, "create_command_encoder", "int32", R"JS_ACTION({ const d = webgpu_devices[device_handle]; if(!d) return 0; const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_encoders[h] = d.createCommandEncoder(); return h; })JS_ACTION", 1, {{ "int32", "device_handle" }} },
    { "wgpu", "CONFIGURE", 96, "configure", "", R"JS_ACTION({ const ctx = contexts[context_handle]; const dev = webgpu_devices[device_handle]; if(ctx && dev) ctx.configure({ device: dev, format: format === 'preferred' ? navigator.gpu.getPreferredCanvasFormat() : format, alphaMode: 'premultiplied' }); })JS_ACTION", 3, {{ "int32", "context_handle" }, { "int32", "device_handle" }, { "string", "format" }} },
    { "wgpu", "GET_CURRENT_TEXTURE_VIEW", 97, "get_current_texture_view", "int32", R"JS_ACTION({ const ctx = contexts[context_handle]; if(!ctx) return 0; const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_views[h] = ctx.getCurrentTexture().createView(); return h; })JS_ACTION", 1, {{ "int32", "context_handle" }} },
    { "wgpu", "BEGIN_RENDER_PASS", 98, "begin_render_pass", "int32", R"JS_ACTION({ const enc = webgpu_encoders[encoder_handle]; const view = webgpu_views[view_handle]; if(!enc || !view) return 0; const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_passes[h] = enc.beginRenderPass({ colorAttachments: [{ view: view, clearValue: {r, g, b, a}, loadOp: 'clear', storeOp: 'store' }] }); return h; })JS_ACTION", 6, {{ "int32", "encoder_handle" }, { "int32", "view_handle" }, { "float32", "r" }, { "float32", "g" }, { "float32", "b" }, { "float32", "a" }} },
    { "wgpu", "END_PASS", 99, "end_pass", "", R"JS_ACTION({ const pass = webgpu_passes[pass_handle]; if(pass) pass.end(); })JS_ACTION", 1, {{ "int32", "pass_handle" }} },
    { "wgpu", "FINISH_ENCODER", 100, "finish_encoder", "int32", R"JS_ACTION({ const enc = webgpu_encoders[encoder_handle]; if(!enc) return 0; const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_buffers[h] = enc.finish(); return h; })JS_ACTION", 1, {{ "int32", "encoder_handle" }} },
    { "wgpu", "QUEUE_SUBMIT", 101, "queue_submit", "", R"JS_ACTION({ const q = webgpu_queues[queue_handle]; const cb = webgpu_buffers[command_buffer_handle]; if(q && cb) q.submit([cb]); })JS_ACTION", 2, {{ "int32", "queue_handle" }, { "int32", "command_buffer_handle" }} },
    { "wgpu", "CREATE_RENDER_PIPELINE_SIMPLE", 102, "create_render_pipeline_simple", "int32", R"JS_ACTION({ const d = webgpu_devices[device_handle]; const vs = webgpu_shaders[vs_module]; const fs = webgpu_shaders[fs_module]; if(!d || !vs || !fs) return 0; const h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); webgpu_pipelines[h] = d.createRenderPipeline({ layout: 'auto', vertex: { module: vs, entryPoint: vs_entry }, fragment: { module: fs, entryPoint: fs_entry, targets: [{ format: format === 'preferred' ? navigator.gpu.getPreferredCanvasFormat() : format }] }, primitive: { topology: 'triangle-list' } }); return h; })JS_ACTION", 6, {{ "int32", "device_handle" }, { "int32", "vs_module" }, { "int32", "fs_module" }, { "string", "vs_entry" }, { "string", "fs_entry" }, { "string", "format" }} },
    { "wgpu", "SET_PIPELINE", 103, "set_pipeline", "", R"JS_ACTION({ const pass = webgpu_passes[pass_handle]; const pipe = webgpu_pipelines[pipeline_handle]; if(pass && pipe) pass.setPipeline(pipe); })JS_ACTION", 2, {{ "int32", "pass_handle" }, { "int32", "pipeline_handle" }} },
    { "wgpu", "DRAW", 104, "draw", "", R"JS_ACTION({ const pass = webgpu_passes[pass_handle]; if(pass) pass.draw(vertex_count, instance_count, first_vertex, first_instance); })JS_ACTION", 5, {{ "int32", "pass_handle" }, { "int32", "vertex_count" }, { "int32", "instance_count" }, { "int32", "first_vertex" }, { "int32", "first_instance" }} },
    { nullptr, nullptr, 0, nullptr, nullptr, nullptr, 0, {} }
};

static const SchemaEvent SCHEMA_EVENTS[] = {
    { "input", "KEY_DOWN", 1, 1, {{ "int32", "key_code" }} },
    { "input", "KEY_UP", 2, 1, {{ "int32", "key_code" }} },
    { "input", "MOUSE_DOWN", 3, 3, {{ "int32", "button" }, { "int32", "x" }, { "int32", "y" }} },
    { "input", "MOUSE_UP", 4, 3, {{ "int32", "button" }, { "int32", "x" }, { "int32", "y" }} },
    { "input", "MOUSE_MOVE", 5, 2, {{ "int32", "x" }, { "int32", "y" }} },
    { "websocket", "MESSAGE", 6, 2, {{ "int32", "handle" }, { "string", "data" }} },
    { "websocket", "OPEN", 7, 1, {{ "int32", "handle" }} },
    { "websocket", "CLOSE", 8, 1, {{ "int32", "handle" }} },
    { "websocket", "ERROR", 9, 1, {{ "int32", "handle" }} },
    { "fetch", "SUCCESS", 10, 2, {{ "int32", "id" }, { "string", "data" }} },
    { "fetch", "ERROR", 11, 2, {{ "int32", "id" }, { "string", "error" }} },
    { "wgpu", "ADAPTER_READY", 12, 1, {{ "int32", "handle" }} },
    { "wgpu", "DEVICE_READY", 13, 1, {{ "int32", "handle" }} },
    { nullptr, nullptr, 0, 0, {} }
};

} // namespace webcc
