# Ling

Ling, in its current state, is a really simple and limited language. It is only meant to be a base for more complex future versions of the project.

## Syntax

Ling treats programs as an array of statements, meaning there is no `main` function. In fact, in its current state, Ling does not implement functions yet. This is going to change in future versions of the language.

### Statements

Ling, in its current state, allows 6 distinct types of statements.

-   Variable declaration - Declares a variable. As of now, the only available variables' types are integers.
    Example:
    ```
    let x = 69;
    ```
-   Variable assignment - Takes an already declared variable and assigns it a value.
    Example:
    ```
    x = 420;
    ```
-   If statement - Evaluates the given condition and performs the provided statement only if the value of the condition is not equal to 0.
    Example
    ```
    if(x)
        x = -1;
    ```
-   While statement - Evaluates the given condition and performs the provided statement as long as the value of the condition is not equal to 0.
    Example:
    ```
    while(x - 10)
        x = x + 1;
    ```
-   Display statement - Since Ling doesn't currently support functions nor syscalls, it is required to have at least one keyword that can display informations on the terminal. Currently the `display` keyword is used for that case but it will be deleted once functions and syscalls are introduced to the language. The display statement lets the user display a value of provided variable.
    Example:
    ```
    display x;
    ```
-   Code block - A list of statements that will be executed sequentially but are treated as a single statement. Important for `if` and `while` statements as they only accept single statement as their body.
    Example:
    ```
    {
        x = 21 + 37;
    }
    ```

### Expressions

The Ling language defines the following expressions.

-   Literal - a constant, hard-coded value. Examples: `2137`, `67`
-   Variable - an identificator of previously declared variable. Retrieves the value stored in the variable. Examples: `x`, `counter`
-   Binary operator - if `p` and `q` are both expressions, the binary operator lets the user combine them in a desired way. The following binary operators are defined in Ling.
    -   `+` - representing addition
    -   `-` - representing subtraction
    -   `*` - representing multiplication
    -   `/` - representing division
    -   `and` - representing boolean AND
    -   `or` - representing boolean OR
    -   relational operators:
        -   `==`
        -   `!=`
        -   `>`
        -   `<`
        -   `>=`
        -   `<=`
-   Unary operator - if `p` is an expression, the unary operator lets the user manipulate it in a desired way. The following unary operators are defined in Ling.
    -   `+` - representing identity
    -   `-` - representing arithmetic negation
    -   `not` - representing boolean NOT

## Usage

In order to use the Ling compiler, it first has to be compiled itself. This can be achieved using the `cmake` system. The following terminal commands executed in the clone of this repository will compile the Ling compiler.
```
mkdir build
cd build
cmake ..
make
```

### Compiling to executable

In order to compile a Ling program, use the following terminal command
```
./ling [program]
```
Note that the program meant to be compiled is given without the `.ling` extension. For example, compilation of the `test.ling` program is done using
```
./ling test
```

### Compiling to assembly

If, for any reason, it is prefered to compile to assembly instead of the ELF64 executable, the `-s` flag can be used. For instance, to compile `test.ling` into an assembly code, one can use
```
./ling test.ling -s
```

## Example programs

Simple examples of the Ling programs are provided in the `tests` directory of this repository.