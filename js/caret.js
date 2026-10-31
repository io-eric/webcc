// the caret's line in el's own px (from its top left corner, a scale transform divided out), null when the caret isn't in el
function __wcc_caret(el) {
    const s = window.getSelection();
    if (!el || !s || !s.rangeCount || !s.focusNode || !el.contains(s.focusNode)) return null;
    const r = document.createRange();
    r.setStart(s.focusNode, s.focusOffset);
    r.collapse(true);
    let b = r.getClientRects()[0];
    // a caret on an empty line has no rect of its own: the element there (the <br> holding the line open, or the line) stands in
    if (!b) {
        let n = s.focusNode.nodeType === 1 ? (s.focusNode.childNodes[s.focusOffset] || s.focusNode) : s.focusNode.parentElement;
        if (n.nodeType !== 1) n = n.parentElement;
        b = n.getBoundingClientRect();
        if (!b.height && n.parentElement) b = n.parentElement.getBoundingClientRect();
    }
    const e = el.getBoundingClientRect();
    const k = el.offsetWidth ? e.width / el.offsetWidth : 1;
    return { left: (b.left - e.left) / k, top: (b.top - e.top) / k, bottom: (b.bottom - e.top) / k };
}
