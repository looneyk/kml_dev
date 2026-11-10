# Parser Functions --- Plan

The functions of `quetzal::expression` are `abs`, `min`, `max`, `mod`, and `str`, each defined in two places: a
signature (name, argument types, return type) in `Parser::init_functions`, and an implementation in a branch of
`Evaluator<T>::apply_function`. A client cannot add a function, and an implementation has no state. A client is to
add any function that meets the constraints below, such as `random` (`pattern_resolution_plan.md`, step 4),
which draws from a random engine. Part 1 adds functions with little change to the current structure, and part 2
replaces that structure with a function table that a client builds and passes in the way it passes a `SymbolTable`.
The two parts are alternative branches.

Libraries that let a client add functions register a name with a callable and its signature: muparser
`Parser::DefineFun`, exprtk `symbol_table::add_function`, and SQLite `sqlite3_create_function`. exprtk keeps its
functions in its symbol table, and muparser and SQLite keep them in the parser or connection.

## Constraints on a function

- **A function has a fixed number of arguments,** each with a type.
- **An argument and a return value are a number or a string,** and in part 2 an array (`### Arrays`).
- **An argument is never null,** and a null argument is a parse error, as it is now.
- **A name has one signature.** A function is not overloaded by argument types.
- **An argument is never converted to another type.** `str` converts a number to a string explicitly.
- **A name is a function call if it is a function,** and an identifier otherwise.
- **An implementation can have state,** such as a random engine.

## Part 1: functions added to the current structure

- **`Parser::add_function(name, argument_types, return_type)` adds a signature** to `m_functions`, beside the
  built-ins that `init_functions` adds. The signature type becomes public. A name that is already a function is
  replaced.
- **An argument is evaluated as its own type,** given by its node's `DataType`, not as `T`. A function whose argument
  types differ from its return type then needs no branch of its own, as `str` has now.
- **`Expression::evaluate<T>(symbol_table, call)` takes a callable** for the functions that are not built in:
  `std::function<Symbol(std::string_view name, const std::vector<Symbol>& arguments)>`. Arguments and the result are
  `Symbol`s, a type and text, the form the symbol table already uses. `apply_function` calls it for a name that has no
  branch, and an empty callable throws as an unknown function does now.
- **A function with state is implemented by the client's callable,** which reaches the state when it is called:
  `PatternResolver` adds its functions' signatures to its parser and passes a callable that draws from `m_random`.
- **A client's function is then defined in two places,** the signature and the branch of its callable, and dispatched
  by comparing names.

## Part 2: a function table

- **A `FunctionTable` maps a name to a `Function`:** its argument types, its return type, and its implementation, a
  `std::function<Value(std::span<const Value> arguments)>`. It is in `FunctionTable.hpp` and `.cpp`.
- **A `Value` is a number, a string, or an array.** The evaluator evaluates every node to a `Value`, without a
  template, and `Expression::evaluate<T>` converts the result to `T`. A symbol's text is converted when the identifier
  is evaluated.
- **The table is passed in, like the symbol table:** `Parser::parse(text, symbol_table, function_table)` uses the
  signatures, and `Expression::evaluate<T>(symbol_table, function_table)` the implementations. `Parser` has no
  functions of its own, and `next` classifies a name as a function call if it is in the table.
- **`builtin_functions()` returns a table of the built-in functions,** and a client copies it and adds its own with
  `add(name, argument_types, return_type, implementation)`.
- **A function with state captures it,** at an address that does not change while the table exists.
  `PatternResolver` keeps its random engine in a `std::unique_ptr`, so a move of the resolver leaves the captures
  valid, and builds its table once, in its constructor.

### Arrays

- **`DataType::Array` is the type of an array of values,** written as a literal `[a, b, c]`.
- **A symbol can be an array,** so a pattern can draw from a list in a store value, such as a weighted choice from
  `region.primary` (`region_spec_plan.md`, `### Values`).
- **A literal number is kept as its text in its node,** as it is now, since a pattern is evaluated once and the
  formatter writes a literal from its text.

## Steps

### Part 1

- [ ] 1. **Arguments evaluated as their own type.** `apply_function` evaluates each argument by its node's type, and
  `str` loses its special case.
- [ ] 2. **Added signatures.** `Parser::add_function` and the public signature type, with tests of a function added
  to a parser.
- [ ] 3. **A callable for added functions.** `Expression::evaluate<T>` takes the callable, and `apply_function` calls
  it for a name with no branch.
- [ ] 4. **The resolver's functions.** `PatternResolver` adds `random` to its parser and passes the callable.

### Part 2

- [ ] 1. **Values.** `Value`, and the evaluator evaluating every node to one.
- [ ] 2. **The function table.** `FunctionTable`, `Function`, and `builtin_functions()`, with the built-ins
  implemented as entries of the table.
- [ ] 3. **The table passed in.** `Parser::parse` and `Expression::evaluate<T>` take the table, and `Parser`'s
  functions and `apply_function` go.
- [ ] 4. **The resolver's table.** `PatternResolver` builds its table from `builtin_functions()` with `random` added,
  its random engine in a `std::unique_ptr`.
- [ ] 5. **Arrays.** `DataType::Array`, array literals, and array symbols.

## Open questions

- **Are an array's elements all one type?** A weighted table mixes strings and weights, so it is either an array of
  mixed values or an array of pairs.
- **What is an array symbol's text?** The literal syntax, or the form of a store value such as
  `{fields 4, forest 4}`.

## Documents revised on approval

- `pattern_resolution_plan.md`, step 4: `random` added with the mechanism built here.
- `work_plan.md`: an item for this plan, and the open questions in the Open section.
