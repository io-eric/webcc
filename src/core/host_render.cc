// First render on the host, for static HTML: the app is compiled natively with this file in
// place of the JS runtime. Commands that build the page are applied to a small tree here, the
// rest are skipped, and when the program ends the tree is printed as HTML.
//
// Compiled together with the app and the core sources by `webcc --render`, which generates two
// files from the schema: host_layouts.inc, the parameter layout of every command (to step over
// the ones that don't matter here), and host_stubs.cc, a weak default for every import with a
// result, so a page may call anything and only the DOM counts.

#include "webcc/dom.h"
#include "webcc/system.h"
#include "host_layouts.inc"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" uint8_t *webcc_scratch_buffer_ptr();
extern "C" uint32_t webcc_scratch_buffer_capacity();

namespace
{
    enum Kind { ELEMENT, TEXT, COMMENT };

    struct Node
    {
        Kind kind;
        std::string tag;                                        // element
        std::vector<std::pair<std::string, std::string>> attrs; // element, in the order they were set
        std::vector<Node *> children;
        Node *parent = nullptr;
        std::string text;      // text, comment
        std::string raw_html;  // element: innerHTML as given, in place of children
        bool has_raw = false;
        int32_t handle = -1;
    };

    std::unordered_map<int32_t, Node *> g_nodes;
    Node *g_body = nullptr;
    int32_t g_next_handle = 1;
    size_t g_mutations = 0;      // tree changes, to tell a frame that did something
    uint32_t g_update_low = 0;   // the update function, as the 32-bit value the app sent
    bool g_update_set = false;
    bool g_frame_requested = false;
    std::string g_big_result;    // a returned string too big for the scratch buffer

    Node *body()
    {
        if (!g_body)
        {
            g_body = new Node{ELEMENT};
            g_body->tag = "body";
            g_body->handle = 0;
            g_nodes[0] = g_body;
        }
        return g_body;
    }

    Node *get(int32_t h)
    {
        if (h == 0) return body();
        auto it = g_nodes.find(h);
        return it == g_nodes.end() ? nullptr : it->second;
    }

    Node *make(Kind k, int32_t h)
    {
        Node *n = new Node{k};
        n->handle = h;
        g_nodes[h] = n;
        return n;
    }

    void detach(Node *n)
    {
        if (!n->parent) return;
        auto &c = n->parent->children;
        for (size_t i = 0; i < c.size(); i++)
            if (c[i] == n) { c.erase(c.begin() + i); break; }
        n->parent = nullptr;
        g_mutations++;
    }

    // before ref, or last when ref is not a child of parent
    void insert(Node *parent, Node *child, Node *ref)
    {
        if (!parent || !child || child == parent) return;
        detach(child);
        auto &c = parent->children;
        size_t at = c.size();
        if (ref)
            for (size_t i = 0; i < c.size(); i++)
                if (c[i] == ref) { at = i; break; }
        c.insert(c.begin() + at, child);
        child->parent = parent;
        g_mutations++;
    }

    const std::string *attr(const Node *n, const std::string &name)
    {
        for (auto &a : n->attrs)
            if (a.first == name) return &a.second;
        return nullptr;
    }

    void set_attr(Node *n, const std::string &name, const std::string &value)
    {
        for (auto &a : n->attrs)
            if (a.first == name) { a.second = value; g_mutations++; return; }
        n->attrs.push_back({name, value});
        g_mutations++;
    }

    void remove_attr(Node *n, const std::string &name)
    {
        for (size_t i = 0; i < n->attrs.size(); i++)
            if (n->attrs[i].first == name) { n->attrs.erase(n->attrs.begin() + i); g_mutations++; return; }
    }

    void set_text(Node *n, const std::string &text)
    {
        for (Node *c : n->children) c->parent = nullptr;
        n->children.clear();
        n->has_raw = false;
        if (!text.empty())
        {
            Node *t = new Node{TEXT};
            t->text = text;
            t->parent = n;
            n->children.push_back(t);
        }
        g_mutations++;
    }

