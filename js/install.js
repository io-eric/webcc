// The browser's install prompt can fire before the module has loaded, so it is caught here
// and kept until the app asks. state: 0 can't prompt, 1 can prompt, 2 installed (or running as one)
const __wcc_install = {
    prompt: null,
    state: (matchMedia('(display-mode: standalone)').matches || navigator.standalone === true) ? 2 : 0,
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
