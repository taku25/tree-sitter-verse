/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

/**
 * Operator precedence levels (matching the Verse language spec).
 * Higher number = higher precedence = binds tighter.
 */
const PREC = {
  ASSIGN: 1,
  RANGE: 3,
  OR: 4,
  AND: 5,
  COMPARE: 7,
  ADD: 8,
  MULT: 9,
  UNARY: 10,
  CALL: 11,
};

module.exports = grammar({
  name: "verse",

  // External scanner handles: indentation tokens + nested block comments.
  externals: $ => [
    $._indent,       // increase in indentation (after ':')
    $._dedent,       // decrease in indentation
    $.block_comment, // <# nested block comments #>
  ],

  // Newlines and horizontal whitespace are insignificant everywhere.
  // INDENT/DEDENT from the external scanner carry the indentation meaning.
  extras: $ => [
    /\s/,              // all whitespace including \n
    $.line_comment,
    $.block_comment,
  ],

  supertypes: $ => [
    $._expression,
    $._type,
    $._literal,
  ],

  word: $ => $.identifier,

  conflicts: $ => [
    // identifier '(' can start a function_definition OR a call_expression
    [$.function_definition, $._expression],
  ],

  rules: {

    // ─────────────────────────────────────────────
    // Source file
    // ─────────────────────────────────────────────
    source_file: $ => repeat($._top_level_item),

    _top_level_item: $ => choice(
      $.using_declaration,
      $.var_declaration,
      $.set_statement,
      $.function_definition,
      $.type_definition,
      $._expression,
    ),

    // ─────────────────────────────────────────────
    // Comments
    // ─────────────────────────────────────────────

    line_comment: $ => token(seq('#', /[^\n]*/)),

    // block_comment is external (supports nested <# ... #>)

    // ─────────────────────────────────────────────
    // Using declarations: using { /path/to/module }
    // ─────────────────────────────────────────────

    using_declaration: $ => seq(
      'using',
      '{',
      $.module_path,
      '}',
    ),

    module_path: $ => seq(
      '/',
      repeat1(seq(
        /[a-zA-Z@][a-zA-Z0-9.\-_]*/,
        optional('/'),
      )),
    ),

    // ─────────────────────────────────────────────
    // Variable declarations: var Name:type = value
    // ─────────────────────────────────────────────

    var_declaration: $ => seq(
      'var',
      field('name', $.identifier),
      ':',
      field('type', $._type),
      optional(seq('=', field('value', $._expression))),
    ),

    // ─────────────────────────────────────────────
    // Set statements: set target op value
    // ─────────────────────────────────────────────

    set_statement: $ => seq(
      'set',
      field('target', $._expression),
      field('operator', choice('=', '+=', '-=', '*=', '/=')),
      field('value', $._expression),
    ),

    // ─────────────────────────────────────────────
    // Function definitions
    //   Name<specs>?(params)<specs>?:ReturnType = body
    // ─────────────────────────────────────────────

    function_definition: $ => seq(
      field('name', $.identifier),
      optional(field('specifiers', $.specifier_list)),
      '(',
      optional(field('parameters', $.parameter_list)),
      ')',
      optional(field('effect_specifiers', $.specifier_list)),
      ':',
      field('return_type', $._type),
      '=',
      field('body', $._function_body),
    ),

    // Function body: indented block, brace block, or inline expression
    _function_body: $ => choice(
      $.brace_block,
      $.indented_block,    // INDENT stmts DEDENT (after '=')
      $._expression,       // inline expression on same line
    ),

    // ─────────────────────────────────────────────
    // Type definitions (via :=)
    //   Name<specs>? := class/struct/enum/interface/module/expr
    // ─────────────────────────────────────────────

    type_definition: $ => seq(
      field('name', $.identifier),
      optional(field('specifiers', $.specifier_list)),
      ':=',
      field('value', choice(
        $.class_definition,
        $.struct_definition,
        $.enum_definition,
        $.interface_definition,
        $.module_definition,
        $._expression,
      )),
    ),

    // ─────────────────────────────────────────────
    // Class / Struct / Enum / Interface / Module
    // ─────────────────────────────────────────────

    class_definition: $ => seq(
      'class',
      optional(field('specifiers', $.specifier_list)),
      optional(seq('(', field('base', $.identifier), ')')),
      $._type_body,
    ),

    struct_definition: $ => seq(
      'struct',
      optional(field('specifiers', $.specifier_list)),
      $._type_body,
    ),

    enum_definition: $ => seq(
      'enum',
      optional(field('specifiers', $.specifier_list)),
      $._enum_body,
    ),

    interface_definition: $ => seq(
      'interface',
      optional(field('specifiers', $.specifier_list)),
      optional(seq('(', field('base', $.identifier), ')')),
      $._type_body,
    ),

    module_definition: $ => seq(
      'module',
      $._type_body,
    ),

    // Type body: ':' INDENT members DEDENT  OR  '{ members }'
    _type_body: $ => choice(
      seq(':', $._indent, repeat($._class_member), $._dedent),
      seq('{', repeat($._class_member), '}'),
    ),

    _enum_body: $ => choice(
      seq(':', $._indent, repeat($._enum_variant), $._dedent),
      seq('{', repeat($._enum_variant), '}'),
    ),

    _class_member: $ => choice(
      $.var_declaration,
      $.function_definition,
      $.field_declaration,
      $.type_definition,
      $.set_statement,
    ),

    // Field declaration inside class/struct: Name<specs>?:type (= default)?
    field_declaration: $ => seq(
      field('name', $.identifier),
      optional(field('specifiers', $.specifier_list)),
      ':',
      field('type', $._type),
      optional(seq('=', field('default', $._expression))),
    ),

    _enum_variant: $ => seq(
      field('name', $.identifier),
      optional(seq('(', field('type', $._type), ')')),
    ),

    // ─────────────────────────────────────────────
    // Specifiers: <keyword> or <keyword{ident}>
    // ─────────────────────────────────────────────

    specifier_list: $ => repeat1($.specifier),

    specifier: $ => seq(
      token.immediate('<'),
      field('name', alias($._specifier_name, $.specifier_name)),
      optional(seq('{', field('value', $.identifier), '}')),
      '>',
    ),

    _specifier_name: $ => choice(
      'abstract', 'computes', 'constructor', 'private', 'public', 'protected',
      'final', 'decides', 'inline', 'native', 'override', 'suspends', 'transacts',
      'internal', 'reads', 'writes', 'allocates', 'scoped', 'converges',
      'castable', 'concrete', 'unique', 'final_super', 'open', 'closed',
      'native_callable', 'module_scoped_var_weak_map_key', 'epic_internal',
      'persistable',
    ),

    // ─────────────────────────────────────────────
    // Parameter lists
    // ─────────────────────────────────────────────

    parameter_list: $ => seq(
      $._parameter,
      repeat(seq(',', $._parameter)),
    ),

    _parameter: $ => choice(
      $.named_parameter,
      $.positional_parameter,
      $.tuple_parameter,
    ),

    positional_parameter: $ => seq(
      field('name', $.identifier),
      ':',
      field('type', $._type),
    ),

    named_parameter: $ => seq(
      '?',
      field('name', $.identifier),
      ':',
      field('type', $._type),
      optional(seq('=', field('default', $._expression))),
    ),

    tuple_parameter: $ => seq(
      '(',
      $._parameter,
      repeat(seq(',', $._parameter)),
      ')',
    ),

    // ─────────────────────────────────────────────
    // Types
    // ─────────────────────────────────────────────

    _type: $ => choice(
      $.identifier,
      $.qualified_type,
      $.optional_type,
      $.array_type,
      $.map_type,
      $.function_type,
      $.tuple_type,
      // built-in type keywords (without 'tuple'/'type' which have special forms)
      alias(choice(
        'int', 'float', 'string', 'logic', 'char', 'any', 'void',
        'comparable', 'rational',
      ), $.builtin_type),
    ),

    qualified_type: $ => seq($.identifier, '.', $.identifier),

    optional_type: $ => seq('?', $._type),

    array_type: $ => seq('[', ']', $._type),

    map_type: $ => seq('[', field('key', $._type), ']', field('value', $._type)),

    // type{_(<params>)<specs>:<return>}
    function_type: $ => seq(
      'type',
      '{',
      '_',
      '(',
      optional($.parameter_list),
      ')',
      optional($.specifier_list),
      ':',
      $._type,
      '}',
    ),

    tuple_type: $ => seq(
      'tuple',
      '(',
      $._type,
      repeat(seq(',', $._type)),
      ')',
    ),

    // ─────────────────────────────────────────────
    // Blocks
    // ─────────────────────────────────────────────

    // Brace block: { stmts... }
    brace_block: $ => seq(
      '{',
      repeat($._block_item),
      '}',
    ),

    // Colon block: ':' INDENT stmts DEDENT  (for if/for/loop/sync/etc.)
    colon_block: $ => seq(
      ':',
      $._indent,
      repeat($._block_item),
      $._dedent,
    ),

    // Dot block: '.' expr  (single expression, for if/loop inline forms)
    dot_block: $ => seq('.', $._expression),

    // Indented block after '=': INDENT stmts DEDENT  (for function bodies)
    indented_block: $ => seq(
      $._indent,
      repeat($._block_item),
      $._dedent,
    ),

    _block_item: $ => choice(
      $.var_declaration,
      $.set_statement,
      $.function_definition,
      $.type_definition,
      $._expression,
    ),

    // ─────────────────────────────────────────────
    // Literals
    // ─────────────────────────────────────────────

    _literal: $ => choice(
      $.integer_literal,
      $.float_literal,
      $.string_literal,
      $.char_literal,
      $.boolean_literal,
      $.path_literal,
    ),

    integer_literal: $ => token(choice(
      /0[xX][0-9a-fA-F][0-9a-fA-F_]*/,
      /0[oO][0-7][0-7_]*/,
      /0[bB][01][01_]*/,
      /[0-9][0-9_]*/,
    )),

    float_literal: $ => token(choice(
      /[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?f64/,
      /[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?/,
      /[0-9]+[eE][+-]?[0-9]+/,
    )),

    string_literal: $ => seq(
      '"',
      repeat(choice(
        $.string_content,
        $.escape_sequence,
        $.string_interpolation,
      )),
      '"',
    ),

    string_content: $ => token.immediate(/[^"\\{]+/),

    escape_sequence: $ => token.immediate(
      seq('\\', /[tnr"'\\{}<>#~&]/),
    ),

    string_interpolation: $ => seq(
      '{',
      optional($._expression),
      '}',
    ),

    char_literal: $ => token(choice(
      seq("'", /[^'\\\n]/, "'"),
      seq("'", /\\[tnr"'\\{}<>#~&]/, "'"),
      /0[oO][0-9a-fA-F]{2}/,
      /0[uU][0-9a-fA-F]{1,6}/,
    )),

    boolean_literal: $ => choice('true', 'false'),

    path_literal: $ => token(
      seq(
        '/',
        /[a-zA-Z@][a-zA-Z0-9.\-_]*/,
        repeat(seq('/', /[a-zA-Z_][a-zA-Z0-9_]*/)),
      ),
    ),

    // ─────────────────────────────────────────────
    // Expressions
    // ─────────────────────────────────────────────

    _expression: $ => choice(
      $._literal,
      $.identifier,
      $.self_expression,
      $.wildcard,
      $.parenthesized_expression,
      $.tuple_expression,
      $.unary_expression,
      $.binary_expression,
      $.range_expression,
      $.query_expression,
      $.call_expression,
      $.failable_call_expression,
      $.member_access,
      $.index_expression,
      $.object_construction,
      $.array_literal,
      $.logic_literal,
      $.option_expression,
      $.if_expression,
      $.for_expression,
      $.loop_expression,
      $.block_expression,
      $.case_expression,
      $.sync_expression,
      $.race_expression,
      $.branch_expression,
      $.spawn_expression,
      $.when_expression,
      $.break_expression,
      $.return_expression,
      $.yield_expression,
    ),

    self_expression: $ => 'Self',

    wildcard: $ => '_',

    parenthesized_expression: $ => seq('(', $._expression, ')'),

    tuple_expression: $ => seq(
      '(',
      $._expression,
      ',',
      $._expression,
      repeat(seq(',', $._expression)),
      ')',
    ),

    unary_expression: $ => prec.right(PREC.UNARY, seq(
      field('operator', choice('not', '-', '+')),
      field('operand', $._expression),
    )),

    binary_expression: $ => choice(
      prec.left(PREC.MULT, seq(
        field('left', $._expression),
        field('operator', choice('*', '/')),
        field('right', $._expression),
      )),
      prec.left(PREC.ADD, seq(
        field('left', $._expression),
        field('operator', choice('+', '-')),
        field('right', $._expression),
      )),
      prec.right(PREC.COMPARE, seq(
        field('left', $._expression),
        field('operator', choice('=', '<>', '<', '<=', '>', '>=')),
        field('right', $._expression),
      )),
      prec.left(PREC.AND, seq(
        field('left', $._expression),
        field('operator', 'and'),
        field('right', $._expression),
      )),
      prec.left(PREC.OR, seq(
        field('left', $._expression),
        field('operator', 'or'),
        field('right', $._expression),
      )),
    ),

    range_expression: $ => prec.left(PREC.RANGE, seq(
      field('start', $._expression),
      '..',
      field('end', $._expression),
    )),

    query_expression: $ => prec.left(PREC.CALL, seq(
      field('operand', $._expression),
      '?',
    )),

    call_expression: $ => prec.left(PREC.CALL, seq(
      field('function', $._expression),
      '(',
      optional(field('arguments', $.argument_list)),
      ')',
    )),

    failable_call_expression: $ => prec.left(PREC.CALL, seq(
      field('function', $._expression),
      '[',
      optional(field('arguments', $.argument_list)),
      ']',
    )),

    member_access: $ => prec.left(PREC.CALL, seq(
      field('object', $._expression),
      '.',
      field('member', $.identifier),
    )),

    index_expression: $ => prec.left(PREC.CALL, seq(
      field('object', $._expression),
      '[',
      field('index', $._expression),
      ']',
    )),

    object_construction: $ => prec.left(PREC.CALL, seq(
      field('type', $._expression),
      '{',
      optional(field('fields', $.field_initializer_list)),
      '}',
    )),

    field_initializer_list: $ => seq(
      $.field_initializer,
      repeat(seq(',', $.field_initializer)),
    ),

    field_initializer: $ => seq(
      field('name', $.identifier),
      ':=',
      field('value', $._expression),
    ),

    argument_list: $ => seq(
      $._argument,
      repeat(seq(',', $._argument)),
    ),

    _argument: $ => choice(
      $.named_argument,
      $._expression,
    ),

    named_argument: $ => seq(
      '?',
      field('name', $.identifier),
      ':=',
      field('value', $._expression),
    ),

    // array{expr, ...}
    array_literal: $ => seq(
      'array',
      '{',
      optional(seq(
        $._expression,
        repeat(seq(',', $._expression)),
      )),
      '}',
    ),

    // logic{expr; expr} — converts failable to bool
    logic_literal: $ => seq(
      'logic',
      '{',
      optional(seq(
        $._expression,
        repeat(seq(choice(';', ','), $._expression)),
      )),
      '}',
    ),

    // option{expr}
    option_expression: $ => seq(
      'option',
      '{',
      $._expression,
      '}',
    ),

    // ─────────────────────────────────────────────
    // Control flow
    // ─────────────────────────────────────────────

    // _control_block: body forms used by if/for/loop/sync/race/branch
    _control_block: $ => choice(
      $.brace_block,
      $.colon_block,
      $.dot_block,
    ),

    // if (cond, ...): block [else if ...] [else block]
    // if (cond) then expr [else expr]
    // if: INDENT cond DEDENT then block [else block]
    if_expression: $ => prec.right(seq(
      'if',
      choice(
        seq(
          '(',
          field('condition', $.if_condition),
          ')',
          field('consequence', $._control_block),
          repeat(seq('else', 'if', '(', field('condition', $.if_condition), ')', field('consequence', $._control_block))),
          optional(seq('else', field('alternative', $._control_block))),
        ),
        seq(
          '(',
          field('condition', $.if_condition),
          ')',
          'then',
          field('consequence', $._expression),
          optional(seq('else', field('alternative', $._expression))),
        ),
        seq(
          ':',
          $._indent,
          field('condition', $.if_condition),
          $._dedent,
          'then',
          field('consequence', $._control_block),
          optional(seq('else', field('alternative', $._control_block))),
        ),
      ),
    )),

    if_condition: $ => seq(
      $._expression,
      repeat(seq(',', $._expression)),
    ),

    for_expression: $ => seq(
      'for',
      '(',
      field('clauses', $.for_clause_list),
      ')',
      field('body', $._control_block),
    ),

    for_clause_list: $ => seq(
      $._for_clause,
      repeat(seq(',', $._for_clause)),
    ),

    _for_clause: $ => choice(
      $.for_iterator,
      $._expression,
    ),

    for_iterator: $ => seq(
      field('variable', $.identifier),
      choice(':', '->'),
      field('iterable', $._expression),
    ),

    loop_expression: $ => seq('loop', field('body', $._control_block)),

    block_expression: $ => seq('block', field('body', $._control_block)),

    // case(value): INDENT arm DEDENT
    case_expression: $ => seq(
      'case',
      '(',
      field('value', $._expression),
      ')',
      ':',
      $._indent,
      repeat1($.case_arm),
      $._dedent,
    ),

    case_arm: $ => seq(
      field('pattern', $._expression),
      '=>',
      field('body', $._expression),
    ),

    sync_expression: $ => seq('sync', field('body', $._control_block)),
    race_expression: $ => seq('race', field('body', $._control_block)),
    branch_expression: $ => seq('branch', field('body', $._control_block)),

    spawn_expression: $ => seq(
      'spawn',
      '{',
      $._expression,
      '}',
    ),

    when_expression: $ => seq(
      'when',
      '(',
      field('condition', $._expression),
      ')',
      field('body', $._control_block),
    ),

    break_expression: $ => prec.right(seq(
      'break',
      optional(field('value', $._expression)),
    )),

    return_expression: $ => prec.right(seq(
      'return',
      optional(field('value', $._expression)),
    )),

    yield_expression: $ => prec.right(seq(
      'yield',
      optional(field('value', $._expression)),
    )),

    // ─────────────────────────────────────────────
    // Identifier
    // ─────────────────────────────────────────────

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,
  },
});
