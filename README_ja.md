# tree-sitter-verse

[English](#english) | [日本語](#japanese)

---

<a id="english"></a>

## tree-sitter-verse

A [Tree-sitter](https://tree-sitter.github.io/tree-sitter/) grammar for the
**Verse** programming language — Epic Games' functional-logic scripting language
for Unreal Editor for Fortnite (UEFN).

This grammar is part of the [UEP.nvim](https://github.com/taku25/UEP.nvim)
suite and provides syntax highlighting, indentation, and symbol navigation for
`.verse` files inside Neovim.

---

### What is Verse?

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

### Supported Syntax

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

### Installation

#### With nvim-treesitter (manual parser registration)

Until this grammar is merged into `nvim-treesitter`'s official parser list, add
the following to your Neovim config:

```lua
local parser_config = require("nvim-treesitter.parsers").get_parser_configs()
parser_config.verse = {
  install_info = {
    url   = "https://github.com/taku25/UEP.nvim",
    files = { "tree-sitter-verse/src/parser.c", "tree-sitter-verse/src/scanner.c" },
    location             = "tree-sitter-verse",
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

### Development

#### Prerequisites

- [Node.js](https://nodejs.org/) ≥ 18
- [tree-sitter CLI](https://tree-sitter.github.io/tree-sitter/creating-parsers)

```sh
npm install          # install devDependencies (tree-sitter-cli)
```

#### Generate the parser

```sh
tree-sitter generate
```

#### Run tests

```sh
tree-sitter test
```

#### Parse a file

```sh
tree-sitter parse path/to/file.verse
```

---

### Project Structure

```
tree-sitter-verse/
├── grammar.js          # Grammar definition (source of truth)
├── src/
│   ├── parser.c        # Generated parser (do not edit)
│   └── scanner.c       # External scanner (INDENT/DEDENT/block comments)
├── queries/
│   ├── highlights.scm  # Neovim syntax highlighting
│   ├── locals.scm      # Scope / binding information
│   └── tags.scm        # Symbol navigation (ctags-style)
└── test/
    └── corpus/         # Tree-sitter test corpus
```

---

### License

MIT

---

---

<a id="japanese"></a>

## tree-sitter-verse（日本語）

**Verse** プログラミング言語向けの [Tree-sitter](https://tree-sitter.github.io/tree-sitter/) 文法です。
Verse は Epic Games が UEFN（Unreal Editor for Fortnite）向けに開発した
関数型・論理型のスクリプト言語です。

この文法は [UEP.nvim](https://github.com/taku25/UEP.nvim) の一部として提供されており、
Neovim 上で `.verse` ファイルのシンタックスハイライト・インデント・シンボルナビゲーションを実現します。

---

### Verse とは？

Verse は Epic Games が設計した新しい静的型付き言語で、以下の特徴を持ちます。

- **関数型・論理型**: 式が「失敗（fail）」できるため、例外を使わずにクリーンな条件ロジックが書ける
- **並行処理**: `sync`・`race`・`branch`・`spawn` が言語組み込みで提供される
- **エフェクトシステム**: `<reads>`・`<writes>`・`<suspends>`・`<transacts>` などのスペシファイアで副作用を宣言する
- **インデント感応**: ブロックは `:` + インデント、`{ ブレース }`、`. ドット`（単一式）の 3 形式で記述できる

言語仕様: <https://github.com/verselang/book>

---

### 対応構文

| 機能 | 例 |
|---|---|
| 宣言 | `X := 42`、`var Score:int = 0` |
| 関数 | `Foo(A:int):string = ...` |
| クラス・構造体 | `MyClass := class: ...` |
| 列挙型 | `Rarity := enum{ common rare }` |
| インターフェース・モジュール | `IFoo := interface: ...` |
| スペシファイア | `<public>`、`<suspends>`、`<decides>` |
| 制御フロー | `if`、`for`、`loop`、`block`、`case` |
| 並行処理 | `sync`、`race`、`branch`、`spawn`、`when` |
| 演算子 | `=`（等値）、`<>`（不等値）、`and`/`or`/`not`、`..`（範囲） |
| 失敗可能呼び出し | `Map[Key]`、`Arr[0]` |
| 文字列補間 | `"Hello {Name}!"` |
| ブロックコメント | `<# ネスト可能 <# コメント #> #>` |
| `using` 宣言 | `using { /Verse.org/Verse }` |

---

### インストール

#### nvim-treesitter を使った手動パーサー登録

公式パーサーリストへのマージ前は、Neovim の設定に以下を追加してください。

```lua
local parser_config = require("nvim-treesitter.parsers").get_parser_configs()
parser_config.verse = {
  install_info = {
    url   = "https://github.com/taku25/UEP.nvim",
    files = { "tree-sitter-verse/src/parser.c", "tree-sitter-verse/src/scanner.c" },
    location             = "tree-sitter-verse",
    generate_requires_npm = false,
    requires_generate_from_grammar = false,
  },
  filetype = "verse",
}
vim.filetype.add({ extension = { verse = "verse" } })
```

その後、パーサーをインストールします。

```vim
:TSInstall verse
```

---

### 開発

#### 前提条件

- [Node.js](https://nodejs.org/) バージョン 18 以上
- [tree-sitter CLI](https://tree-sitter.github.io/tree-sitter/creating-parsers)

```sh
npm install   # devDependencies（tree-sitter-cli）のインストール
```

#### パーサーの生成

```sh
tree-sitter generate
```

#### テストの実行

```sh
tree-sitter test
```

#### ファイルのパース

```sh
tree-sitter parse path/to/file.verse
```

---

### ディレクトリ構成

```
tree-sitter-verse/
├── grammar.js          # 文法定義（唯一の編集対象）
├── src/
│   ├── parser.c        # 生成されたパーサー（直接編集不可）
│   └── scanner.c       # 外部スキャナー（INDENT/DEDENT/ブロックコメント）
├── queries/
│   ├── highlights.scm  # Neovim シンタックスハイライト
│   ├── locals.scm      # スコープ・バインディング情報
│   └── tags.scm        # シンボルナビゲーション（ctagsスタイル）
└── test/
    └── corpus/         # tree-sitter テストコーパス
```

---

### ライセンス

MIT
