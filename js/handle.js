// a handle for a node the page made (innerHTML, the browser's editing): the one it has, or a new one
function __wcc_handle(elements, n) {
    let h = n.__wcc_h;
    if (!h || elements[h] !== n) {
        h = (window.webcc_next_id = (window.webcc_next_id || 0) + 1);
        n.__wcc_h = h;
        elements[h] = n;
    }
    return h;
}
