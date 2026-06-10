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
        }
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
