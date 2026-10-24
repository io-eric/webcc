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

    // ---- export ----
    // points, origin top-left, y down

    num(x) { return String(+(+x).toFixed(3)); },

    create(handle) {
        this.writers[handle] = { pages: [], cur: null, x: 0, y: 0, gstates: new Map(), images: [], imageIds: new Map(), font: false,
                                 fonts: new Map(), fontName: '' };
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
        // copy pixels now, the source may change before finish
        // an <img> can't change, reuse its copy
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

    // a TrueType font for draw_text, by name; more files under one name are tried in order for
    // each character (a Latin file, then an extended one). Fetched when the writer finishes
    addFont(handle, name, url) {
        const w = this.writers[handle];
        if (!w || !name || !url) return;
        if (!w.fonts.has(name)) w.fonts.set(name, []);
        w.fonts.get(name).push(url);
    },
    setFont(handle, name) {
        const w = this.writers[handle];
        if (w) w.fontName = w.fonts.has(name) ? name : '';
    },

    text(handle, str, x, y, size) {
        const w = this.writers[handle];
        if (!w || !w.cur) return;
        // in an added font: the glyphs are picked once the files are in (build)
        const op = { font: w.fontName, str: String(str), x, y, size };
        w.cur.ops.push(w.fontName ? op : this.helv(w, op));
    },

    // Helvetica/WinAnsi, in every reader: Latin-1 only, the rest becomes '?'
    helv(w, op) {
        w.font = true;
        let s = '';
        for (const ch of op.str) {
            const c = ch.codePointAt(0);
            const b = c < 256 && c >= 32 ? ch : '?';
            s += b === '\\' || b === '(' || b === ')' ? '\\' + b : b;
        }
        const n = this.num;
        return 'BT /Helv ' + n(op.size) + ' Tf 1 0 0 -1 ' + n(op.x) + ' ' + n(op.y) + ' Tm (' + s + ') Tj ET';
    },

    // ---- TrueType: read a font, pick glyphs, cut it down to them ----

    ttf(buf) {
        const dv = new DataView(buf);
        const u16 = (o) => dv.getUint16(o), i16 = (o) => dv.getInt16(o), u32 = (o) => dv.getUint32(o);
        const tables = {};
        for (let i = 0, n = u16(4); i < n; i++) {
            const r = 12 + i * 16;
            tables[String.fromCharCode(dv.getUint8(r), dv.getUint8(r + 1), dv.getUint8(r + 2), dv.getUint8(r + 3))] = { off: u32(r + 8), len: u32(r + 12) };
        }
        for (const t of ['head', 'hhea', 'hmtx', 'maxp', 'loca', 'glyf', 'cmap']) if (!tables[t]) throw new Error('not a TrueType font (no ' + t + ')');
        const head = tables.head.off, hhea = tables.hhea.off;
        const f = {
            buf, dv, tables,
            upem: u16(head + 18), bbox: [i16(head + 36), i16(head + 38), i16(head + 40), i16(head + 42)],
            longLoca: i16(head + 50) === 1, ascent: i16(hhea + 4), descent: i16(hhea + 6),
            nMetrics: u16(hhea + 34), nGlyphs: u16(tables.maxp.off + 4), capHeight: 0, italic: 0, cmap: null,
        };
        const os2 = tables['OS/2'];
        f.capHeight = os2 && u16(os2.off) >= 2 && os2.len >= 90 ? i16(os2.off + 88) : f.ascent;
        if (tables.post) f.italic = dv.getInt32(tables.post.off + 4) / 65536;
        // the Unicode subtable: format 12 (all planes) before format 4 (the first one)
        const cm = tables.cmap.off;
        let best = -1, bestFmt = 0;
        for (let i = 0, n = u16(cm + 2); i < n; i++) {
            const pid = u16(cm + 4 + i * 8), eid = u16(cm + 6 + i * 8), off = cm + u32(cm + 8 + i * 8);
            const fmt = u16(off);
            if ((pid === 3 && (eid === 1 || eid === 10)) || pid === 0) {
                if (fmt === 12 && bestFmt !== 12) { best = off; bestFmt = 12; }
                else if (fmt === 4 && !bestFmt) { best = off; bestFmt = 4; }
            }
        }
        f.cmap = { off: best, fmt: bestFmt };
        return f;
    },

    glyphOf(f, cp) {
        const { dv } = f, o = f.cmap.off;
        if (o < 0) return 0;
        if (f.cmap.fmt === 12) {
            for (let i = 0, n = dv.getUint32(o + 12); i < n; i++) {
                const g = o + 16 + i * 12, start = dv.getUint32(g), end = dv.getUint32(g + 4);
                if (cp >= start && cp <= end) return dv.getUint32(g + 8) + cp - start;
            }
            return 0;
        }
        if (cp > 0xffff) return 0;
        const seg2 = dv.getUint16(o + 6), ends = o + 14, starts = ends + seg2 + 2, deltas = starts + seg2, ranges = deltas + seg2;
        for (let i = 0; i < seg2; i += 2) {
            if (cp > dv.getUint16(ends + i)) continue;
            const start = dv.getUint16(starts + i);
            if (cp < start) return 0;
            const delta = dv.getInt16(deltas + i), ro = dv.getUint16(ranges + i);
            if (!ro) return (cp + delta) & 0xffff;
            const g = dv.getUint16(ranges + i + ro + (cp - start) * 2);
            return g ? (g + delta) & 0xffff : 0;
        }
        return 0;
    },

    advance(f, gid) {
        const i = Math.min(gid, f.nMetrics - 1);
        return f.dv.getUint16(f.tables.hmtx.off + i * 4);
    },

    glyphSpan(f, gid) {
        const { dv } = f, l = f.tables.loca.off;
        return f.longLoca ? [dv.getUint32(l + gid * 4), dv.getUint32(l + gid * 4 + 4)] : [dv.getUint16(l + gid * 2) * 2, dv.getUint16(l + gid * 2 + 2) * 2];
    },

    // the font with every glyph but the used ones (and the parts they're built from) emptied;
    // glyph ids stay as they were, so the PDF can name glyphs by id
    subset(f, used) {
        const { dv } = f, glyf = f.tables.glyf.off;
        const keep = new Set([0, ...used]);
        const todo = [...keep];
        while (todo.length) {
            const [a, b] = this.glyphSpan(f, todo.pop());
            if (b - a < 10 || dv.getInt16(glyf + a) >= 0) continue;
            let p = glyf + a + 10;
            for (;;) {
                const flags = dv.getUint16(p), comp = dv.getUint16(p + 2);
                if (!keep.has(comp)) { keep.add(comp); todo.push(comp); }
                p += 4 + (flags & 1 ? 4 : 2) + (flags & 8 ? 2 : flags & 0x40 ? 4 : flags & 0x80 ? 8 : 0);
                if (!(flags & 0x20)) break;
            }
        }
        const parts = [], loca = new DataView(new ArrayBuffer((f.nGlyphs + 1) * 4));
        let at = 0;
        for (let g = 0; g < f.nGlyphs; g++) {
            loca.setUint32(g * 4, at);
            if (!keep.has(g)) continue;
            const [a, b] = this.glyphSpan(f, g);
            const len = (b - a + 3) & ~3;
            const piece = new Uint8Array(len);
            piece.set(new Uint8Array(f.buf, glyf + a, b - a));
            parts.push(piece);
            at += len;
        }
        loca.setUint32(f.nGlyphs * 4, at);
        const glyfOut = new Uint8Array(at);
        let o = 0;
        for (const p of parts) { glyfOut.set(p, o); o += p.length; }
        const headOut = new Uint8Array(f.buf.slice(f.tables.head.off, f.tables.head.off + f.tables.head.len));
        const hv = new DataView(headOut.buffer);
        hv.setUint32(8, 0);     // checkSumAdjustment: not worked out again
        hv.setInt16(50, 1);     // long offsets in loca
        const out = { head: headOut, loca: new Uint8Array(loca.buffer), glyf: glyfOut };
        for (const t of ['hhea', 'hmtx', 'maxp', 'cvt ', 'fpgm', 'prep', 'cmap', 'OS/2', 'post', 'name'])
            if (f.tables[t]) out[t] = new Uint8Array(f.buf, f.tables[t].off, f.tables[t].len);
        return this.sfnt(out);
    },

    // tables into a font file: the directory, then each table on a 4-byte boundary
    sfnt(tables) {
        const tags = Object.keys(tables).sort();
        const n = tags.length;
        let size = 12 + n * 16;
        for (const t of tags) size += (tables[t].length + 3) & ~3;
        const out = new Uint8Array(size), dv = new DataView(out.buffer);
        const pow = 2 ** Math.floor(Math.log2(n));
        dv.setUint32(0, 0x00010000); dv.setUint16(4, n); dv.setUint16(6, pow * 16); dv.setUint16(8, Math.log2(pow)); dv.setUint16(10, n * 16 - pow * 16);
        let at = 12 + n * 16;
        tags.forEach((t, i) => {
            const data = tables[t], r = 12 + i * 16;
            for (let k = 0; k < 4; k++) dv.setUint8(r + k, t.charCodeAt(k));
            out.set(data, at);
            let sum = 0;
            for (let k = 0; k < data.length; k += 4) sum = (sum + dv.getUint32(at + k)) >>> 0;
            dv.setUint32(r + 4, sum); dv.setUint32(r + 8, at); dv.setUint32(r + 12, data.length);
            at += (data.length + 3) & ~3;
        });
        return out;
    },

    // the added fonts the text needs, fetched and read; a file that can't be is left out
    async loadFonts(w) {
        const urls = new Set();
        for (const p of w.pages) for (const o of p.ops) if (typeof o !== 'string') for (const u of w.fonts.get(o.font) || []) urls.add(u);
        const files = new Map();
        await Promise.all([...urls].map(async (u) => {
            try {
                const res = await fetch(u);
                if (!res.ok) throw new Error(res.status + ' for ' + u);
                files.set(u, { font: this.ttf(await res.arrayBuffer()), used: new Map(), ref: '', name: '' });
            } catch (e) { console.warn('pdf: font not loaded,', this.msg(e)); }
        }));
        return files;
    },

    // a text op in an added font: runs of characters per file, each glyph noted as used
    runs(w, files, op) {
        const chain = (w.fonts.get(op.font) || []).map((u) => files.get(u)).filter(Boolean);
        if (!chain.length) return null;
        const runs = [];
        for (const ch of op.str) {
            const cp = ch.codePointAt(0);
            let file = chain[0], gid = 0;
            for (const c of chain) { const g = this.glyphOf(c.font, cp); if (g) { file = c; gid = g; break; } }
            // in no file: a '?', which copies as one too
            if (!gid) { file = chain[0]; gid = this.glyphOf(file.font, 63); file.used.set(gid, '?'); }
            else if (!file.used.has(gid)) file.used.set(gid, ch);
            const last = runs[runs.length - 1];
            if (last && last.file === file) last.gids.push(gid);
            else runs.push({ file, gids: [gid] });
        }
        return runs;
    },

    utf16hex(s) {
        let h = '';
        for (let i = 0; i < s.length; i++) h += s.charCodeAt(i).toString(16).padStart(4, '0');
        return h;
    },

    // one file as a Type0 font: the cut-down file, glyph widths, and the map back to text so
    // the PDF's text can be searched and copied
    async embedFont(file, n, add, ref) {
        const f = file.font, k = 1000 / f.upem, num = this.num;
        const gids = [...file.used.keys()].sort((a, b) => a - b);
        const tag = String.fromCharCode(...[...String(n).padStart(6, '0')].map((d) => 65 + +d));
        const name = '/' + tag + '+' + file.name.replace(/[^A-Za-z0-9-]/g, '');
        const bytes = this.subset(f, gids);
        const fontFile = add('<< /Length1 ' + bytes.length + ' /Filter /FlateDecode >>', await this.deflate(bytes));
        const desc = add('<< /Type /FontDescriptor /FontName ' + name + ' /Flags 4 /FontBBox [' + f.bbox.map((v) => num(v * k)).join(' ') + ']' +
            ' /ItalicAngle ' + num(f.italic) + ' /Ascent ' + num(f.ascent * k) + ' /Descent ' + num(f.descent * k) +
            ' /CapHeight ' + num(f.capHeight * k) + ' /StemV 80 /FontFile2 ' + ref(fontFile) + ' >>');
        const widths = gids.map((g) => g + ' [' + num(this.advance(f, g) * k) + ']').join(' ');
        const cid = add('<< /Type /Font /Subtype /CIDFontType2 /BaseFont ' + name + ' /CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >>' +
            ' /FontDescriptor ' + ref(desc) + ' /W [' + widths + '] /CIDToGIDMap /Identity >>');
        let map = '/CIDInit /ProcSet findresource begin\n12 dict begin\nbegincmap\n/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n' +
            '/CMapName /Adobe-Identity-UCS def\n/CMapType 2 def\n1 begincodespacerange\n<0000> <FFFF>\nendcodespacerange\n';
        for (let i = 0; i < gids.length; i += 100) {
            const part = gids.slice(i, i + 100);
            map += part.length + ' beginbfchar\n' + part.map((g) => '<' + g.toString(16).padStart(4, '0') + '> <' + this.utf16hex(file.used.get(g)) + '>').join('\n') + '\nendbfchar\n';
        }
        map += 'endcmap\nCMapName currentdict /CMap defineresource pop\nend\nend\n';
        const toUni = add('<< /Filter /FlateDecode >>', await this.deflate(this.latin1(map)));
        return add('<< /Type /Font /Subtype /Type0 /BaseFont ' + name + ' /Encoding /Identity-H /DescendantFonts [' + ref(cid) + '] /ToUnicode ' + ref(toUni) + ' >>');
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

    // opaque as JPEG, alpha as deflated RGB + mask
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
        const objs = []; // [dict, stream or null], object number = index + 1
        const add = (dict, data) => { objs.push([dict, data || null]); return objs.length; };
        const ref = (n) => n + ' 0 R';
        add('<< /Type /Catalog /Pages 2 0 R >>');
        add(null); // pages, filled in below

        // text in added fonts: glyph runs per file; a font that didn't load falls back to Helvetica
        const files = await this.loadFonts(w);
        [...files.values()].forEach((f, i) => { f.key = 'F' + i; });
        for (const p of w.pages) {
            p.ops = p.ops.map((o) => {
                if (typeof o === 'string') return o;
                const runs = this.runs(w, files, o);
                if (!runs) return this.helv(w, o);
                const n = this.num;
                let s = 'BT 1 0 0 -1 ' + n(o.x) + ' ' + n(o.y) + ' Tm';
                for (const r of runs) s += ' /' + r.file.key + ' ' + n(o.size) + ' Tf <' + r.gids.map((g) => g.toString(16).padStart(4, '0')).join('') + '> Tj';
                return s + ' ET';
            });
        }
        const fontRes = [];
        let fn = 0;
        for (const [url, file] of files) {
            if (!file.used.size) continue;
            file.name = url.split('/').pop().replace(/\.[a-z0-9]+$/i, '');
            fontRes.push('/' + file.key + ' ' + ref(await this.embedFont(file, fn++, add, ref)));
        }

        const res = [];
        if (w.font) fontRes.push('/Helv ' + ref(add('<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>')));
        if (fontRes.length) res.push('/Font << ' + fontRes.join(' ') + ' >>');
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
