// Run with `webcc tests/host/page.cc --run`: the host page read through webcc/host.h and sent
// events, as a browser would. Exits 1 on the first check that fails.
#include "webcc/dom.h"
#include "webcc/host.h"
#include <cstdio>
#include <cstdlib>

using namespace webcc;

static int failed = 0;
#define CHECK(c) do { if (!(c)) { printf("page.cc:%d  %s\n", __LINE__, #c); failed++; } } while (0)

static DOMElement el(DOMElement parent, const char *tag, const char *cls)
{
    DOMElement e = dom::create_element(tag);
    if (*cls) dom::set_attribute(e, "class", cls);
    dom::append_child(parent, e);
    return e;
}

int main()
{
    DOMElement body = dom::get_body();
    DOMElement box = el(body, "div", "box main");
    dom::set_attribute(box, "id", "top");
    DOMElement count = el(box, "span", "count");
    dom::set_inner_text(count, "0");
    DOMElement save = el(box, "button", "save");
    DOMElement label = el(save, "b", "");
    dom::set_inner_text(label, "Save");
    DOMElement name = el(box, "input", "name");
    DOMElement check = el(box, "input", "");
    dom::set_attribute(check, "type", "checkbox");
    DOMElement list = el(body, "ul", "");
    for (int i = 0; i < 3; i++)
    {
        DOMElement li = el(list, "li", i == 1 ? "on" : "");
        dom::set_attribute(li, "data-i", i == 1 ? "1" : "x");
        dom::set_inner_text(li, i == 1 ? "b" : "a");
    }
    dom::add_click_listener(save);
    dom::add_click_listener(box);
    dom::add_click_target_listener(box);
    dom::add_input_listener(name);
    dom::add_change_listener(check);
    dom::add_keydown_listener(name);

    // selectors
    CHECK(host::query(".save") == save);
    CHECK(host::query("div.box.main > span") == count);
    CHECK(host::query("#top b") == label);
    CHECK(!host::query("#top > b").is_valid());
    CHECK(host::query("[type=checkbox]") == check);
    CHECK(host::query("li[data-i='1']") == host::query("li", 1));
    CHECK(host::query(".missing, ul li.on") == host::query("li", 1));
    CHECK(host::count("li") == 3);
    CHECK(host::count("*") == 10);
    CHECK(!host::query("li", 3).is_valid());
    CHECK(!host::query("li:first-child").is_valid());

    // reading
    CHECK(host::text(save) == "Save");
    CHECK(host::text(DOMElement(0)) == "0Saveaba");
    CHECK(host::html(list) == "<ul><li data-i=\"x\">a</li><li class=\"on\" data-i=\"1\">b</li><li data-i=\"x\">a</li></ul>");

    // a click on the label bubbles: the box's target listener, then the button's and the box's click
    CHECK(host::click(label));
    Event e;
    CHECK(poll_event(e) && e.as<dom::ClickTargetEvent>() && e.as<dom::ClickTargetEvent>()->handle == box && e.as<dom::ClickTargetEvent>()->target == label);
    CHECK(poll_event(e) && e.as<dom::ClickEvent>() && e.as<dom::ClickEvent>()->handle == save);
    CHECK(poll_event(e) && e.as<dom::ClickEvent>() && e.as<dom::ClickEvent>()->handle == box);
    CHECK(!poll_event(e));
    CHECK(!host::click(host::query("li")));

    CHECK(host::input(name, "Ada"));
    CHECK(poll_event(e) && e.as<dom::InputEvent>() && e.as<dom::InputEvent>()->value == "Ada");
    CHECK(dom::get_attribute(name, "value") == "Ada");
    CHECK(host::keydown(name, 13));
    CHECK(poll_event(e) && e.as<dom::KeydownEvent>() && e.as<dom::KeydownEvent>()->keycode == 13);

    // a checkbox flips on a click and says so with change
    CHECK(host::click(check));
    CHECK(poll_event(e) && e.as<dom::ClickTargetEvent>());
    CHECK(poll_event(e) && e.as<dom::ClickEvent>() && e.as<dom::ClickEvent>()->handle == box);
    CHECK(poll_event(e) && e.as<dom::ChangeEvent>() && e.as<dom::ChangeEvent>()->value == "true");
    CHECK(host::query("[checked]") == check);
    CHECK(host::change(check, "false"));
    CHECK(poll_event(e) && e.as<dom::ChangeEvent>() && e.as<dom::ChangeEvent>()->value == "false");
    CHECK(!host::query("[checked]").is_valid());

    host::clear();
    CHECK(host::count("*") == 0);
    CHECK(!host::click(save));

    printf(failed ? "host page: %d failed\n" : "host page: ok\n", failed);
    return failed ? 1 : 0;
}
