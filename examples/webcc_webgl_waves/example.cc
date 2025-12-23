#include "webcc/webgl.h"
#include "webcc/canvas.h"
#include "webcc/dom.h"
#include "webcc/system.h"
#include "webcc/core/math.h"

// --- GL Constants ---
constexpr uint32_t GL_TRIANGLES = 0x0004;
constexpr uint32_t GL_DEPTH_TEST = 0x0B71;
constexpr uint32_t GL_FLOAT = 0x1406;
constexpr uint32_t GL_ARRAY_BUFFER = 0x8892;
constexpr uint32_t GL_STATIC_DRAW = 0x88E4;
constexpr uint32_t GL_COLOR_BUFFER_BIT = 0x4000;
constexpr uint32_t GL_DEPTH_BUFFER_BIT = 0x0100;

// --- Shaders ---
const char* vs_source = 
    "attribute vec3 position;\n"
    "varying float vHeight;\n"
    "varying vec2 vUv;\n"
    "uniform float uTime;\n"
    "void main() {\n"
    "  vec3 p = position;\n"
    "  // Procedural Wave Displacement\n"
    "  float d = sin(p.x * 2.0 + uTime) * cos(p.z * 2.0 + uTime) * 0.5;\n"
    "  d += sin(p.x * 5.0 + uTime * 2.3) * 0.1;\n"
    "  d += cos(p.z * 4.5 + uTime * 1.7) * 0.1;\n"
    "  p.y += d;\n"
    "  vHeight = d;\n"
    "  vUv = position.xz;\n"
    "\n"
    "  // Camera Transform\n"
    "  // 1. Translate world relative to camera\n"
    "  p.y -= 3.5;\n"
    "  p.z -= 4.0;\n"
    "  // 2. Rotate X-axis to look down\n"
    "  float angle = 0.4;\n"
    "  float c = cos(angle); float s = sin(angle);\n"
    "  float y = p.y; float z = p.z;\n"
    "  p.y = y * c - z * s;\n"
    "  p.z = y * s + z * c;\n"
    "\n"
    "  // Simple Perspective Transform\n"
    "  float zNear = 0.1; float zFar = 100.0;\n"
    "  float fov = 1.0;\n"
    "  gl_Position = vec4(p.x / fov, p.y / fov, (p.z * (zFar+zNear)/(zNear-zFar) + (2.0*zFar*zNear)/(zNear-zFar)), -p.z);\n"
    "}";

const char* fs_source = 
    "precision mediump float;\n"
    "varying float vHeight;\n"
    "varying vec2 vUv;\n"
    "void main() {\n"
    "  float grid = abs(sin(vUv.x * 20.0)) * abs(sin(vUv.y * 20.0));\n"
    "  grid = pow(1.0 - grid, 40.0);\n" // Sharp grid lines
    "  vec3 deep = vec3(0.937, 0.937, 0.937);\n"
    "  vec3 mid = vec3(0.0, 0.1, 0.5);\n"
    "  vec3 high = vec3(0.580, 0.467, 1.0);\n"
    "  float h = smoothstep(-0.5, 0.5, vHeight);\n"
    "  vec3 color = mix(deep, mid, h);\n"
    "  color = mix(color, high, smoothstep(0.25, 1.0, h));\n"
    "  gl_FragColor = vec4(color + grid * 0.2, 1.0);\n"
    "}";

// --- Grid Generation ---
const int GRID_RES = 60; // 60x60 segments = 7200 triangles
float terrain_data[GRID_RES * GRID_RES * 6 * 3]; // 2 triangles per cell
int vertex_count = 0;

void generate_grid() {
    float size = 4.0f;
    float step = size / (float)GRID_RES;
    int idx = 0;
    for(int z=0; z<GRID_RES; ++z) {
        for(int x=0; x<GRID_RES; ++x) {
            float x0 = -size/2 + x*step;
            float z0 = -size/2 + z*step;
            float x1 = x0 + step;
            float z1 = z0 + step;

            // Triangle 1
            terrain_data[idx++] = x0; terrain_data[idx++] = 0; terrain_data[idx++] = z0;
            terrain_data[idx++] = x1; terrain_data[idx++] = 0; terrain_data[idx++] = z0;
            terrain_data[idx++] = x0; terrain_data[idx++] = 0; terrain_data[idx++] = z1;
            // Triangle 2
            terrain_data[idx++] = x1; terrain_data[idx++] = 0; terrain_data[idx++] = z0;
            terrain_data[idx++] = x1; terrain_data[idx++] = 0; terrain_data[idx++] = z1;
            terrain_data[idx++] = x0; terrain_data[idx++] = 0; terrain_data[idx++] = z1;
        }
    }
    vertex_count = idx / 3;
}

