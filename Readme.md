<div align = "center" >

# Kompy

 A compiler built from scratch to explore parsing, expression evaluation, and language implementation.

 ![Status](https://img.shields.io/badge/Status-Finished-brightgreen)
![Language](https://img.shields.io/badge/Language-C%2B%2B-orange)
![Stage](https://img.shields.io/badge/Stage-Functional%20Prototype-green)

</div>

---

## Overview

I am building a custom compiler written in `C++`. Through this project I want to understand the core concepts of *Compiler Internals*, which is why rather than using existing frameworks, I am making everything from scratch. The compiler now supports variables, scoping, arithmetic/comparison/logical expressions, conditionals, loops, and functions, and compiles `.ko` source files directly into a native x86-64 Linux executable.

For a full technical breakdown — grammar, AST, and code generation internals — see [`DOCUMENTATION.md`](./DOCUMENTATION.md).

---

## Current Syntax

Variable declaration:

```txt
assume <identifier> = <expr>;
```

Variable reassignment:

```txt
<identifier> = <expr>;
```

Program exit:

```txt
getback(<expr>);
```

Conditionals:

```txt
maybe (<condition>) {
    ...
} otherwise {
    ...
}
```

While loops:

```txt
while (<condition>) {
    ...
}
```

Functions:

```txt
fn <name>(<params>) {
    ...
    return(<expr>);
}
```

Example:

```txt
fn factorial(n) {
    maybe (n <= 1) {
        return(1);
    }
    return(n * factorial(n - 1));
}

getback(factorial(5));
```

---

## Features Implemented

### Variables & Scoping

Variables are declared with `assume` and can be reassigned afterward. Every `{ }` block introduces its own scope, so variables declared inside a block, loop, or function don't leak outside it.

### Arithmetic Operations

Kompy supports all standard **Arithmetic Operations**, maintaining the proper `Precedence Order`.

Supported Operations:
<div align = "center">

```bash
+       -       *       /       %
```
</div>

---

### Comparision Operations

Kompy handles all the possible comparision operators, maintaining precedence order.

Supported Comparisions:
<div align = "center">

```txt
==      !=      <       >       <=      >=
```
</div>

---

### Logical Operations

Kompy supports the logical operations `AND`, `OR`, and `NOT`.

Supported Operations:

<div align = "center">

```txt
&&          ||          !
```
</div>

---

### Conditionals

`maybe` / `otherwise` support if/else and else-if chains (`otherwise maybe`).

### Loops

`while` loops are supported, with correct scoping for variables declared inside the loop body.

### Functions

Functions are declared with `fn`, take any number of parameters, can call themselves or each other (including recursively), and return values with `return(<expr>)`.

---

## Building

Generate build files:

```bash
cmake -B build
```

Build the compiler:

```bash
cmake --build build
```

---

## Running

Compile a source file:

```bash
./build/kompy [filename].ko
```

Execute generated output:

```bash
./out
```

Check the returned exit value:

```bash
echo $?
```

You can also use the provided `Makefile`:

```bash
make build
make run FILE=[filename].ko
make clean
```

---

## Example

File: `example.ko`

```txt
fn factorial(n) {
    maybe (n <= 1) {
        return(1);
    }
    return(n * factorial(n - 1));
}

getback(factorial(5));
```

Run:

```bash
./build/kompy example.ko
./out
echo $?
```

Output:

```txt
120
```

---
