#pragma once
#include "EpdFont.h"
#include <cassert>

/// A glyph resolved from a font family's primary or fallback chain.
/// `glyph` and `data` are always from the same EpdFontData (source-aligned).
/// Never call getGlyph() + getData() separately — use resolveGlyph() instead.
struct ResolvedGlyph {
    const EpdGlyph*    glyph;  ///< May be nullptr if primary lacks REPLACEMENT_GLYPH (bare font).
                               ///< CALLERS MUST null-check before dereferencing.
    const EpdFontData* data;   ///< Always the EpdFontData that owns `glyph`.
                               ///< Non-null even when glyph is nullptr (points to primary data).
                               ///< Use data metrics (advanceY, ascender) as safe fallback when glyph is null.
};

class EpdFontFamily {
 public:
  // Bitmask of text style flags carried per-word through layout and serialized in page cache.
  // Bits 0-1 select the font variant (BOLD/ITALIC); bits 2-5 are decoration/positioning overlays
  // applied at render time without changing the underlying font. getFont() ignores all bits
  // above bit 1 so decorations compose freely with bold/italic (e.g. BOLD | UNDERLINE | SUP).
  enum Style : uint8_t {
    REGULAR = 0,
    BOLD = 1,
    ITALIC = 2,
    BOLD_ITALIC = 3,
    UNDERLINE = 4,      // drawn as a line below baseline by TextBlock::render()
    STRIKETHROUGH = 8,  // drawn as a line through midline by TextBlock::render()
    SUP = 16,           // superscript: glyph scaled 50%, raised ~40% of ascender
    SUB = 32,           // subscript: glyph scaled 50%, lowered ~25% of ascender
  };

  explicit EpdFontFamily(const EpdFont* regular, const EpdFont* bold = nullptr, const EpdFont* italic = nullptr,
                         const EpdFont* boldItalic = nullptr)
      : regular(regular), bold(bold), italic(italic), boldItalic(boldItalic) {}
  ~EpdFontFamily() = default;
  void getTextDimensions(const char* string, int* w, int* h, Style style = REGULAR) const;
  const EpdFontData* getData(Style style = REGULAR) const;
  const EpdGlyph* getGlyph(uint32_t cp, Style style = REGULAR) const;
  int8_t getKerning(uint32_t leftCp, uint32_t rightCp, Style style = REGULAR) const;
  uint32_t applyLigatures(uint32_t cp, const char*& text, Style style = REGULAR) const;

  // True iff any participating variant has CJK UI fallback enabled. We check every
  // non-null variant (not just `regular`) so an asymmetric configuration — e.g. flag
  // set on `bold` but not `regular` — still triggers the family-level metric inflation
  // in GfxRenderer. Per-glyph dispatch in EpdFont::getGlyph consults its own flag, so
  // the metric path here must err on the side of "any variant might synthesize a CJK
  // glyph, so reserve the 20px box". src/main.cpp sets the flag uniformly across all
  // UI variants today; this guard catches a future-variant drift.
  bool hasCjkUiFallback() const {
    if (regular && regular->cjkUiFallbackEnabled()) return true;
    if (bold && bold->cjkUiFallbackEnabled()) return true;
    if (italic && italic->cjkUiFallbackEnabled()) return true;
    if (boldItalic && boldItalic->cjkUiFallbackEnabled()) return true;
    return false;
  }

  /// Register a fallback EpdFont consulted when the primary misses a codepoint.
  /// Replaces any previously registered fallback. Pass nullptr to clear.
  ///
  /// PRECONDITION: `f` (and the EpdFontFamily's primary fonts) MUST use static
  /// glyph storage — `f->data->glyphMissHandler == nullptr`. The resolver's
  /// pointer-identity miss detection (in `resolveGlyph`) calls
  /// `getGlyph(REPLACEMENT_GLYPH)` before `getGlyph(cp)`; with ring-buffer-backed
  /// SD card fonts, the second call invalidates the first pointer and the miss
  /// detection silently corrupts. The assert below structurally enforces this
  /// constraint instead of leaving it to documentation.
  void setFallback(const EpdFont* f) {
    if (f) {
      assert(f->data->glyphMissHandler == nullptr &&
             "setFallback: ring-buffer-backed fonts (glyphMissHandler != nullptr) "
             "are unsafe with the resolver's pointer-identity miss detection. "
             "If you need SD-backed fallback, implement Option A (lookupGlyph) first.");
    }
    fallback_ = f;
  }

  /// Resolve a codepoint to a glyph+data pair from primary or fallback chain.
  /// The returned pair is always source-aligned: glyph and data come from the
  /// same EpdFontData. If both primary and fallback miss, returns primary's
  /// REPLACEMENT_GLYPH with primary's data.
  ResolvedGlyph resolveGlyph(uint32_t cp, Style style = REGULAR) const;

  /// Returns the maximum ascender across primary (and fallback when present).
  /// Task 6 upgrades this stub to max(primary, fallback).
  int getMaxAscender(Style style = REGULAR) const;

  /// Returns the maximum advanceY across primary (and fallback when present).
  /// Task 6 upgrades this stub to max(primary, fallback).
  int getMaxAdvanceY(Style style = REGULAR) const;

 private:
  const EpdFont* regular;
  const EpdFont* bold;
  const EpdFont* italic;
  const EpdFont* boldItalic;
  const EpdFont* fallback_ = nullptr;  ///< Optional fallback chain (e.g. CJK font)

  const EpdFont* getFont(Style style) const;
};
