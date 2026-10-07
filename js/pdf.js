// webcc pdf: import renders pages with pdf.js (loaded on first use from the URL the app
// passes to set_library), export writes a PDF from canvas-like drawing commands.
const __wcc_pdf = {
    lib: '', worker: '', mod: null,
    docs: [], renders: [], writers: [],

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
        // cmaps/ and standard_fonts/ from pdfjs-dist, next to the library, are only fetched
        // for PDFs that need them (Asian text, fonts not embedded in the file)
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
        // the loading task owns the document and its worker side (doc.destroy is gone in pdf.js 6)
        if (e.task) e.task.destroy();
    },

    // ---- export ----
    // Coordinates are PDF points (1/72 inch), origin top-left, y down like a canvas.

    num(x) { return String(+(+x).toFixed(3)); },

    create(handle) {
        this.writers[handle] = { pages: [], cur: null, x: 0, y: 0, gstates: new Map(), images: [], imageIds: new Map(), font: false };
    },

    op(handle, s) {
        const w = this.writers[handle];
        if (w && w.cur) w.cur.ops.push(s);
        return w;
    },

    page(handle, width, height) {
        const w = this.writers[handle];
        if (!w) return;
        w.cur = { w: width, h: height, ops: ['1 0 0 -1 0 ' + this.num(height) + ' cm'] };
        w.pages.push(w.cur);
    },

    color(handle, r, g, b, a, fill) {
        const w = this.writers[handle];
        if (!w || !w.cur) return;
        const n = this.num;
        w.cur.ops.push(n(r / 255) + ' ' + n(g / 255) + ' ' + n(b / 255) + (fill ? ' rg' : ' RG'));
        const key = (fill ? 'f' : 's') + n(Math.min(1, Math.max(0, a)));
        if (!w.gstates.has(key)) w.gstates.set(key, 'G' + w.gstates.size);
        w.cur.ops.push('/' + w.gstates.get(key) + ' gs');
    },

    move(handle, x, y) { const w = this.op(handle, this.num(x) + ' ' + this.num(y) + ' m'); if (w) { w.x = x; w.y = y; } },
    line(handle, x, y) { const w = this.op(handle, this.num(x) + ' ' + this.num(y) + ' l'); if (w) { w.x = x; w.y = y; } },
    curve(handle, c1x, c1y, c2x, c2y, x, y) {
        const n = this.num;
        const w = this.op(handle, [c1x, c1y, c2x, c2y, x, y].map(n).join(' ') + ' c');
        if (w) { w.x = x; w.y = y; }
    },
    quad(handle, cx, cy, x, y) {
        const w = this.writers[handle];
        if (!w) return;
        // quadratic to cubic: control points 2/3 of the way to the quadratic one
        this.curve(handle, w.x + 2 / 3 * (cx - w.x), w.y + 2 / 3 * (cy - w.y), x + 2 / 3 * (cx - x), y + 2 / 3 * (cy - y), x, y);
    },

    image(handle, src, x, y, width, height) {
        const w = this.writers[handle];
        if (!w || !w.cur || !src) return;
        const sw = src.naturalWidth ?? src.width, sh = src.naturalHeight ?? src.height;
        if (!(sw > 0 && sh > 0)) return;
        // Pixels are copied now: the source may change or be freed before finish.
        // An <img> can't change, so drawing it again reuses the copy.
        let id = src instanceof HTMLImageElement ? w.imageIds.get(src) : undefined;
        if (id === undefined) {
            const c = document.createElement('canvas');
            c.width = sw; c.height = sh;
            c.getContext('2d').drawImage(src, 0, 0);
            id = w.images.length;
            w.images.push(c);
            if (src instanceof HTMLImageElement) w.imageIds.set(src, id);
        }
        const n = this.num;
        w.cur.ops.push('q ' + n(width) + ' 0 0 ' + n(-height) + ' ' + n(x) + ' ' + n(y + height) + ' cm /Im' + id + ' Do Q');
    },

    text(handle, str, x, y, size) {
        const w = this.writers[handle];
        if (!w || !w.cur) return;
        w.font = true;
        // Built-in Helvetica covers Latin-1 (WinAnsi); other characters become '?'
        let s = '';
        for (const ch of str) {
            const c = ch.codePointAt(0);
            const b = c < 256 && c >= 32 ? ch : '?';
            s += b === '\\' || b === '(' || b === ')' ? '\\' + b : b;
        }
        const n = this.num;
        w.cur.ops.push('BT /Helv ' + n(size) + ' Tf 1 0 0 -1 ' + n(x) + ' ' + n(y) + ' Tm (' + s + ') Tj ET');
    },

    async deflate(u8) {
        const stream = new Blob([u8]).stream().pipeThrough(new CompressionStream('deflate'));
        return new Uint8Array(await new Response(stream).arrayBuffer());
    },

    latin1(s) {
        const u8 = new Uint8Array(s.length);
        for (let i = 0; i < s.length; i++) u8[i] = s.charCodeAt(i);
        return u8;
    },

    // Opaque images as JPEG, images with transparency as deflated RGB + alpha mask
    async encodeImage(c) {
        const px = c.getContext('2d').getImageData(0, 0, c.width, c.height).data;
        let opaque = true;
        for (let i = 3; i < px.length; i += 4) if (px[i] !== 255) { opaque = false; break; }
        const dims = '/Width ' + c.width + ' /Height ' + c.height + ' /BitsPerComponent 8';
        if (opaque) {
            const blob = await new Promise((res) => c.toBlob(res, 'image/jpeg', 0.92));
            return { dict: '/Type /XObject /Subtype /Image ' + dims + ' /ColorSpace /DeviceRGB /Filter /DCTDecode', data: new Uint8Array(await blob.arrayBuffer()) };
        }
        const n = c.width * c.height;
        const rgb = new Uint8Array(n * 3), alpha = new Uint8Array(n);
        for (let i = 0; i < n; i++) {
            rgb[i * 3] = px[i * 4]; rgb[i * 3 + 1] = px[i * 4 + 1]; rgb[i * 3 + 2] = px[i * 4 + 2];
            alpha[i] = px[i * 4 + 3];
        }
        return {
            dict: '/Type /XObject /Subtype /Image ' + dims + ' /ColorSpace /DeviceRGB /Filter /FlateDecode',
            data: await this.deflate(rgb),
            mask: { dict: '/Type /XObject /Subtype /Image ' + dims + ' /ColorSpace /DeviceGray /Filter /FlateDecode', data: await this.deflate(alpha) },
        };
    },

    async build(w) {
        const objs = []; // [dict string, stream bytes or null], object number = index + 1
        const add = (dict, data) => { objs.push([dict, data || null]); return objs.length; };
        const ref = (n) => n + ' 0 R';
        add('<< /Type /Catalog /Pages 2 0 R >>');
        add(null); // pages, filled in below

        const res = [];
        if (w.font) res.push('/Font << /Helv ' + ref(add('<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>')) + ' >>');
        if (w.gstates.size) {
            let gs = '/ExtGState <<';
            for (const [key, name] of w.gstates) gs += ' /' + name + ' ' + ref(add('<< /Type /ExtGState /' + (key[0] === 'f' ? 'ca ' : 'CA ') + key.slice(1) + ' >>'));
            res.push(gs + ' >>');
        }
        if (w.images.length) {
            let xo = '/XObject <<';
            for (let i = 0; i < w.images.length; i++) {
                const img = await this.encodeImage(w.images[i]);
                let dict = img.dict;
                if (img.mask) dict += ' /SMask ' + ref(add('<< ' + img.mask.dict + ' >>', img.mask.data));
                xo += ' /Im' + i + ' ' + ref(add('<< ' + dict + ' >>', img.data));
            }
            res.push(xo + ' >>');
        }
        const resources = '<< ' + res.join(' ') + ' >>';

        const kids = [];
        for (const p of w.pages) {
            const content = ref(add('<< /Filter /FlateDecode >>', await this.deflate(this.latin1(p.ops.join('\n')))));
            kids.push(ref(add('<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ' + this.num(p.w) + ' ' + this.num(p.h) + '] /Resources ' + resources + ' /Contents ' + content + ' >>')));
        }
        objs[1][0] = '<< /Type /Pages /Kids [' + kids.join(' ') + '] /Count ' + kids.length + ' >>';

        const parts = [this.latin1('%PDF-1.4\n%\xe2\xe3\xcf\xd3\n')];
        let pos = parts[0].length;
        const offsets = [];
        objs.forEach(([dict, data], i) => {
            offsets.push(pos);
            const head = this.latin1((i + 1) + ' 0 obj\n' + (data ? dict.replace(/ >>$/, ' /Length ' + data.length + ' >>') + '\nstream\n' : dict + '\n'));
            parts.push(head); pos += head.length;
            if (data) {
                parts.push(data); pos += data.length;
                const end = this.latin1('\nendstream\n');
                parts.push(end); pos += end.length;
            }
            const end = this.latin1('endobj\n');
            parts.push(end); pos += end.length;
        });
        let xref = 'xref\n0 ' + (objs.length + 1) + '\n0000000000 65535 f \n';
        for (const o of offsets) xref += String(o).padStart(10, '0') + ' 00000 n \n';
        xref += 'trailer\n<< /Size ' + (objs.length + 1) + ' /Root 1 0 R >>\nstartxref\n' + pos + '\n%%EOF\n';
        parts.push(this.latin1(xref));

        const out = new Uint8Array(parts.reduce((s, p) => s + p.length, 0));
        let o = 0;
        for (const p of parts) { out.set(p, o); o += p.length; }
        return out;
    },

    finish(handle) {
        const w = this.writers[handle];
        if (!w) { queueMicrotask(() => push_event_pdf_WRITTEN(handle, -1, 'unknown writer')); return; }
        this.writers[handle] = undefined;
        this.build(w).then((bytes) => {
            const b = (window.webcc_next_id = (window.webcc_next_id || 0) + 1);
            blobs[b] = bytes;
            push_event_pdf_WRITTEN(handle, b, '');
        }, (e) => push_event_pdf_WRITTEN(handle, -1, this.msg(e)));
    },
};
