; Verse tags.scm — symbol navigation tags for nvim-treesitter

; ─────────────────────────────────────────────
; Type definitions
; ─────────────────────────────────────────────

; Class definition
(type_definition
  name: (identifier) @name
  value: (class_definition)) @definition.class

; Struct definition
(type_definition
  name: (identifier) @name
  value: (struct_definition)) @definition.struct

; Enum definition
(type_definition
  name: (identifier) @name
  value: (enum_definition)) @definition.enum

; Interface definition
(type_definition
  name: (identifier) @name
  value: (interface_definition)) @definition.interface

; Module definition
(type_definition
  name: (identifier) @name
  value: (module_definition)) @definition.module

; ─────────────────────────────────────────────
; Function definitions
; ─────────────────────────────────────────────

(function_definition
  name: (_) @name) @definition.function

; ─────────────────────────────────────────────
; Variable/constant definitions
; ─────────────────────────────────────────────

(var_declaration
  name: (identifier) @name) @definition.var

(type_definition
  name: (identifier) @name) @definition.constant

; ─────────────────────────────────────────────
; Enum variants (useful as tags too)
; ─────────────────────────────────────────────

(enum_variant
  name: (identifier) @name) @definition.constant
