; Verse tree-sitter highlights for Neovim/nvim-treesitter

; ─────────────────────────────────────────────
; Comments
; ─────────────────────────────────────────────
(line_comment) @comment
(block_comment) @comment

; ─────────────────────────────────────────────
; Keywords
; ─────────────────────────────────────────────

; Control flow
[
  "if"
  "then"
  "else"
  "for"
  "loop"
  "block"
  "case"
  "break"
  "return"
  "yield"
] @keyword

; Declaration keywords
[
  "var"
  "set"
  "using"
] @keyword

; Type definition keywords
[
  "class"
  "struct"
  "enum"
  "interface"
  "module"
] @keyword.type

; Concurrency
[
  "sync"
  "race"
  "branch"
  "spawn"
  "when"
] @keyword.coroutine

; Built-in type keywords (used as type annotations)
(builtin_type) @type.builtin

; ─────────────────────────────────────────────
; Literals
; ─────────────────────────────────────────────
(integer_literal) @number
(float_literal) @number.float
(boolean_literal) @boolean

(char_literal) @character

(string_literal) @string
(string_content) @string
(escape_sequence) @string.escape
(string_interpolation) @string.special

(path_literal) @string.special

; ─────────────────────────────────────────────
; Identifiers
; ─────────────────────────────────────────────

; Type definitions (classes, structs, etc.)
(type_definition
  name: (identifier) @type.definition)

(class_definition) @type
(struct_definition) @type
(enum_definition) @type
(interface_definition) @type

; Enum variant names (variants appear as name: children of enum_definition)
(enum_definition
  name: (identifier) @constant)

; Function definitions
(function_definition
  name: (identifier) @function)

; Field names in declarations
(field_declaration
  name: (identifier) @variable.member)

; Field initializers in object construction
(field_initializer
  name: (identifier) @variable.member)

; Variable declarations
(var_declaration
  name: (identifier) @variable)

; Parameters
(positional_parameter
  name: (identifier) @variable.parameter)

(named_parameter
  name: (identifier) @variable.parameter)

; Function calls
(call_expression
  function: (identifier) @function.call)

(index_expression
  object: (identifier) @function.call)

(call_expression
  function: (member_access
    member: (identifier) @function.method.call))

(index_expression
  object: (member_access
    member: (identifier) @function.method.call))

; Member access
(member_access
  member: (identifier) @variable.member)

; Named arguments in calls
(named_argument
  name: (identifier) @variable.parameter)

; General identifier fallback
(identifier) @variable

; Self
(self_expression) @variable.builtin

; Wildcard
(wildcard) @variable.builtin

; ─────────────────────────────────────────────
; Types
; ─────────────────────────────────────────────
(function_definition
  return_type: (_) @type)

(field_declaration
  type: (_) @type)

(var_declaration
  type: (_) @type)

(positional_parameter
  type: (_) @type)

(named_parameter
  type: (_) @type)

(optional_type) @type
(array_type) @type
(map_type) @type
(function_type) @type
(qualified_type) @type

; ─────────────────────────────────────────────
; Operators
; ─────────────────────────────────────────────
[
  ":="
  "="
  "+="
  "-="
  "*="
  "/="
] @operator

[
  "+"
  "-"
  "*"
  "/"
  "="
  "<>"
  "<"
  "<="
  ">"
  ">="
  "and"
  "or"
  "not"
  ".."
  "?"
  "=>"
  "->"
] @operator

; ─────────────────────────────────────────────
; Punctuation
; ─────────────────────────────────────────────
[
  "("
  ")"
  "["
  "]"
  "{"
  "}"
] @punctuation.bracket

[
  ","
  "."
  ":"
  ";"
] @punctuation.delimiter

; ─────────────────────────────────────────────
; Specifiers (angle-bracket annotations)
; ─────────────────────────────────────────────
(specifier) @attribute

; Access modifiers specifically
(specifier
  name: (_) @keyword.modifier
  (#any-of? @keyword.modifier "public" "private" "protected" "internal"))

; Effect specifiers
(specifier
  name: (_) @keyword.modifier
  (#any-of? @keyword.modifier "computes" "reads" "writes" "transacts"
            "decides" "suspends" "allocates" "converges"))

; Using module path
(module_path) @module

(using_declaration
  (module_path) @module)
