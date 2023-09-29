# Argparse
## Overview
C++ commandline argument parsing.

In general, there are 2 types of arguments.  Positional arguments are
arguments that are parsed based on their position in the commandline.
Flags are arguments that begin with a prefix, usually "-".  The prefix
character is assumed to be "-" for the rest of this doc.  Flags can
appear in any order and can be short or long.  Short flags begin with a
single prefix character and are specified by a single character.  Long
flags are prefixed by 2 prefix characters and a full name of the flag.
Long flags must be followed by a space.  A space before the first
argument to short flags is optional.  Short boolean flags can be
combined as a single argument.

All flag names must be unique.  Positional arguments can have the same
name, but this is discouraged because it can be confusing.  If there
are multiple positional arguments with the same name, then the full help
message will indicate the positional index of the argument.

Arguments can be multi-valued in which case the corresponding values
should be consecutive.  They will be interrupted by flags or the
`--0` special flag.  The `--` or `--N` special flags will not interrupt
multi-valued sequences.

## short flag examples:
`-v|--verbose`: bool, `-d|--debug`: bool, `-c|--count`: int

`-vd` = `-v -d` = `--verbose -d` = `--verbose --debug` ...

`-vc5` = `-vc 5` = `-v -c 5` = `-v --count 5` ...

bad: `-vc5d`

c takes an argument and there are trailing characters for c.  This means
that `5d` is the argument which is not a valid int (base 10).

## Special flags
### `--`
`--` means to treat all remaining arguments as
positional arguments regardless of whether or not they begin with a prefix
character.

### `--N`
`--N` where N is an integer has a similar meaning to `--` except it only
treats the next N arguments as positional arguments regardless of prefix
characters.

### `--0`
`--0` stops variable-length parsing.  There must be at least 2 prefix
characters followed by a 0.  The number of dashes indicates the level
at which it should stop.  For example, a variable-length argument
consisting of variable-length sequence of ints, `--0` would only stop
the lowest level and start the next variable-length list.  `---0` would
stop both the lowest level and the higher level.

example:
`1 2 3 --0 --0 4 5 6 ---0 7`

When parsing the above arguments as a list of list of ints they would
be `[[1, 2, 3], [], [4, 5, 6]]`.  The variable length list takes the
`1`, `2`, and `3`.  At the `--0`, it is interrupted and ends the first
list.  The next argument is a `--0` so the next list is interrupted
immediately, resulting in an empty list.  The next list is then `4`,
`5`, `6`.  The `---0` interrupts both the current list and the list
of lists.  As a result, the `7` would be left for the next argument.

### `-h|--help`
`-h` and `--help` trigger a help message.  `-h` prints a short version
of the help messages that just show the arguments and their counts.
`--help` will trigger the full help message complete with defaults,
descriptions, etc.  Arguments named `h` or `help` will take precedence
over the default `-h` and `--help` flags.
The count descriptions are:
* `xN`: exactly N values.
* `...`: variable number of values.
* `++`: bool flag.  Increments a value for every occurrence.
* `!!`: bool flag.  Inverts a boolean value for every occurrence.
Required arguments will be surrounded in `<>` and optional arguments
will be surrounded in `[]`.

## Usage
See `test/demo.cpp` for example usage.

All classes and functions are in the `argparse::` namespace.

### Constructing the parser
```
Parser(
  const char *description=nullptr,
  char prefix='-',
  std::ostream& out=std::cerr)
```

`description` is a description of what the program does if given.

`prefix` indicates the prefix character to use

`out` is the output stream to print any help or error messages to.

### Arguments.
Positional arguments use the `Arg` type.  Flags use the `Flag` type.
Both flags and positional arguments have the same general interface.
Arguments wrap a value which is modified directly during parsing.
A type and a count determine the type that is wrapped and are given as
template paramters.  The count defaults to `1`.

Multivalue arguments (count > 1 or count < 0) have operator[] defined
allowing indexing on the underlying `std::vector` or `std::array`.
All args have `*` and `->` operators defined to access the underlying
value.  Alternatively, the value can be accessed as the `.data` member.

