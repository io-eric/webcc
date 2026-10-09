// Keyed loop rows are built fresh on every sync; morphing keeps the live row's nodes and makes them
// look like the fresh ones, so a re-render doesn't drop pointer capture, focus, hover or scroll.
// Afterwards the fresh row's handles name the live nodes and its listeners sit on them.
const __wcc_morph = {
    // listener sets kept on a node by the add_*_listener commands, with their options
    kinds: [['__wcc_ptr', undefined], ['__wcc_wh', { passive: false }], ['__wcc_drop', undefined], ['__wcc_focus', undefined]],

    item(elements, oh, nh) {
        const o = elements[oh], n = elements[nh];
        if (!o || !n || o === n) return;
        if (!this.same(o, n)) {
            o.replaceWith(n);
            this.forget(elements, o);
            return;
        }
        n.remove();
        this.node(elements, o, n);
    },

    same(o, n) {
        return o.nodeType === n.nodeType && (o.nodeType !== 1 || o.tagName === n.tagName);
    },

    // n's handle now names o; o's own handle is dead
    alias(elements, o, n) {
        if (o.__wcc_h !== undefined && elements[o.__wcc_h] === o) elements[o.__wcc_h] = undefined;
        if (n.__wcc_h !== undefined) elements[n.__wcc_h] = o;
        o.__wcc_h = n.__wcc_h;
    },

    node(elements, o, n) {
        this.alias(elements, o, n);
        if (o.nodeType !== 1) {
            if (o.nodeValue !== n.nodeValue) o.nodeValue = n.nodeValue;
            return;
        }
        for (const a of Array.from(o.attributes)) {
            if (!n.hasAttribute(a.name)) o.removeAttribute(a.name);
        }
        for (const a of n.attributes) {
            if (o.getAttribute(a.name) !== a.value) o.setAttribute(a.name, a.value);
        }
        this.listeners(o, n);

        let oc = o.firstChild;
        let nc = n.firstChild;
        while (nc) {
            const next = nc.nextSibling;
            if (oc && this.same(oc, nc)) {
                const after = oc.nextSibling;
                this.node(elements, oc, nc);
                oc = after;
            } else {
                o.insertBefore(nc, oc);
            }
            nc = next;
        }
        while (oc) {
            const next = oc.nextSibling;
            oc.remove();
            this.forget(elements, oc);
            oc = next;
        }

        // what a fresh node would show; a value the row doesn't set stays as typed
        if (o.tagName === 'SELECT' || ((o.tagName === 'INPUT' || o.tagName === 'TEXTAREA') && n.hasAttribute('value'))) {
            if (o.value !== n.value) o.value = n.value;
        }
        if (o.tagName === 'INPUT' && o.checked !== n.checked) o.checked = n.checked;
    },

    listeners(o, n) {
        for (const [k, opt] of this.kinds) {
            if (o[k] === n[k]) continue;
            if (o[k]) for (const [ev, f] of o[k]) o.removeEventListener(ev, f, opt);
            if (n[k]) for (const [ev, f] of n[k]) o.addEventListener(ev, f, opt);
            o[k] = n[k];
        }
        if (o.__wcc_sc !== n.__wcc_sc) {
            if (o.__wcc_sc) o.removeEventListener('scroll', o.__wcc_sc);
            if (n.__wcc_sc) o.addEventListener('scroll', n.__wcc_sc, { passive: true });
            o.__wcc_sc = n.__wcc_sc;
        }
        // resize reports by the handle kept on the node; observing again sends the first size to it
        const rs = window.__wcc_rs;
        if (rs && (o.__wcc_rs || n.__wcc_rs)) {
            rs.ro.unobserve(o); rs.els.delete(o);
            rs.ro.unobserve(n); rs.els.delete(n);
            o.__wcc_rs = n.__wcc_rs;
            if (o.__wcc_rs) { rs.els.add(o); rs.observe(o); }
        }
    },

    // a node that left: its handles are dead
    forget(elements, node) {
        if (node.__wcc_h !== undefined && elements[node.__wcc_h] === node) elements[node.__wcc_h] = undefined;
        for (let c = node.firstChild; c; c = c.nextSibling) this.forget(elements, c);
    },
};