webcc::handle gl, prog, vbo;
webcc::handle uTimeLoc;
float total_time = 0.0f;

void update(float time_ms) {
    total_time = time_ms / 1000.0f;

    webcc::webgl::viewport(gl, 0, 0, 1920, 1080);
    webcc::webgl::clear_color(gl, 0.02f, 0.02f, 0.05f, 1.0f);
    webcc::webgl::clear(gl, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    webcc::webgl::enable(gl, GL_DEPTH_TEST);
    webcc::webgl::use_program(gl, prog);
    
    // Pass time to shader
    webcc::webgl::uniform_1f(gl, uTimeLoc, total_time);

    webcc::webgl::bind_buffer(gl, GL_ARRAY_BUFFER, vbo);
    webcc::webgl::enable_vertex_attrib_array(gl, 0);
    webcc::webgl::vertex_attrib_pointer(gl, 0, 3, GL_FLOAT, 0, 0, 0);

    webcc::webgl::draw_arrays(gl, GL_TRIANGLES, 0, vertex_count);
    webcc::flush();
}
int main() {
    webcc::handle body = webcc::dom::get_body();
    webcc::dom::set_attribute(body, "style", "margin: 0; overflow: hidden; background-color: #05050d;");

    webcc::handle text = webcc::dom::create_element("div");
    webcc::dom::set_attribute(text, "style", 
        "position: absolute; "
        "top: 30%; left: 50%; "
        "transform: translate(-50%, -50%); "
        "font-family: 'Arial Black', sans-serif; "
        "font-size: 8vw; "
        "text-shadow: 0 5px 15px rgba(0,0,0,0.5); "
        "z-index: 2; "
        "pointer-events: none; "
        "user-select: none;");
    webcc::handle webSpan = webcc::dom::create_element("span");
    webcc::dom::set_inner_text(webSpan, "Web");
    webcc::dom::set_attribute(webSpan, "style", "color: #ffffff;");
    webcc::dom::append_child(text, webSpan);
    webcc::handle ccSpan = webcc::dom::create_element("span");
    webcc::dom::set_inner_text(ccSpan, "CC");
    webcc::dom::set_attribute(ccSpan, "style", "color: #9477ff;");
    webcc::dom::append_child(text, ccSpan);
    webcc::dom::append_child(body, text);

    webcc::handle canvas = webcc::dom::create_element("canvas");
    webcc::dom::set_attribute(canvas, "width", "1920");
    webcc::dom::set_attribute(canvas, "height", "1080");
    webcc::dom::set_attribute(canvas, "style", "width: 100vw; height: 100vh; display: block; position: absolute; top: 0; left: 0; z-index: 1;");
    webcc::dom::append_child(body, canvas);
    gl = webcc::canvas::get_context(canvas, "webgl");
    gl = webcc::canvas::get_context(canvas, "webgl");
    
    generate_grid();

    webcc::handle vs = webcc::webgl::create_shader(gl, 0x8B31, vs_source);
    webcc::handle fs = webcc::webgl::create_shader(gl, 0x8B30, fs_source);
    prog = webcc::webgl::create_program(gl);
    webcc::webgl::attach_shader(gl, prog, vs);
    webcc::webgl::attach_shader(gl, prog, fs);
    webcc::webgl::bind_attrib_location(gl, prog, 0, "position");
    webcc::webgl::link_program(gl, prog);
    webcc::flush();

    uTimeLoc = webcc::webgl::get_uniform_location(gl, prog, "uTime");

    vbo = webcc::webgl::create_buffer(gl);
    webcc::webgl::bind_buffer(gl, GL_ARRAY_BUFFER, vbo);
    webcc::webgl::buffer_data(gl, GL_ARRAY_BUFFER, (uint32_t)terrain_data, sizeof(terrain_data), GL_STATIC_DRAW);

    webcc::system::set_main_loop(update);
    webcc::flush();
    return 0;
}