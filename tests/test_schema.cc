// Tests for schema.def parsing (load_defs) and the binary cache round-trip
// (save_defs_binary / load_defs_binary).
//
// load_defs() reads from a file path, so these tests write small schema
// fixtures to a temp file and parse them. This pins opcode assignment,
// parameter typing, handle(T) extraction, RET handling, and inheritance.
#include "framework.h"
#include "schema.h"
#include "utils.h"

#include <cstdio>
#include <fstream>
#include <string>

using namespace webcc;

namespace
{
    // Write `contents` to a unique temp file and return its path.
    std::string write_temp(const std::string &contents, const char *tag)
    {
        std::string path = std::string("/tmp/webcc_test_") + tag + ".def";
        std::ofstream out(path);
        out << contents;
        out.close();
        return path;
    }

    const SchemaCommand *find_cmd(const SchemaDefs &d, const std::string &name)
    {
        for (const auto &c : d.commands)
            if (c.name == name)
                return &c;
        return nullptr;
    }

    const SchemaEvent *find_event(const SchemaDefs &d, const std::string &name)
    {
        for (const auto &e : d.events)
            if (e.name == name)
                return &e;
        return nullptr;
    }
}

TEST(schema_parses_basic_command)
{
    std::string path = write_temp(
        "dom|command|CREATE_ELEMENT|create_element|string:tag RET:handle(DOMElement)|{ /*js*/ }\n",
        "basic_cmd");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    CHECK_EQ(d.commands.size(), (size_t)1);
    const SchemaCommand *c = find_cmd(d, "CREATE_ELEMENT");
    CHECK(c != nullptr);
    if (!c)
        return;
    CHECK_EQ(c->ns, std::string("dom"));
    CHECK_EQ(c->func_name, std::string("create_element"));
    CHECK_EQ(c->params.size(), (size_t)1);
    CHECK_EQ(c->params[0].type, std::string("string"));
    CHECK_EQ(c->params[0].name, std::string("tag"));
    // RET:handle(DOMElement) should populate return type + handle type, and must
    // NOT be counted as a positional parameter.
    CHECK_EQ(c->return_type, std::string("handle"));
    CHECK_EQ(c->return_handle_type, std::string("DOMElement"));
}

TEST(schema_assigns_sequential_opcodes_per_kind)
{
    std::string path = write_temp(
        "ns|command|A|fa||{}\n"
        "ns|command|B|fb||{}\n"
        "ns|command|C|fc||{}\n"
        "ns|event|E1|int32:x\n"
        "ns|event|E2|int32:y\n",
        "opcodes");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    // Commands and events each start their opcode numbering at 1.
    CHECK_EQ((int)find_cmd(d, "A")->opcode, 1);
    CHECK_EQ((int)find_cmd(d, "B")->opcode, 2);
    CHECK_EQ((int)find_cmd(d, "C")->opcode, 3);
    CHECK_EQ((int)find_event(d, "E1")->opcode, 1);
    CHECK_EQ((int)find_event(d, "E2")->opcode, 2);
}

// Command opcodes are 16 bit
TEST(schema_command_opcodes_past_255)
{
    std::string def;
    for (int i = 1; i <= 300; ++i)
        def += "ns|command|C" + std::to_string(i) + "|f" + std::to_string(i) + "||{}\n";
    std::string path = write_temp(def, "many_cmds");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    CHECK_EQ(d.commands.size(), (size_t)300);
    CHECK_EQ((int)find_cmd(d, "C255")->opcode, 255);
    CHECK_EQ((int)find_cmd(d, "C256")->opcode, 256);
    CHECK_EQ((int)find_cmd(d, "C300")->opcode, 300);

    // Survives the binary cache
    std::string cache = "/tmp/webcc_test_many_cmds.bin";
    CHECK(save_defs_binary(d, cache));
    SchemaDefs loaded;
    CHECK(load_defs_binary(loaded, cache));
    std::remove(cache.c_str());
    CHECK_EQ(loaded.commands.size(), (size_t)300);
    if (loaded.commands.size() == 300)
        CHECK_EQ((int)loaded.commands[299].opcode, 300);
}

