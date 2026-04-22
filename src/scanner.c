#include "tree_sitter/parser.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────────
 * Token types – must match the order of externals in grammar.js.
 * ───────────────────────────────────────────────────────────────── */
typedef enum {
  INDENT,        // 0
  DEDENT,        // 1
  BLOCK_COMMENT, // 2
} TokenType;

/* ─────────────────────────────────────────────────────────────────
 * Scanner state
 * ───────────────────────────────────────────────────────────────── */
#define MAX_INDENT 256

typedef struct {
  uint32_t indent_stack[MAX_INDENT];
  uint32_t stack_size;
  /* pending DEDENT count: emit one DEDENT per call until 0 */
  uint32_t pending_dedents;
  /* bracket depth: () [] {}; INDENT/DEDENT suppressed when > 0 */
  uint32_t bracket_depth;
} Scanner;

/* ─────────────────────────────────────────────────────────────────
 * Helpers
 * ───────────────────────────────────────────────────────────────── */

static inline uint32_t current_indent(const Scanner *s) {
  return s->stack_size > 0 ? s->indent_stack[s->stack_size - 1] : 0;
}

/* Advance past one character (consuming it). */
static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

/* Skip a character as whitespace (not part of any token). */
static inline void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

/* ─────────────────────────────────────────────────────────────────
 * Block comment scanner: <# ... #> with arbitrary nesting.
 * ───────────────────────────────────────────────────────────────── */
static bool scan_block_comment(TSLexer *lexer) {
  /* Caller has confirmed we are at '<#'. */
  advance(lexer); /* '<' */
  advance(lexer); /* '#' */

  int depth = 1;
  while (lexer->lookahead != 0) {
    if (lexer->lookahead == '<') {
      advance(lexer);
      if (lexer->lookahead == '#') {
        advance(lexer);
        depth++;
      }
    } else if (lexer->lookahead == '#') {
      advance(lexer);
      if (lexer->lookahead == '>') {
        advance(lexer);
        depth--;
        if (depth == 0) {
          lexer->result_symbol = BLOCK_COMMENT;
          return true;
        }
      }
    } else {
      advance(lexer);
    }
  }
  /* Unterminated — still return true so the lexer can recover. */
  lexer->result_symbol = BLOCK_COMMENT;
  return true;
}

/* ─────────────────────────────────────────────────────────────────
 * Main scan function
 * ───────────────────────────────────────────────────────────────── */
