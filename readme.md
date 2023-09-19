# Argparse
C++ template argument parsing library.
There are 2 types of arguments: flags and positionals.

Flags are given by first giving the argument name prefixed with the
prefix char (usually `-`) and then the corresponding arguments.  They
can be short (single char and single prefix char) or long (full name and
double prefix char).  The short form can have an optional space followed
by corresponding values.  The longform must appear alone followed by any
corresponding values.  Shortform flags can be merged into a single
argument as long as they take no arguments (bool flags).  non-bool flags
can be merged as well as long as they are last.  This means at most 1
non-bool flag is allowed per flag argument.

example:

flags: `-v|--verbose`, `-a|--auto`, `-n|--name` `<name>`, `-c|--color` `<color>`

allowable arguments:

`-va`: verbose and auto
`-av`: auto and verbose
`-n` `<name>`: specify a name
`-n<name>`: specify a name
`-avn` `<name>`: auto, verbose and with name
`-avn<name>`: auto, verbose and with name (space is optional for short form)
`-avc` `<color>`: auto, verbose and with color

bad arguments:

`-vca`: c is short form of color. color requires 1 arg, but c is not last.
`-anc`: n is short form of name. name requires 1 arg, but n is not last.
`--color<color>`: long form requires space

Special flags are flags of the form `--` or `--N` where `N` is a
nonnegative integer.  `--` means treat all remaining arguments as
positional arguments.  `--N` means treat the next N arguments as
positional arguments.  `--0` is a special case that interrupts
multivalue positional arguments.

Positional args are given by position.  Multivalue arguments expect the
arguments to be consecutive, uninterrupted by normal flags.  Special
flags besides `--0` are allowed though.

Argument names must be unique and each argument is allowed at most 1
full name (more than 1 char.)

There is also an implicit `-h|--help` argument.  `-h` triggers a short
help message.  `--help` triggers a long help message.

In the help message, required arguments will be surrounded by `<>`
and optional arguments will be surrounded by `[]`.  The argument
may have additional specifier to indicate the number of required
commandline arguments.
* `xN`: requires exactly N arguments
* `...`: variable number of arguments.
* `++`: a counting flag (count == 1, type bool)
* `!!`: a toggling flag (count == 0, type bool)

# Usage
Instantiate an `argparse::Parser`, add arguments, and then parse.
All pointer arguments are taken as is so their lifetime should be longer
than the parser and corresponding argument structs.

## Constructing the parser
`Parser(const char *help="", const char *prefix="-")`

`help` is a help message describing the overall program.

`prefix` is a 1-char string that indicates the prefix character for
flag arguments.

## Adding arguments.
`structtype add<type, count=1>(
  const char *name, const char *help [, std::initializer_list<type> defaults])`

`structtype add<type, count=1>(
  std::initializer_list<const char*> names, const char *help
  [, std::initializer_list<type> defaults])`

This returns a struct that can act as the corresponding argument.  The
underlying data is accessible via the `.data` member.  The struct also
has dereference and arrow operator.  Multi args also have conversion
operator to `T*`.

`type`: The type of the argument.  Integral arguments are parsed using
base 10.  To use other bases, use `argparse::Base<type, base>` as the
`type`.

`count`: The number of arguments in the argument list that corresponds
to this argument.  This determines the underlying data container for the
argument:
* `count < 0`: The argument is variable-length and the underlying
  container is an `std::vector<type>`.  The struct will define members
  `operator[]()`, `size()`, `begin()`, and `end()`.
* `count == 1`: This indicates a single value.  The `.data` member is
  `type`.
* `count > 1`: This is similar to the `count < 0` case, except the
  length is fixed and the underlying container is
  `std::array<type, count>`

This has some exceptions when `type` is `bool`:
* `count == 1`: In this case, the argument must be a flag and the flag
  is a counting flag.  That is to say, the occurrences of the flag
  are counted and `data` is an int that corresponds to the number of
  times the flag is given.
* `count == 0`: In this case, the argument must be a flag and the flag
  is a toggle flag.  That is to say, every time the flag is encountered,
  the value of the argument is flipped.

To help remember which is which, 0 is a circle so the bool will cycle
between true/false with each successive flag.  1 is more like a
line/arrow so it increments with each flag.

`name`: The name of the argument.  If it starts with the prefix char,
it is a flag.  Otherwise, it is a positional argument.  In the case of
the initializer list, the 1st string is the flag and the rest are
aliases that point to the flag.  Aliases are essentially the same as
normal flags for flag matching purposes.  However, the specified flag
and corresponding aliases all share the same struct.

`help`: A help message describing the argument.

`defaults`: an initializer list for the default values of the argument.
If given, then the argument is considered an optional argument.
Otherwise, it is a required argument.  It can be empty to mark the arg
as optional but without any explicit default values.  Otherwise, it
should match the length of the argument.

## Argument groups
`auto mygroup = parser.group("groupname")`
`mygroup.add<tp, num=1>(...)`
Returns a group struct which can be used to add arguments under a group.
Groups will group arguments together in the help message. The add method
has the same signature is the Parser.  Adding with Parser adds to the
default group.

## Parsing the argument list.
`argparse::ParseResult parse(int argc, char *argv[] [, const char *program])`

This parses the argument list.  If program is given, it is the name
of the program to be used in the help messages.  Otherwise, `argv[0]`
is assumed to be the name of the program.

ParseResult has a `code` attribute which indicates parsing success.  It
is 0 on success, 1 if parsing was interrupted by a help flag, and 2 if
parsing failed due to an error.

ParseResult also has a `bool parsed(const char *name)` member function
that returns whether an argument was parsed.  This is most useful for
checking if optional arguments were given when all values are valid.

See tests for examples.
