SummaryPlugin - A clang plugin to emit compilation statistics.
==============================================================

What
----
Wouldn't it be nice if the compiler presented you with the number of lines it just
compiled?  This tool can produce a more accurate line count than merely
grep-counting semicolons, or trying to count non-whitespace lines
in uncommented code.  Additionally, the user can customize the
output string to have the compiler present additional information.

Format String
-------------
Everyone has different preferences.  The plugin accepts a user-defined string
provided as a clang command line argument `-fmt` or environment variable
`SUMMARYPLUGIN_FMT`.  These options are used to tailor the SummaryPlugin's
output to your desire (or emit nothing, which is such a heinous thought).  If
both formatting options are specified, the environment variable will take
precedence.

Check the top of the source code (`SummaryPlugin.cpp`) for the conversion
variables `ConversionChar` that can be used in the format string.  Each
conversion character is prefixed with a `%` and will be symbolically replaced
with the item it represents.

How
---
To use this clang plugin the required pieces you need to pass to clang are the
following.  Note that if you want to use this like a tool, and not generate an
object file, replace `-add-plugin` with `-plugin`.
~~~~
-fplugin=SummaryPlugin.so -Xclang -add-plugin -Xclang summary
~~~~

Options can be added, such as:
~~~~
-plugin-arg-summary -Xclang -fmt="I just compiled %L lines from %F."
~~~~

Building
--------
Compile using the provided `Makefile`. Ensure that the `CXX`, and `CFG`
variables are using the desired version of the compiler that you would like to
use this plugin with.  If not, you will probably discover some gross loader
errors when trying to use a plugin built from a different version of clang.


References
----------
* https://clang.llvm.org/docs/ClangPlugins.html
* \<clang-source\>/examples/