    std::vector<std::string> classes(const Node *n)
    {
        std::vector<std::string> out;
        const std::string *v = attr(n, "class");
        if (!v) return out;
        std::string cur;
        for (char c : *v)
        {
            if (c == ' ' || c == '\t' || c == '\n') { if (!cur.empty()) out.push_back(cur); cur.clear(); }
            else cur += c;
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    }

    void set_classes(Node *n, const std::vector<std::string> &cs)
    {
        std::string v;
        for (auto &c : cs) { if (!v.empty()) v += ' '; v += c; }
        set_attr(n, "class", v);
    }

    void set_style(Node *n, const std::string &name, const std::string &value)
    {
        // "a: b; c: d" taken apart, one property changed, put back
        std::vector<std::pair<std::string, std::string>> props;
        const std::string *s = attr(n, "style");
        if (s)
        {
            size_t i = 0;
            while (i < s->size())
            {
                size_t end = s->find(';', i);
                if (end == std::string::npos) end = s->size();
                std::string part = s->substr(i, end - i);
                size_t colon = part.find(':');
                if (colon != std::string::npos)
                {
                    auto trim = [](std::string t) {
                        size_t a = t.find_first_not_of(" \t"), b = t.find_last_not_of(" \t");
                        return a == std::string::npos ? std::string() : t.substr(a, b - a + 1);
                    };
                    props.push_back({trim(part.substr(0, colon)), trim(part.substr(colon + 1))});
                }
                i = end + 1;
            }
        }
        bool found = false;
        for (size_t i = 0; i < props.size(); i++)
        {
            if (props[i].first != name) continue;
            found = true;
            if (value.empty()) props.erase(props.begin() + i);
            else props[i].second = value;
            break;
        }
        if (!found && !value.empty()) props.push_back({name, value});
        std::string out;
        for (auto &p : props) { if (!out.empty()) out += "; "; out += p.first + ": " + p.second; }
        if (out.empty()) remove_attr(n, "style");
        else set_attr(n, "style", out);
    }

    // keyed loop rows: the live node o takes on the fresh node n, and n's handle names o from now on
    bool same(const Node *o, const Node *n) { return o->kind == n->kind && (o->kind != ELEMENT || o->tag == n->tag); }

    void morph_node(Node *o, Node *n)
    {
        if (o->handle >= 0 && g_nodes[o->handle] == o) g_nodes.erase(o->handle);
        if (n->handle >= 0) g_nodes[n->handle] = o;
        o->handle = n->handle;
        if (o->kind != ELEMENT)
        {
            if (o->text != n->text) { o->text = n->text; g_mutations++; }
            return;
        }
        for (size_t i = 0; i < o->attrs.size();)
        {
            if (!attr(n, o->attrs[i].first)) o->attrs.erase(o->attrs.begin() + i);
            else i++;
        }
        for (auto &a : n->attrs)
        {
            const std::string *v = attr(o, a.first);
            if (!v || *v != a.second) set_attr(o, a.first, a.second);
        }
        if (n->has_raw) { o->has_raw = true; o->raw_html = n->raw_html; }
        size_t oi = 0;
        std::vector<Node *> fresh = n->children;
        for (Node *nc : fresh)
        {
            if (oi < o->children.size() && same(o->children[oi], nc))
            {
                morph_node(o->children[oi], nc);
                oi++;
            }
            else
            {
                Node *ref = oi < o->children.size() ? o->children[oi] : nullptr;
                insert(o, nc, ref);
                oi++;
            }
        }
        while (o->children.size() > oi) detach(o->children.back());
    }

    void morph(Node *o, Node *n)
    {
        if (!o || !n || o == n) return;
        if (!same(o, n))
        {
            Node *parent = o->parent;
            if (parent)
            {
                size_t at = 0;
                for (; at < parent->children.size(); at++) if (parent->children[at] == o) break;
                Node *after = at + 1 < parent->children.size() ? parent->children[at + 1] : nullptr;
                detach(o);
                insert(parent, n, after);
            }
            return;
        }
        detach(n);
        morph_node(o, n);
    }

    // --- the command stream ---------------------------------------------------------------

    struct Reader
    {
        const uint8_t *p, *end;
        bool ok = true;
        uint32_t u32() { if (p + 4 > end) { ok = false; return 0; } uint32_t v; memcpy(&v, p, 4); p += 4; return v; }
        int32_t i32() { return (int32_t)u32(); }
        float f32() { if (p + 4 > end) { ok = false; return 0; } float v; memcpy(&v, p, 4); p += 4; return v; }
        double f64()
        {
            uintptr_t a = (uintptr_t)p;
            if (a % 8) p += 8 - a % 8;
            if (p + 8 > end) { ok = false; return 0; }
            double v; memcpy(&v, p, 8); p += 8; return v;
        }
        std::string str()
        {
            uint32_t len = u32();
            if (!ok || p + len > end) { ok = false; return {}; }
            std::string s((const char *)p, len);
            p += (len + 3) & ~3u;
            return s;
        }
        // a command whose meaning doesn't matter here, stepped over by its layout
        void skip(const char *layout)
        {
            for (; *layout && ok; layout++)
                switch (*layout)
                {
                case 'i': u32(); break;
                case 'f': f32(); break;
                case 'd': f64(); break;
                case 's': str(); break;
                }
        }
    };

    void set_property(Node *n, const std::string &name, const std::string &value)
    {
        if (name == "className") set_attr(n, "class", value);
        else if (name == "textContent" || name == "innerText") set_text(n, value);
        else if (name == "innerHTML") { set_text(n, ""); n->has_raw = true; n->raw_html = value; }
        else if (name == "checked" || name == "disabled" || name == "selected" || name == "hidden" || name == "open")
        {
            if (value == "true" || value == "1") set_attr(n, name, "");
            else remove_attr(n, name);
        }
        else set_attr(n, name, value);
    }

    void run(Reader &r)
    {
        using namespace webcc::dom;
        while (r.p < r.end && r.ok)
        {
            uint32_t op = r.u32();
            if (!r.ok) break;
            switch (op)
            {
            case OP_CREATE_ELEMENT_DEFERRED: { int32_t h = r.i32(); std::string tag = r.str(); make(ELEMENT, h)->tag = tag; break; }
            case OP_CREATE_ELEMENT_DEFERRED_SCOPED:
            {
                int32_t h = r.i32(); std::string tag = r.str(), scope = r.str();
                Node *n = make(ELEMENT, h); n->tag = tag; n->attrs.push_back({"coi-scope", scope});
                break;
            }
            case OP_CREATE_COMMENT_DEFERRED: { int32_t h = r.i32(); make(COMMENT, h)->text = r.str(); break; }
            case OP_CREATE_TEXT_NODE_DEFERRED: { int32_t h = r.i32(); make(TEXT, h)->text = r.str(); break; }
            case OP_SET_NODE_VALUE: { Node *n = get(r.i32()); std::string t = r.str(); if (n && n->kind != ELEMENT) { n->text = t; g_mutations++; } break; }
            case OP_SET_ATTRIBUTE: { Node *n = get(r.i32()); std::string k = r.str(), v = r.str(); if (n && n->kind == ELEMENT) set_attr(n, k, v); break; }
            case OP_SET_PROPERTY: { Node *n = get(r.i32()); std::string k = r.str(), v = r.str(); if (n && n->kind == ELEMENT) set_property(n, k, v); break; }
            case OP_SET_STYLE: { Node *n = get(r.i32()); std::string k = r.str(), v = r.str(); if (n && n->kind == ELEMENT) set_style(n, k, v); break; }
            case OP_APPEND_CHILD: { Node *p = get(r.i32()); Node *c = get(r.i32()); if (p && c) insert(p, c, nullptr); break; }
            case OP_INSERT_BEFORE:
            case OP_MOVE_BEFORE:
            case OP_PLACE:
            {
                Node *p = get(r.i32()); Node *c = get(r.i32()); int32_t rh = r.i32();
                Node *ref = rh > 0 ? get(rh) : nullptr;
                if (p && c && (op != OP_PLACE || c->parent != p || (ref ? true : c != p->children.back())))
                    insert(p, c, ref && ref->parent == p ? ref : nullptr);
                break;
            }
            case OP_REMOVE_ELEMENT: { int32_t h = r.i32(); Node *n = get(h); if (n && h != 0) { detach(n); g_nodes.erase(h); } break; }
            case OP_DETACH: { Node *n = get(r.i32()); if (n && n != body()) detach(n); break; }
            case OP_MORPH: { Node *o = get(r.i32()); Node *n = get(r.i32()); morph(o, n); break; }
            case OP_SET_INNER_HTML: { Node *n = get(r.i32()); std::string h = r.str(); if (n) { set_text(n, ""); n->has_raw = true; n->raw_html = h; } break; }
            case OP_SET_INNER_TEXT: { Node *n = get(r.i32()); std::string t = r.str(); if (n) set_text(n, t); break; }
            case OP_ADD_CLASS:
            {
                Node *n = get(r.i32()); std::string c = r.str();
                if (!n) break;
                auto cs = classes(n);
                bool has = false;
                for (auto &x : cs) if (x == c) has = true;
                if (!has) { cs.push_back(c); set_classes(n, cs); }
                break;
            }
            case OP_REMOVE_CLASS:
            {
                Node *n = get(r.i32()); std::string c = r.str();
                if (!n) break;
                auto cs = classes(n);
                for (size_t i = 0; i < cs.size();) { if (cs[i] == c) cs.erase(cs.begin() + i); else i++; }
                set_classes(n, cs);
                break;
            }
            case webcc::system::OP_SET_UPDATE: g_update_low = r.u32(); g_update_set = true; break;
            case webcc::system::OP_REQUEST_FRAME: g_frame_requested = true; break;
            default:
            {
                const char *layout = op < kLayoutCount ? kLayouts[op] : nullptr;
                if (!layout)
                {
                    fprintf(stderr, "render: unknown command %u in the stream\n", op);
                    r.ok = false;
                    break;
                }
                r.skip(layout);
                break;
            }
            }
        }
        if (!r.ok)
        {
            fprintf(stderr, "render: the command stream could not be read\n");
            exit(1);
        }
    }

    // --- output ---------------------------------------------------------------------------

    bool is_void(const std::string &tag)
    {
        static const char *const v[] = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "source", "track", "wbr"};
        for (const char *t : v) if (tag == t) return true;
        return false;
    }

