<div align = "center" >

# Kompy

 A compiler built from scratch to explore parsing, expression evaluation, and language implementation.

 ![Status](https://img.shields.io/badge/Status-Active%20Development-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B-orange)
![Stage](https://img.shields.io/badge/Stage-Early%20Prototype-green)

</div>

---

## Overview

I am trying to build a custom compiler written in `C++`. Through this project I want to understand the core concepts of *Compiler Internals*, which is why rather than using existing frameworks, I am making everything from scratch. At the moment, the compiler supports basic expression handling and execution.

---

## Current Syntax

Variable assignment:

```txt
assume <identifier> = <expr>;
```

Return expression:

```txt
getback(<expr>);
```

Example:

```txt
assume x = 10;
assume y = x * 2;

getback(y + 5);
```

---


## Features Implemented

### Arithmetic Operations

Currently my language supports handling any kind of **Arithmetic Operations**, along with maintaining the proper `Precedence Order`. 

Supported Operations:
<div align = "center">

```bash
+       -       *       /       %
```
</div>

---

### Comparision Operations

My kompy can also handle all the possible comparision operators maintaining the precedence order.

Supported Comparisions:
<div align = "center">

```txt
==      !=      <       >       <=      >=
```
</div>

---

### Logical Operations

Currently kompy can only handle few of the present logical operations like `AND`, `OR` and `NOT`.

Supported Operations:

<div align = "center">

```txt
&&          ||          !
```
</div>

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
./kompy [filename].ko
```

Execute generated output:

```bash
./out
```

Check the returned exit value:

```bash
echo $?
```

---

## Example

File: `example.ko`

```txt
assume a = 5;
assume b = 10;

getback((a + b) * 2);
```

Run:

```bash
./kompy example.ko
./out
echo $?
```

Output:

```txt
30
```

---


## What I Plan to Add Next

The compiler is still under active development.

Upcoming additions include:

- Scope handling
- Functions
- Conditional statements
- Variables and symbol tables
- Type checking
- Intermediate Representation (IR)
- Optimization stages
- Better error reporting

---


