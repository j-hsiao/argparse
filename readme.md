# Argparse
C++ template argument parsing library.
There are 2 types of arguments: flags and positionals.  Flags are
given by first giving the argument name prefixed with the prefix char
(usually `-`) and then the corresponding arguments.  Positional args
are given by position.  Multivalue arguments expect the arguments to
be consecutive and will be interrupted by flags.  Special flags are
the empty flag and integer flags (`--` or `--N` where `N` is an int).
These special flags indicate the next `N` arguments are to be treated
as positional arguments.  In the case of the empty flag, all remaining
arguments will be treated as positional arguments.  These can be useful
for giving negative numbers as arguments, though an alternative is to
prefix it with an escaped space.

Flag arguments match the first exact match.  If there are no exact
matches, then the longest unique prefix match is used.  If the longest
match is not unique, then parsing is failed due to ambiguity.

There is also an implicit `-help` argument.  `-help` also follows
the above rules.  However, there is special handling for the case
where 2 prefix chars are used.  In this case, the flags will
preferentially match with help.  That is to say, `--h`, `--he`,
`--hel`, `--help` will all match with -help always while the
corresponding `-h`, `-he`, `-hel`, `-help` will match with any
arguments first, and then `-help` only if no arguments were matched.

When the `-help` flag is matched,  there are 2 possible messages.
If the argument was incomplete (`-h`, `-he`, `-hel`), then a short
help message will be given.  Otherwise (`-help`), the full help message
will be given.

In the help message, required arguments will be surrounded by `<>`
and optional arguments will be surrounded by `[]`.  The argument
may have additional specifier to indicate the number of required
commandline arguments.
* `xN`: requires exactly N arguments
* `...`: variable number of arguments.
* `++`: a counting flag
* `!!`: a toggling flag

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

## Parsing the argument list.
`int parse(int argc, char *argv[] [, const char *program])`

This parses the argument list.  If program is given, it is the name
of the program to be used in the help messages.  Otherwise, `argv[0]`
is assumed to be the name of the program.  Return `0` on success, `1` if
a help flag was encountered, and `2` if there was a parse error.  When
`1` or `2` is returned, the appropriate messages are printed to stderr.

example:

```
#include <argparse/argparse.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
  argparse::parser p("description", "-");
  auto arg = p.add<const char*>(
    "name of arg", "description of arg",
    {"optional initializer list of default values"});
  auto twonums = p.add<int, 2>("twonums", "2 positional integers");
  auto variablenums = p.add<float, -1>(
    "variablenums", "variable numbers", {1, 2, 3});

  if (int result = p.parse(argc, argv))
  {
    if (result == 1) { return 0; }
    else
    {
      std::cerr << "error parsing arguments." << std::endl;
      return 1;
    }
  }

  std::cerr << "the first arg was " << arg << std::endl;

  std::cerr << "the 2 numbers were << twonums[0] << ", "
    << twonums[1] << std::endl;

  std::cerr << "Got " << variablenums.size() << " floats" << std::endl;
}
```