    void esc_text(const std::string &s, std::string &out)
    {
        for (char c : s)
            switch (c)
            {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            default: out += c;
            }
    }

    void esc_attr(const std::string &s, std::string &out)
    {
        for (char c : s)
            switch (c)
            {
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
            }
    }

    void print(const Node *n, std::string &out)
    {
        switch (n->kind)
        {
        case TEXT: esc_text(n->text, out); return;
        case COMMENT: out += "<!--" + n->text + "-->"; return;
        case ELEMENT: break;
        }
        out += '<' + n->tag;
        for (auto &a : n->attrs)
        {
            // runtime bookkeeping, not page content
            if (a.first == "data-c" || a.first == "data-i" || a.first == "data-g" || a.first == "data-k") continue;
            out += ' ' + a.first;
            if (!a.second.empty()) { out += "=\""; esc_attr(a.second, out); out += '"'; }
        }
        out += '>';
        if (is_void(n->tag)) return;
        if (n->has_raw) out += n->raw_html;
        else for (const Node *c : n->children) print(c, out);
        out += "</" + n->tag + '>';
    }

    // the update function's pointer, from the 32 bits the app sent: the code sits near this file's
    // own functions, so the upper bits are theirs
    void (*update_fn())()
    {
        if (!g_update_set) return nullptr;
        uintptr_t ref = (uintptr_t)&body;
        uintptr_t base = ref & ~(uintptr_t)0xffffffffu;
        uintptr_t best = base | g_update_low;
        uintptr_t alt[2] = {best + 0x100000000ull, best >= 0x100000000ull ? best - 0x100000000ull : best};
        for (uintptr_t a : alt)
        {
            uintptr_t d1 = best > ref ? best - ref : ref - best;
            uintptr_t d2 = a > ref ? a - ref : ref - a;
            if (d2 < d1) best = a;
        }
        return (void (*)())best;
    }