// helper lines survive the cache
TEST(schema_parses_helpers)
{
    {
        std::ofstream js("/tmp/webcc_test_helper.js");
        js << "const __t_help = { go(h) { push_event_net_DONE(h); return blobs[h]; } };\n";
    }
    std::string path = write_temp(
        "net|helper|__t_help|webcc_test_helper.js\n"
        "net|event|DONE|handle:h\n"
        "net|command|GO|go|int32:h|{ __t_help.go(h); }\n",
        "helpers");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());
    std::remove("/tmp/webcc_test_helper.js");
    CHECK_EQ(d.helpers.size(), (size_t)1);
    if (d.helpers.empty())
        return;
    CHECK_EQ(d.helpers[0].name, std::string("__t_help"));
    CHECK(d.helpers[0].code.find("push_event_net_DONE") != std::string::npos);

    std::string cache = "/tmp/webcc_test_helpers.bin";
    CHECK(save_defs_binary(d, cache));
    SchemaDefs loaded;
    CHECK(load_defs_binary(loaded, cache));
    std::remove(cache.c_str());
    CHECK_EQ(loaded.helpers.size(), (size_t)1);
    if (!loaded.helpers.empty())
        CHECK_EQ(loaded.helpers[0].code, d.helpers[0].code);
}

// 'last' column survives the cache
TEST(schema_parses_event_last)
{
    std::string path = write_temp(
        "net|event|DATA|handle(Req):id string:data\n"
        "net|event|DONE|handle(Req):id|last\n",
        "event_last");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());
    CHECK_EQ(d.events.size(), (size_t)2);
    if (d.events.size() != 2)
        return;
    CHECK(!d.events[0].last);
    CHECK(d.events[1].last);

    std::string cache = "/tmp/webcc_test_event_last.bin";
    CHECK(save_defs_binary(d, cache));
    SchemaDefs loaded;
    CHECK(load_defs_binary(loaded, cache));
    std::remove(cache.c_str());
    CHECK(loaded.events.size() == 2 && !loaded.events[0].last && loaded.events[1].last);
}

// enum/flags groups, declared before or after use
TEST(schema_parses_groups)
{
    std::string path = write_temp(
        "gfx|event|TAP|handle(Pad):h Phase:phase Mods:mods\n"
        "gfx|command|LISTEN|listen|handle(Pad):h Mods:mods=0|{ }\n"
        "gfx|command|STATE|state|handle(Pad):h RET:Phase|{ return 0; }\n"
        "gfx|enum|Phase:uint8|DOWN MOVE UP\n"
        "keys|flags|Mods:uint32|SHIFT=1 CTRL=2\n",
        "groups");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());
    CHECK_EQ(d.groups.size(), (size_t)2);
    if (d.groups.size() != 2)
        return;
    CHECK(!d.groups[0].flags);
    CHECK_EQ(d.groups[0].values.size(), (size_t)3);
    CHECK_EQ(d.groups[0].values[2].second, std::string("2"));
    CHECK(d.groups[1].flags);
    CHECK_EQ(d.groups[1].wire, std::string("uint32"));

    CHECK(d.events.size() == 1 && d.events[0].params.size() == 3);
    if (d.events.size() == 1 && d.events[0].params.size() == 3)
    {
        CHECK_EQ(d.events[0].params[1].enum_type, std::string("gfx::Phase"));
        CHECK_EQ(d.events[0].params[1].type, std::string("uint8"));
        CHECK_EQ(d.events[0].params[2].enum_type, std::string("keys::Mods"));
        CHECK_EQ(d.events[0].params[2].type, std::string("uint32"));
    }
    const SchemaCommand *st = find_cmd(d, "STATE");
    CHECK(st && st->return_enum_type == "gfx::Phase" && st->return_type == "uint8");
}

