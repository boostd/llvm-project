.. title:: clang-tidy - readability-no-partial-argument-wrapping

readability-no-partial-argument-wrapping
========================================

This check ensures that function arguments in definitions are not partially
wrapped to multiple lines. It enforces a consistent style where arguments are
either all on the same line as the function's opening and closing parentheses,
or the first argument is on a new line after the opening parenthesis, and the
closing parenthesis is also on its own new line after the last argument.

This rule applies to function definitions, including member functions, constructors,
destructors, and lambda call operators. It does not apply to function declarations
or callsites. The check ignores constructs inside macros.

Example of **incorrect** formatting (partial wrapping):

.. code-block:: c++

    int f(int x, int y, int z,  // First parameter on same line as '('
    int w) {                    // Last parameter on different line
      // ...
    }

    void g(std::string s1,      // First parameter on same line as '('
           std::string s2, int val) { // Other parameters on new lines
      // ...
    }

    void h(
      int x, int y, int z, int w) { // Closing ')' on same line as last parameter
      // ...
    }


Examples of **correct** formatting:

.. code-block:: c++

    // All on one line
    int f1(int x, int y, int z, int w) {
      // ...
    }

    // Properly wrapped
    int f2(
        int x, int y, int z,
        int w
    ) {
      // ...
    }

    // Also properly wrapped (though typically formatted with '(' on previous line)
    int f3
    (
        int x, int y, int z,
        int w
    ) {
      // ...
    }

    // Single line of parameters, but wrapped correctly relative to parentheses
    int f4(
        int x, int y, int z, int w
    ) {
      // ...
    }


This check helps maintain a clear and consistent code style, improving readability
especially for functions with many parameters.
