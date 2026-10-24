// The PDF writer's TrueType fonts: a chain of files under one name, cut down to the glyphs used
// (and the parts a composite glyph is built from), the text kept for copying, and Helvetica
// when a file can't be fetched. fixtures/tiny.ttf: .notdef, A, B, ?, dieresis, and Ä made of
// A + dieresis (glyph ids 0-5), each glyph a box.
//
// Usage: node tests/js/pdf_fonts.mjs <repo-root>
import { readFileSync } from "node:fs";
import { join } from "node:path";
import { inflateSync } from "node:zlib";

const root = process.argv[2];
const src = readFileSync(join(root, "js/pdf.js"), "utf8");
const tiny = readFileSync(join(root, "tests/js/fixtures/tiny.ttf"));

let failed = 0;
const check = (ok, what) => { console.log((ok ? "  PASS  " : "  FAIL  ") + what); if (!ok) failed++; };

globalThis.window = globalThis;
globalThis.blobs = {};
let written = null;
globalThis.push_event_pdf_WRITTEN = (h, b, err) => { written = { b, err }; };
globalThis.fetch = async (u) => u.endsWith("tiny.ttf")
  ? { ok: true, arrayBuffer: async () => tiny.buffer.slice(tiny.byteOffset, tiny.byteOffset + tiny.length) }
  : { ok: false, status: 404 };
const quiet = console.warn;
console.warn = () => {};
const pdf = new Function(src.replace(/^const __wcc_pdf =/m, "return"))();

pdf.create(1);
pdf.addFont(1, "tiny", "/f/tiny.ttf");
pdf.addFont(1, "lost", "/f/missing.ttf");
pdf.page(1, 200, 200);
pdf.setFont(1, "tiny");
pdf.text(1, "AÄz", 10, 20, 12);     // z: in no file, drawn as ?
pdf.setFont(1, "lost");
pdf.text(1, "gone", 10, 40, 12);
pdf.finish(1);
for (let i = 0; i < 100 && !written; i++) await new Promise((r) => setTimeout(r, 10));
console.warn = quiet;

check(written && !written.err, "the writer finishes");
const bytes = written ? globalThis.blobs[written.b] : new Uint8Array();
const text = Buffer.from(bytes).toString("latin1");

// every deflated stream, inflated
const streams = [];
for (let at = text.indexOf("stream\n"); at >= 0; at = text.indexOf("stream\n", at + 1)) {
  if (text.slice(at - 3, at) === "end") continue;
  const end = text.indexOf("\nendstream", at);
  try { streams.push(inflateSync(bytes.subarray(at + 7, end))); } catch (e) {}
}
const content = streams.map((s) => s.toString("latin1")).find((s) => s.includes(" Tj"));
check(content && content.includes("<000100050003> Tj"), "A, Ä and the ? for z as glyph ids 1, 5, 3");
check(content && content.includes("/Helv 12 Tf") && content.includes("(gone) Tj"), "a font that can't be fetched falls back to Helvetica");
check(text.includes("/Subtype /Type0") && text.includes("/Encoding /Identity-H") && text.includes("/CIDToGIDMap /Identity"), "a Type0 font with Identity-H");
check(/\/W \[1 \[600\] 3 \[600\] 5 \[600\]\]/.test(text), "widths for the glyphs used, in thousandths of the em");

const cmap = streams.map((s) => s.toString("latin1")).find((s) => s.includes("beginbfchar"));
check(cmap && cmap.includes("<0001> <0041>") && cmap.includes("<0005> <00c4>"), "ToUnicode maps the glyphs back to A and Ä");
check(cmap && cmap.includes("<0003> <003f>"), "the ? standing in for z copies as ?, not z");

// the embedded file: glyf keeps A (1), ? (3), Ä (5) and the dieresis it is built from (4); B (2) is empty
const font = streams.find((s) => s.length > 12 && s.readUInt32BE(0) === 0x00010000);
check(!!font, "the font file is embedded");
if (font) {
  const n = font.readUInt16BE(4), tables = {};
  for (let i = 0; i < n; i++) tables[font.toString("latin1", 12 + i * 16, 16 + i * 16)] = font.readUInt32BE(20 + i * 16);
  const loca = (g) => font.readUInt32BE(tables.loca + g * 4);
  const len = (g) => loca(g + 1) - loca(g);
  check(len(1) > 0 && len(3) > 0 && len(4) > 0 && len(5) > 0, "used glyphs and a composite's parts are kept");
  check(len(2) === 0, "an unused glyph is emptied");
}

if (failed) { console.log(`\n${failed} failed.`); process.exit(1); }
console.log("\nThe PDF writer's fonts work.");
