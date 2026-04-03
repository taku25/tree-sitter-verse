# tree-sitter-verse

[English](#english) | [日本語](README_ja.md)

A [Tree-sitter](https://tree-sitter.github.io/tree-sitter/) grammar for the
**Verse** programming language — Epic Games' functional-logic scripting language
for Unreal Editor for Fortnite (UEFN).

This grammar provides syntax highlighting, indentation, and symbol navigation for
`.verse` files inside Neovim (and any editor with Tree-sitter support).

---

## What is Verse?

Verse is a new, statically-typed, functional-logic programming language designed
by Epic Games for scripting gameplay in UEFN (Unreal Editor for Fortnite).
Key characteristics:

- **Functional-logic**: expressions can *fail*, allowing clean conditional logic
  without exceptions
- **Concurrency**: built-in `sync`, `race`, `branch`, `spawn` constructs
- **Effect system**: functions annotate their side-effects with specifiers such
  as `<reads>`, `<writes>`, `<suspends>`, `<transacts>`
- **Indentation-sensitive**: blocks can be written with `:` + indented lines,
  `{ braces }`, or `. dot` (single expression)

Language spec: <https://github.com/verselang/book>

---

## Supported Syntax

| Feature | Examples |
|---|---|
| Declarations | `X := 42`, `var Score:int = 0` |
| Functions | `Foo(A:int):string = ...` |
| Classes / Structs | `MyClass := class: ...` |
| Enums | `Rarity := enum{ common rare }` |
| Interfaces / Modules | `IFoo := interface: ...` |
| Specifiers | `<public>`, `<suspends>`, `<decides>` |
| Control flow | `if`, `for`, `loop`, `block`, `case` |
| Concurrency | `sync`, `race`, `branch`, `spawn`, `when` |
| Operators | `=` (eq), `<>` (neq), `and`/`or`/`not`, `..` (range) |
| Failable calls | `Map[Key]`, `Arr[0]` |
| String interpolation | `"Hello {Name}!"` |
| Block comments | `<# nested <# comments #> #>` |
| `using` declarations | `using { /Verse.org/Verse }` |

---

## Installation

### With nvim-treesitter (manual parser registration)

Until this grammar is merged into `nvim-treesitter`'s official parser list, add
the following to your Neovim config:

```lua
local parser_config = require("nvim-treesitter.parsers").get_parser_configs()
parser_config.verse = {
  install_info = {
    url   = "https://github.com/taku25/tree-sitter-verse",
    files = { "src/parser.c", "src/scanner.c" },
    generate_requires_npm = false,
    requires_generate_from_grammar = false,
  },
  filetype = "verse",
}
vim.filetype.add({ extension = { verse = "verse" } })
```

Then install the parser:

```vim
:TSInstall verse
```

---

## Development

### Prerequisites

- [Node.js](https://nodejs.org/) ≥ 18
- [tree-sitter CLI](https://tree-sitter.github.io/tree-sitter/creating-parsers)

```sh
npm install          # install devDependencies (tree-sitter-cli)
```

### Generate the parser

```sh
tree-sitter generate
```

### Run tests

```sh
tree-sitter test
```

### Parse a file

```sh
tree-sitter parse path/to/file.verse
```

---

## Project Structure

```
tree-sitter-verse/
├── grammar.js          # Grammar definition (source of truth)
├── src/
│   ├── parser.c        # Generated parser (do not edit)
│   └── scanner.c       # External scanner (INDENT/DEDENT/block comments)
├── queries/
│   ├── highlights.scm  # Syntax highlighting
│   ├── locals.scm      # Scope / binding information
│   └── tags.scm        # Symbol navigation (ctags-style)
└── test/
    └── corpus/         # Tree-sitter test corpus
```

---

## License

[MIT](LICENSE) © 2025 taku25
