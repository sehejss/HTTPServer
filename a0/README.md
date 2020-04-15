AUTHOR
      Sehej Sohal
      sssohal@ucsc.edu
      1650056
NAME
       dog - concatenate files and print on the standard output

INSTRUCTIONS
       make dog - builds dog executable.
       make clean - removes executable.
       make spotless - removes all unneeded files.
       ./unitTest.sh - runs self-written tests.

ASSUMPTIONS
       Assuming that, since cat can in fact take two -'s as arguments, e.g: cat - -
       and prints to stdout until EOF, and then prints to stdout until a second
       EOF, that it is ok to do so in dog. It was stated in the assignment spec that
       cat can only handle one dash, but that does not seem to be the case.

SYNOPSIS
       ./dog [OPTION]... [FILE]...

DESCRIPTION
       Concatenate FILE(s) to standard output.

       With no FILE, or when FILE is -, read standard input.

EXAMPLES
       ./dog f - g
              Output f's contents, then standard input, then g's contents.

       ./dog    Copy standard input to standard output.