TEST(schema_parses_bytes_return)
{
    std::string path = write_temp(
        "net|command|READ|read|handle(Blob):handle RET:bytes|{ const ret = blobs[handle]; }\n",
        "bytes_ret");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    const SchemaCommand *c = find_cmd(d, "READ");
    CHECK(c != nullptr);
    if (!c)
        return;
    CHECK_EQ(c->return_type, std::string("bytes"));
    CHECK_EQ(c->params.size(), (size_t)1);
}

TEST(schema_parses_consts_and_defaults)
{
    std::string path = write_temp(
        "ui|const|BIG|42\n"
        "ui|command|SHOW|show|handle(DOMElement):el uint8:flags=3 string:label=\"hi\"|{}\n",
        "consts");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    CHECK_EQ(d.consts.size(), (size_t)1);
    if (d.consts.size() == 1)
    {
        CHECK_EQ(d.consts[0].ns, std::string("ui"));
        CHECK_EQ(d.consts[0].name, std::string("BIG"));
        CHECK_EQ(d.consts[0].value, std::string("42"));
    }
    // Constants don't take command opcodes
    const SchemaCommand *c = find_cmd(d, "SHOW");
    CHECK(c != nullptr);
    if (!c)
        return;
    CHECK_EQ((int)c->opcode, 1);
    CHECK_EQ(c->params.size(), (size_t)3);
    CHECK_EQ(c->params[0].default_value, std::string(""));
    CHECK_EQ(c->params[1].name, std::string("flags"));
    CHECK_EQ(c->params[1].default_value, std::string("3"));
    CHECK_EQ(c->params[2].name, std::string("label"));
    CHECK_EQ(c->params[2].default_value, std::string("\"hi\""));
}

TEST(schema_extracts_handle_param_types)
{
    std::string path = write_temp(
        "canvas|command|FILL_RECT|fill_rect|handle(CanvasContext2D):handle float64:x float64:y|{}\n",
        "handle_param");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    const SchemaCommand *c = find_cmd(d, "FILL_RECT");
    CHECK(c != nullptr);
    if (!c)
        return;
    CHECK_EQ(c->params.size(), (size_t)3);
    CHECK_EQ(c->params[0].type, std::string("handle"));
    CHECK_EQ(c->params[0].handle_type, std::string("CanvasContext2D"));
    CHECK_EQ(c->params[0].name, std::string("handle"));
    CHECK_EQ(c->params[1].type, std::string("float64"));
    CHECK_EQ(c->params[2].type, std::string("float64"));
}

TEST(schema_parses_event_params)
{
    std::string path = write_temp(
        "input|event|MOUSE_DOWN|int32:button int32:x int32:y\n",
        "event_params");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    const SchemaEvent *e = find_event(d, "MOUSE_DOWN");
    CHECK(e != nullptr);
    if (!e)
        return;
    CHECK_EQ(e->params.size(), (size_t)3);
    CHECK_EQ(e->params[0].name, std::string("button"));
    CHECK_EQ(e->params[2].name, std::string("y"));
}

TEST(schema_parses_inheritance)
{
    std::string path = write_temp(
        "meta|inherit|Canvas|DOMElement\n"
        "meta|inherit|Image|DOMElement\n",
        "inherit");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    CHECK_EQ(d.handle_inheritance.size(), (size_t)2);
    CHECK_EQ(d.handle_inheritance.at("Canvas"), std::string("DOMElement"));
    CHECK_EQ(d.handle_inheritance.at("Image"), std::string("DOMElement"));
}

