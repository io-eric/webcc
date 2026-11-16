// localStorage, or one in memory for this page where the browser gives none (null in a WebView with
// DOM storage off, a throw when site data is blocked)
const __wcc_store = (() => {
    try { if (localStorage) return localStorage; } catch (e) {}
    const m = new Map();
    return { getItem: k => m.has(k) ? m.get(k) : null, setItem: (k, v) => m.set(k, String(v)),
             removeItem: k => m.delete(k), clear: () => m.clear() };
})();