static bool scan(Scanner *s, TSLexer *lexer, const bool *valid_symbols) {
  /* ── Emit pending DEDENTs first ──────────────────────────────── */
  if (s->pending_dedents > 0 && valid_symbols[DEDENT]) {
    s->pending_dedents--;
    if (s->stack_size > 0) s->stack_size--;
    lexer->result_symbol = DEDENT;
    return true;
  }

  /* ── BLOCK_COMMENT: try '<#' at any point ─────────────────────  */
  /* (tree-sitter extras mechanism will skip whitespace before here,
     but we need to detect '<#' before '<' is consumed by grammar.)  */
  if (valid_symbols[BLOCK_COMMENT]) {
    if (lexer->lookahead == '<') {
      lexer->mark_end(lexer);
      advance(lexer);
      if (lexer->lookahead == '#') {
        advance(lexer); /* '#' */
        /* Special case: <#> (immediately followed by '>') is the UEFN
           single-line comment form. Scan to EOL only, not to '#>'. */
        if (lexer->lookahead == '>') {
          advance(lexer); /* '>' */
          while (lexer->lookahead != '\n' && lexer->lookahead != '\r' &&
                 lexer->lookahead != 0) {
            advance(lexer);
          }
          lexer->mark_end(lexer);
          lexer->result_symbol = BLOCK_COMMENT;
          return true;
        }
        /* Normal nested block comment: scan until matching '#>' or EOF. */
        int depth = 1;
        while (lexer->lookahead != 0) {
          if (lexer->lookahead == '<') {
            advance(lexer);
            if (lexer->lookahead == '#') { advance(lexer); depth++; }
          } else if (lexer->lookahead == '#') {
            advance(lexer);
            if (lexer->lookahead == '>') { advance(lexer); depth--; if (depth == 0) break; }
          } else {
            advance(lexer);
          }
        }
        lexer->mark_end(lexer); /* token ends at closing #> or EOF (overrides initial mark) */
        lexer->result_symbol = BLOCK_COMMENT;
        return true;
      }
      /* Not a block comment — don't consume. But we already advanced
         past '<'. Return false so the grammar can try other rules. */
      return false;
    }
  }

  /* ── Skip horizontal whitespace (not newlines). ──────────────── */
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' ||
         lexer->lookahead == '\r') {
    skip(lexer);
  }

  /* ── Track bracket depth so we suppress INDENT/DEDENT inside
        (), [], {} ─────────────────────────────────────────────── */
  /* We don't intercept bracket chars here; rely on the fact that
     INDENT/DEDENT are only requested when the grammar is at a
     colon-block or indented-block position, which can't be inside
     an expression's brackets (the grammar rules ensure that).
     Therefore, bracket_depth tracking is unnecessary in this
     approach — we just rely on the grammar constraints. */

  /* ── EOF: emit phantom INDENT (for empty bodies) or DEDENT ──── */
  if (lexer->lookahead == 0) {
    if (valid_symbols[INDENT] && !valid_symbols[DEDENT]) {
      /* Empty body at end-of-file: `foo := interface:` with nothing after.
         Push a phantom indent level so the following DEDENT call can close it. */
      uint32_t prev = current_indent(s);
      if (s->stack_size < MAX_INDENT) {
        s->indent_stack[s->stack_size++] = prev + 1;
      }
      lexer->result_symbol = INDENT;
      return true;
    }
    if (valid_symbols[DEDENT] && s->stack_size > 1) {
      /* Close any remaining open indentation levels at EOF. */
      s->stack_size--;
      lexer->result_symbol = DEDENT;
      return true;
    }
    return false;
  }

  /* ── INDENT / DEDENT: only after a newline ────────────────────  */
  if (lexer->lookahead != '\n' && lexer->lookahead != '\r') {
    /* Not at a newline — only INDENT/DEDENT are requested here,
       and neither is possible. */
    return false;
  }

  if (!valid_symbols[INDENT] && !valid_symbols[DEDENT]) {
    return false;
  }

  /* Consume the newline(s). */
  while (lexer->lookahead == '\n' || lexer->lookahead == '\r') {
    skip(lexer);
  }

  /* Count the column of the next non-whitespace, non-comment line. */
  while (true) {
    /* Count leading spaces on this line. */
    uint32_t col = 0;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      if (lexer->lookahead == '\t') col = (col + 8) & ~7u; /* tab = 8 */
      else col++;
      skip(lexer);
    }

    /* Skip blank lines and comment-only lines. */
    if (lexer->lookahead == '\n' || lexer->lookahead == '\r') {
      while (lexer->lookahead == '\n' || lexer->lookahead == '\r') skip(lexer);
      continue;
    }
    if (lexer->lookahead == '#') {
      /* skip to end of line */
      while (lexer->lookahead != '\n' && lexer->lookahead != '\r' &&
             lexer->lookahead != 0) {
        skip(lexer);
      }
      while (lexer->lookahead == '\n' || lexer->lookahead == '\r') skip(lexer);
      continue;
    }

    /* EOF reached after consuming newlines/blanks/comments. */
    if (lexer->lookahead == 0) {
      uint32_t prev = current_indent(s);
      /* Prefer closing open bodies at EOF over opening new phantom bodies.
         Only emit phantom INDENT if DEDENT is not also valid (i.e., we're
         strictly waiting for an INDENT to start an empty body). */
      if (valid_symbols[INDENT] && !valid_symbols[DEDENT]) {
        if (s->stack_size < MAX_INDENT) {
          s->indent_stack[s->stack_size++] = prev + 1;
        }
        lexer->result_symbol = INDENT;
        return true;
      }
      if (valid_symbols[DEDENT] && s->stack_size > 1) {
        s->stack_size--;
        lexer->result_symbol = DEDENT;
        return true;
      }
      return false;
    }

    uint32_t prev = current_indent(s);

    if (col > prev && valid_symbols[INDENT]) {
      /* Increase in indentation. */
      if (s->stack_size < MAX_INDENT) {
        s->indent_stack[s->stack_size++] = col;
      }
      lexer->result_symbol = INDENT;
      return true;
    }

    /* Empty body: grammar expects INDENT but next non-blank content is at
       the same indentation level (or lower). This happens with abstract/native
       classes that have no members (e.g. `abstract_class<native>:` immediately
       followed by a sibling declaration at the same level).
       Emit a phantom INDENT by pushing (prev+1) and schedule an immediate
       DEDENT via pending_dedents=1. This produces an empty block and prevents
       the next sibling from being erroneously nested inside this empty body.
       After INDENT is consumed, the scanner is called again at the same lexer
       position (still pointing at the sibling's first character, not a newline).
       Without pending_dedents, the `lookahead != '\n'` guard would return false
       and DEDENT would never fire, causing infinite nesting. */
    if (col <= prev && valid_symbols[INDENT]) {
      if (s->stack_size < MAX_INDENT) {
        s->indent_stack[s->stack_size++] = prev + 1;
      }
      s->pending_dedents = 1;
      lexer->result_symbol = INDENT;
      return true;
    }

    if (col < prev) {
      /* Decrease in indentation: may need multiple DEDENTs. */
      uint32_t dedents = 0;
      while (s->stack_size > 0 && s->indent_stack[s->stack_size - 1] > col) {
        s->stack_size--;
        dedents++;
      }
      if (dedents > 0 && valid_symbols[DEDENT]) {
        /* Emit first DEDENT now; stash remaining. */
        s->pending_dedents = dedents - 1;
        /* stack_size already decremented above for all levels;
           restore and let pending_dedents decrement one by one. */
        s->stack_size += dedents; /* undo: will be re-decremented */
        s->stack_size--;          /* first DEDENT */
        lexer->result_symbol = DEDENT;
        return true;
      }
    }

    return false;
  }
}

