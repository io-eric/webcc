// The browser's install prompt can fire before the module has loaded, so it is caught here
// and kept until the app asks. state: 0 can't prompt, 1 can prompt, 2 installed (or running as one),
// 3 no prompt but the Share menu has Add to Home Screen (iPhone, iPad), 4 no prompt but Safari's
// File menu has Add to Dock (Safari 17+ on a Mac)
const __wcc_install = {
    prompt: null,
    state: (() => {
        if (matchMedia('(display-mode: standalone)').matches || navigator.standalone === true) return 2;
        const ua = navigator.userAgent;
        // an iPad asks for desktop pages and says it's a Mac; the touch points give it away
        if (/iPhone|iPad|iPod/.test(ua) || (/Macintosh/.test(ua) && navigator.maxTouchPoints > 1)) return 3;
        const v = /Macintosh.*Version\/(\d+)[.\d]* Safari/.exec(ua);
        if (v && +v[1] >= 17 && !/Chrome|Chromium|Edg|OPR|Firefox/.test(ua)) return 4;
        return 0;
    })(),
    notify: null
};
window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    __wcc_install.prompt = e;
    __wcc_install.state = 1;
    if (__wcc_install.notify) __wcc_install.notify();
});
window.addEventListener('appinstalled', () => {
    __wcc_install.prompt = null;
    __wcc_install.state = 2;
    if (__wcc_install.notify) __wcc_install.notify();
});
