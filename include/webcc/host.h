// The page of a program built for the host (webcc --run, --render): read it, and send it
// events as a browser would. Only in a host build, there is no such import in wasm.
#pragma once
#include "webcc.h"
#include "webcc/core/handle.h"
#include "webcc/core/handles.h"
#include "webcc/core/string_view.h"
#include "webcc/core/string.h"

namespace webcc::host {
    extern "C" int32_t webcc_host_query(const char* selector, uint32_t len, int32_t index);
    extern "C" int32_t webcc_host_count(const char* selector, uint32_t len);
    extern "C" uint32_t webcc_host_text(int32_t handle);
    extern "C" uint32_t webcc_host_html(int32_t handle);
    extern "C" uint8_t webcc_host_click(int32_t handle);
    extern "C" uint8_t webcc_host_input(int32_t handle, const char* value, uint32_t len);
    extern "C" uint8_t webcc_host_change(int32_t handle, const char* value, uint32_t len);
    extern "C" uint8_t webcc_host_keydown(int32_t handle, int32_t keycode);
    extern "C" void webcc_host_clear();

    // the index-th element in the body matching a CSS selector, invalid when there is none
    inline DOMElement query(string_view selector, int32_t index = 0) {
        ::webcc::flush();
        return DOMElement(webcc_host_query(selector.data(), selector.length(), index));
    }

    inline int32_t count(string_view selector) {
        ::webcc::flush();
        return webcc_host_count(selector.data(), selector.length());
    }

    // textContent; the body's with handle 0
    inline string text(DOMElement el) {
        ::webcc::flush();
        return take_string_result(webcc_host_text((int32_t)el));
    }

    // outerHTML; the body's content with handle 0
    inline string html(DOMElement el) {
        ::webcc::flush();
        return take_string_result(webcc_host_html((int32_t)el));
    }

    // the events below go to the element and its ancestors' listeners, and say whether one had
    // any; the program reads them with poll_event as in a browser

    inline bool click(DOMElement el) {
        ::webcc::flush();
        return webcc_host_click((int32_t)el);
    }

    // sets the value, then input
    inline bool input(DOMElement el, string_view value) {
        ::webcc::flush();
        return webcc_host_input((int32_t)el, value.data(), value.length());
    }

    // a select's value, or "true"/"false" for a checkbox, then change
    inline bool change(DOMElement el, string_view value) {
        ::webcc::flush();
        return webcc_host_change((int32_t)el, value.data(), value.length());
    }

    inline bool keydown(DOMElement el, int32_t keycode) {
        ::webcc::flush();
        return webcc_host_keydown((int32_t)el, keycode);
    }

    // empties the body
    inline void clear() {
        ::webcc::flush();
        webcc_host_clear();
    }
}