#### type examples:
* `Arg<int, 1>`: `int`
* `Arg<int, N>`: `std::array<int, N>`, where `N > 1`
* `Arg<int, -1>`: `std::vector<int>`
* `Arg<std::vector<int>, 2>`: `std::array<std::vector<int>, 2>`
* `Arg<std::vector<int>, -1>`: `std::vector<std::vector<int>>`
* `Flag<bool, 1>`: int: increments the int whenever the argument is encountered.
* `Flag<bool, 0>`: bool: toggles the bool whenever the argument is encountered.

`<bool, 1|0>` makes most sense for flags and are not tested for
positional arguments.  The other `<type, count>` are applicable to both
flags and positionals.

The `Base<type, base>` type has conversion operators to the underlying
`type` as well as `*` operator for explicit access.  Alternatively the
value can be accessed by the `.data` member.

#### Argument Constructors
Argument constructors take a few arguments.

`Arg(parser, name, help, defaults)`

`parser` is the `Parser` instance to add the argument to.

`name` is the name (`const char*`) or names (`std::initializer_list<const char*>`)
for the argument.  Note that even if it is a `Flag` argument, the names
should not begin with a prefix char.  This allows easy changing of the
prefix char via the `Parser` argument if needed.

`help` is the help message for the argument (`const char*`).  nullptr is
acceptable to indicate no help message.  The help message is used in the
generated full help message.

`defaults` is optional.  If `defaults` are provided, the argument is
classified as optional.  Otherwise, it is required.  `defaults` should
match whatever type the argument is.  std::array also has an overload
that allows using empty braces `{}` to indicate no particular default
but that the argument is also not required.

To parse numbers not in base 10, the `Base<type, base>` template can
be used as the type instead.  `Flag<Base<int, 8>, 1>` will result
in a flag argument that expects a single octal int.  The wrapped type
will be `Base<int, 8>`.

### Parsing Arguments.
Use `Parser::parse(...)` to parse arguments.  There are several
overloads of the `parse` method for convenience.

* `parse(int argc, char *argv[])`: The arguments should have the same
structure as those to `main()`: (The program name is assumed to be
argv[0]).
* `parse(int argc, char *argv[], const char *program)`: Parse arguments.
Here, the program name is explicitly given, so `argv` is assumed to only
contain arguments to parse.
* `parse(array, const char *program)`:  This overload allows more
convenient parsing of values in an array by deducing the number of
arguments.
* `parse(ArgIter&, const char *program)`: Takes an ArgIter which should
contain only the arguments to parse.

Parsing returns a `ParseResult`.
ParseResult contains an `int code` member that indicates the status of
the parsing.  `success` means the parse was successful.
`help` means a help message was printed.  `missing` means there was
a missing required argument.`  `unknown` means that an unknown argument
was encountered.  `error` means an error occurred during parsing.  This
could range from invalid value, missing values, or anything else.

`ParseResult` has an `operator bool()` that converts to `true` if the
code is not `success`.  It also has a `bool parsed(arg)` method
to check if a value was actually parsed or not.  This can be useful
for optional arguments if it matters whether it was parsed or not.

### Argument groups
Groups can be instantiated with the Group type `Group(parser, name)`.
The group can be used in place of the parser when instantiating
arguments.  In regards to parsing, the effect is the same as just
instantiating it with the parser, but when generating a full help,
arguments in the same group will be grouped together.  This can make
reading the help message a little easier.

### Custom types
Custom types can be used with argparse by defining a
`int parse(Type &, ArgIter &it)` method.  This method could be an
overload in the `argparse::` namespace, but the most appropriate would
be in whatever namespace the custom type is in.  The method will be
found via adl.  The members probably individually already have parse
methods that you can use.

example:
```
struct MyCustomType
{
  int v;
  const char *name;
  ...
};

int parse(MyCustomType &val, ArgIter &it)
{
  return argparse::parse(val.v) && argparse::parse(val.name);
}

using namespace argparse;
Parser p("custom type example");

Arg<MyCustomType, 1> somearg(p, "myarg", "An argument of type MyCustomType");

const char *args[] = {"5", "bananas"};
ParseResult result = p.parse(args, "dummy_program");

assert(somearg->v == 5);
assert(somearg->name == std::string("bananas"));
```
