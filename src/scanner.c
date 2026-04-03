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
      /* Peek ahead: we need the next char to be '#'. Use mark_end
         trick: advance '<', check '#', if not restore. We cannot
         restore in tree-sitter scanner, so we only commit when sure. */
      /* Actually: tree-sitter will call us only when BLOCK_COMMENT
         is valid AND we're at a position where '<' appears. Advance. */
      lexer->mark_end(lexer);
      advance(lexer);
      if (lexer->lookahead == '#') {
        /* Confirmed: this is a block comment. Back up to start. */
        /* We already advanced past '<' — use a local buffer trick.
           Actually tree-sitter scan cannot back up. We have to do
           the full scan from the '<'. Reset: we consumed '<', now
           continue with the '#' logic inline. */
        advance(lexer); /* '#' */
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

    uint32_t prev = current_indent(s);

    if (col > prev && valid_symbols[INDENT]) {
      /* Increase in indentation. */
      if (s->stack_size < MAX_INDENT) {
        s->indent_stack[s->stack_size++] = col;
      }
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