    // frames the page asked for, until one changes nothing; then the page
    __attribute__((destructor)) void finish()
    {
        void (*update)() = update_fn();
        for (int i = 0; i < 8 && update && g_frame_requested; i++)
        {
            g_frame_requested = false;
            size_t before = g_mutations;
            update();
            if (g_mutations == before) break;
        }
        std::string out;
        for (const Node *c : body()->children) print(c, out);
        fwrite(out.data(), 1, out.size(), stdout);
        fflush(stdout);
    }

    uint32_t give_string(const std::string &s)
    {
        if (s.size() <= webcc_scratch_buffer_capacity())
            memcpy(webcc_scratch_buffer_ptr(), s.data(), s.size());
        else
            g_big_result = s;
        return (uint32_t)s.size();
    }
} // namespace

extern "C" void webcc_js_flush(uintptr_t ptr, size_t size)
{
    Reader r{(const uint8_t *)ptr, (const uint8_t *)ptr + size};
    run(r);
}

extern "C" void webcc_js_read_result(void *dst)
{
    memcpy(dst, g_big_result.data(), g_big_result.size());
}

// the clock, as a page would see it
extern "C" double webcc_system_get_date_now()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

// the DOM imports with a result: these build or read the tree
extern "C" int32_t webcc_dom_get_body() { body(); return 0; }
extern "C" int32_t webcc_dom_get_element_by_id(const char *, uint32_t) { return -1; }
extern "C" int32_t webcc_dom_create_element(const char *tag, uint32_t len)
{
    int32_t h = g_next_handle++;
    make(ELEMENT, h)->tag = std::string(tag, len);
    return h;
}
extern "C" int32_t webcc_dom_create_element_scoped(const char *tag, uint32_t len, const char *scope, uint32_t slen)
{
    int32_t h = g_next_handle++;
    Node *n = make(ELEMENT, h);
    n->tag = std::string(tag, len);
    n->attrs.push_back({"coi-scope", std::string(scope, slen)});
    return h;
}
extern "C" int32_t webcc_dom_create_comment(const char *text, uint32_t len)
{
    int32_t h = g_next_handle++;
    make(COMMENT, h)->text = std::string(text, len);
    return h;
}
extern "C" int32_t webcc_dom_create_text_node(const char *text, uint32_t len)
{
    int32_t h = g_next_handle++;
    make(TEXT, h)->text = std::string(text, len);
    return h;
}
extern "C" uint32_t webcc_dom_get_attribute(int32_t h, const char *name, uint32_t len)
{
    Node *n = get(h);
    const std::string *v = n ? attr(n, std::string(name, len)) : nullptr;
    return give_string(v ? *v : std::string());
}
extern "C" uint32_t webcc_dom_get_property(int32_t h, const char *name, uint32_t len)
{
    Node *n = get(h);
    if (!n) return give_string("");
    std::string k(name, len);
    if (k == "className") { const std::string *v = attr(n, "class"); return give_string(v ? *v : ""); }
    if (k == "textContent" || k == "innerText")
    {
        std::string t;
        for (const Node *c : n->children) if (c->kind == TEXT) t += c->text;
        return give_string(t);
    }
    if (k == "tagName") { std::string t = n->tag; for (char &c : t) c = (char)toupper((unsigned char)c); return give_string(t); }
    const std::string *v = attr(n, k);
    return give_string(v ? *v : "");
}