/* ─────────────────────────────────────────────────────────────────
 * Tree-sitter interface
 * ───────────────────────────────────────────────────────────────── */

void *tree_sitter_verse_external_scanner_create(void) {
  Scanner *s = calloc(1, sizeof(Scanner));
  s->indent_stack[0] = 0;
  s->stack_size = 1;
  return s;
}

void tree_sitter_verse_external_scanner_destroy(void *payload) {
  free(payload);
}

bool tree_sitter_verse_external_scanner_scan(
    void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Scanner *s = (Scanner *)payload;
  return scan(s, lexer, valid_symbols);
}

unsigned tree_sitter_verse_external_scanner_serialize(void *payload,
                                                       char *buffer) {
  Scanner *s = (Scanner *)payload;
  unsigned n = 0;
  buffer[n++] = (char)s->pending_dedents;
  memcpy(buffer + n, &s->stack_size, sizeof(uint32_t));
  n += sizeof(uint32_t);
  unsigned copy = s->stack_size < MAX_INDENT ? s->stack_size : MAX_INDENT;
  memcpy(buffer + n, s->indent_stack, copy * sizeof(uint32_t));
  n += copy * sizeof(uint32_t);
  return n;
}

void tree_sitter_verse_external_scanner_deserialize(void *payload,
                                                     const char *buffer,
                                                     unsigned length) {
  Scanner *s = (Scanner *)payload;
  if (length == 0) return;
  unsigned n = 0;
  s->pending_dedents = (uint8_t)buffer[n++];
  if (n + sizeof(uint32_t) > length) return;
  memcpy(&s->stack_size, buffer + n, sizeof(uint32_t));
  n += sizeof(uint32_t);
  unsigned copy = s->stack_size < MAX_INDENT ? s->stack_size : MAX_INDENT;
  if (n + copy * sizeof(uint32_t) > length) return;
  memcpy(s->indent_stack, buffer + n, copy * sizeof(uint32_t));
}