TEST(schema_action_preserves_pipes_in_js_body)
{
    // The JS action itself can contain '|' (logical OR). The parser must treat
    // everything after the FUNC|TYPES| separators as the verbatim action.
    std::string path = write_temp(
        "ns|command|OR|do_or|int32:a|{ return a || 0; }\n",
        "action_pipe");
    SchemaDefs d = load_defs(path);
    std::remove(path.c_str());

    const SchemaCommand *c = find_cmd(d, "OR");
    CHECK(c != nullptr);
    if (!c)
        return;
    CHECK_EQ(c->action, std::string("{ return a || 0; }"));
}

// ---- Binary cache round-trip --------------------------------------------

TEST(binary_cache_roundtrips_real_schema)
{
    // Parse the real project schema, serialize it, read it back, and confirm a
    // deep round-trip. This guards the schema.wcc.bin format that the toolchain
    // loads at runtime instead of re-parsing schema.def.
    std::string def = std::string(WEBCC_SCHEMA_DEF);
    SchemaDefs original = load_defs(def);
    CHECK(original.commands.size() > 0);
    CHECK(original.events.size() > 0);

    std::string cache = "/tmp/webcc_test_cache.bin";
    CHECK(save_defs_binary(original, cache));

    SchemaDefs loaded;
    CHECK(load_defs_binary(loaded, cache));
    std::remove(cache.c_str());

    CHECK_EQ(loaded.commands.size(), original.commands.size());
    CHECK_EQ(loaded.events.size(), original.events.size());
    CHECK_EQ(loaded.handle_inheritance.size(), original.handle_inheritance.size());

    for (size_t i = 0; i < original.commands.size(); ++i)
    {
        const auto &a = original.commands[i];
        const auto &b = loaded.commands[i];
        CHECK_EQ(a.ns, b.ns);
        CHECK_EQ(a.name, b.name);
        CHECK_EQ((int)a.opcode, (int)b.opcode);
        CHECK_EQ(a.func_name, b.func_name);
        CHECK_EQ(a.action, b.action);
        CHECK_EQ(a.return_type, b.return_type);
        CHECK_EQ(a.return_handle_type, b.return_handle_type);
        CHECK_EQ(a.params.size(), b.params.size());
        for (size_t j = 0; j < a.params.size(); ++j)
        {
            CHECK_EQ(a.params[j].type, b.params[j].type);
            CHECK_EQ(a.params[j].name, b.params[j].name);
            CHECK_EQ(a.params[j].handle_type, b.params[j].handle_type);
            CHECK_EQ(a.params[j].default_value, b.params[j].default_value);
            CHECK_EQ(a.params[j].enum_type, b.params[j].enum_type);
        }
        CHECK_EQ(a.return_enum_type, b.return_enum_type);
    }

    CHECK(original.groups.size() > 0);
    CHECK_EQ(loaded.groups.size(), original.groups.size());
    for (size_t i = 0; i < original.groups.size() && i < loaded.groups.size(); ++i)
    {
        CHECK_EQ(loaded.groups[i].name, original.groups[i].name);
        CHECK_EQ(loaded.groups[i].wire, original.groups[i].wire);
        CHECK(loaded.groups[i].flags == original.groups[i].flags);
        CHECK(loaded.groups[i].values == original.groups[i].values);
    }

    CHECK_EQ(loaded.consts.size(), original.consts.size());
    for (size_t i = 0; i < original.consts.size() && i < loaded.consts.size(); ++i)
    {
        CHECK_EQ(loaded.consts[i].ns, original.consts[i].ns);
        CHECK_EQ(loaded.consts[i].name, original.consts[i].name);
        CHECK_EQ(loaded.consts[i].value, original.consts[i].value);
    }
}

TEST(binary_cache_rejects_bad_magic)
{
    std::string path = "/tmp/webcc_test_badmagic.bin";
    std::ofstream out(path, std::ios::binary);
    const char junk[] = "not a real cache file at all";
    out.write(junk, sizeof(junk));
    out.close();

    SchemaDefs d;
    CHECK(!load_defs_binary(d, path)); // must reject, not crash
    std::remove(path.c_str());
}
