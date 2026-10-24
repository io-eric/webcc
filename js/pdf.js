const __wcc_pdf = {
    lib: '', worker: '', mod: null,
    docs: [], renders: [],

    msg(e) { return String((e && e.message) || e || 'failed'); },

    load() {
        if (!this.mod) {
            if (!this.lib) return Promise.reject(new Error('pdf: call set_library first'));
            const base = document.baseURI;
            this.mod = import(new URL(this.lib, base).href).then((m) => {
                m.GlobalWorkerOptions.workerSrc = new URL(this.worker, base).href;
                return m;
            });
            this.mod.catch(() => { this.mod = null; }); // retry on the next open
        }
        return this.mod;
    },

    // ---- import ----

    open(handle, bytes) {
        const entry = { doc: null, task: null, sizes: [] };
        this.docs[handle] = entry;
        // cmaps and fonts only load for PDFs that need them
        const dir = new URL('.', new URL(this.lib, document.baseURI)).href;
        this.load().then((m) => {
            entry.task = m.getDocument({ data: bytes, cMapUrl: dir + 'cmaps/', cMapPacked: true, standardFontDataUrl: dir + 'standard_fonts/' });
            return entry.task.promise;
        }).then(async (doc) => {
            if (this.docs[handle] !== entry) return; // closed while opening
            entry.doc = doc;
            for (let i = 1; i <= doc.numPages; i++) {
                const v = (await doc.getPage(i)).getViewport({ scale: 1 });
                entry.sizes.push([v.width, v.height]);
            }
            push_event_pdf_OPENED(handle, doc.numPages, '');
        }).catch((e) => {
            if (this.docs[handle] === entry) push_event_pdf_OPENED(handle, 0, this.msg(e));
        });
    },

    size(handle, index, axis) {
        const e = this.docs[handle];
        const s = e && e.sizes[index];
        return s ? s[axis] : 0;
    },

    render(req, handle, index, canvas, scale) {
        const entry = this.docs[handle];
        if (!entry || !entry.doc) { queueMicrotask(() => push_event_pdf_RENDERED(req, 0, 'document not open')); return; }
        if (!canvas || !canvas.getContext) { queueMicrotask(() => push_event_pdf_RENDERED(req, 0, 'not a canvas')); return; }
        const job = { task: null, cancelled: false };
        this.renders[req] = job;
        entry.doc.getPage(index + 1).then((page) => {
            if (job.cancelled) throw new Error('cancelled');
            const vp = page.getViewport({ scale: scale > 0 ? scale : 1 });
            canvas.width = Math.ceil(vp.width);
            canvas.height = Math.ceil(vp.height);
            job.task = page.render({ canvasContext: canvas.getContext('2d'), canvas, viewport: vp });
            return job.task.promise;
        }).then(() => {
            this.renders[req] = undefined;
            push_event_pdf_RENDERED(req, 1, '');
        }, (e) => {
            this.renders[req] = undefined;
            push_event_pdf_RENDERED(req, 0, job.cancelled ? 'cancelled' : this.msg(e));
        });
    },

    cancel(req) {
        const job = this.renders[req];
        if (!job) return;
        job.cancelled = true;
        if (job.task) job.task.cancel();
    },

    close(handle) {
        const e = this.docs[handle];
        if (!e) return;
        this.docs[handle] = undefined;
        // destroying the task frees the doc and its worker
        if (e.task) e.task.destroy();
    },
};
