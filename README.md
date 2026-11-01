# Mini-Language Compiler Front-End

A complete compiler front-end built in C using **Flex** (Fast Lexical Analyzer) and **Bison** (GNU Parser Generator). This project implements the four core stages of compilation: lexical analysis, syntax parsing, Abstract Syntax Tree (AST) construction, and scoped semantic analysis.

---

## Features & Language Specification

* **Data Types:** `int`, `float`, `string`, `bool`
* **Arithmetic Operations:** `+`, `-`, `*`, `/` (with strict operator precedence)
* **Relational & Equality Operations:** `<`, `>`, `==`
* **Control Flow:** `if-else` conditionals (with dangling-else resolution) and `do-while` loops
* **Scoped Blocks:** Nested `{ ... }` block scopes managed via an active/inactive symbol table
* **Output Statement:** `print(expr);` and `print expr;`
* **Multi-Phase Error Reporting:**
  * **Lexical Errors:** Catches malformed floating-point numbers (`12.3.4`), invalid identifiers (`12abc`), unterminated strings/comments, and unrecognized characters (`@`).
  * **Syntax Errors:** Reports exact line numbers and unexpected tokens via `yyerror`.
  * **Semantic Errors:** Detects undeclared variables, duplicate declarations in the same scope, expression/assignment type mismatches, non-boolean loop/if conditions, and uninitialized variable usage.

---

## Project Architecture

```text
MiniCompiler/
├── lexer.l       # Flex specification for tokenization and lexical error handling
├── parser.y      # Bison grammar rules, operator precedence, and main driver
├── ast.h         # AST node structure definitions and constructor prototypes
├── ast.c         # AST node allocation and recursive tree printing
├── symtab.h      # Scoped Symbol Table data structures and prototypes
├── symtab.c      # Symbol Table management and AST-driven semantic analysis
└── test*.txt     # Suite of 10 test programs (valid and error test cases)
```

---

## Prerequisites

Ensure you have the following installed and added to your system `PATH` (via MinGW/MSYS2 on Windows or standard package managers on Linux/macOS):
* **GCC** (GNU Compiler Collection)
* **Flex**
* **Bison**

---

## How to Build and Run

### 1. Compile the Project
Open your terminal or Command Prompt in the project directory and run the following commands in order:

```bash
bison -d parser.y
flex lexer.l
gcc parser.tab.c lex.yy.c ast.c symtab.c
```
*(This generates the `a.exe` executable on Windows or `a.out` on Linux/macOS.)*

### 2. Run the Test Suite
Pass any source file as an argument to `a.exe` to run the compiler pipeline:

```bash
a.exe test1_valid_basic.txt
a.exe test2_valid_control_scope.txt
a.exe test3_lexical_errors.txt
a.exe test4_syntax_errors.txt
a.exe test5_sem_undeclared.txt
a.exe test6_sem_duplicate.txt
a.exe test7_sem_expr_mismatch.txt
a.exe test8_sem_assign_mismatch.txt
a.exe test9_sem_scope_violation.txt
a.exe test10_sem_uninitialized.txt
```

---

## Test Suite Overview

| Test File | Description | Expected Outcome |
| :--- | :--- | :--- |
| `test1_valid_basic.txt` | Variable declarations, arithmetic precedence, and `print` statements | Compilation Successful (Prints AST & Symbol Table) |
| `test2_valid_control_scope.txt` | `if-else` branches, `do-while` loops, and nested block scopes | Compilation Successful (Prints AST & Symbol Table) |
| `test3_lexical_errors.txt` | Malformed floats, invalid identifiers, illegal `@` token, unterminated string | Fails with Lexical Errors |
| `test4_syntax_errors.txt` | Missing semicolons and invalid operator placement | Fails with Syntax Errors |
| `test5_sem_undeclared.txt` | Using variables before declaration | Fails with Semantic Errors |
| `test6_sem_duplicate.txt` | Redeclaring an existing variable in the same scope | Fails with Semantic Errors |
| `test7_sem_expr_mismatch.txt` | Mixing `int` and `float` in arithmetic and non-boolean `if` conditions | Fails with Semantic Errors |
| `test8_sem_assign_mismatch.txt` | Assigning a `string` or `int` to incompatible variable types | Fails with Semantic Errors |
| `test9_sem_scope_violation.txt` | Accessing an inner-block variable from an outer scope | Fails with Semantic Errors |
| `test10_sem_uninitialized.txt` | Using a declared variable in an expression before initializing it | Fails with Semantic Errors |

---

## Sample Output (`test1_valid_basic.txt`)

**Input Source Code:**
```c
int a = 10;
int b = 20;
int sum = a + b * 2;
float pi = 3.14;
string msg = "Compiler Lab Project";
bool is_valid = (sum > 30);
print(sum);
print(msg);
```

**Compiler Output:**
```text
=== Compilation Successful (0 errors) ===

BlockNode (line 1)
  DeclNode [type: int, id: a] (line 2)
    IntLiteral [10] (line 2)
  DeclNode [type: int, id: b] (line 3)
    IntLiteral [20] (line 3)
  DeclNode [type: int, id: sum] (line 4)
    BinaryOpNode [op: +] (line 4)
      VarNode [id: a] (line 4)
      BinaryOpNode [op: *] (line 4)
        VarNode [id: b] (line 4)
        IntLiteral [2] (line 4)
  DeclNode [type: float, id: pi] (line 5)
    FloatLiteral [3.14] (line 5)
  DeclNode [type: string, id: msg] (line 6)
    StringLiteral ["Compiler Lab Project"] (line 6)
  DeclNode [type: bool, id: is_valid] (line 7)
    BinaryOpNode [op: >] (line 7)
      VarNode [id: sum] (line 7)
      IntLiteral [30] (line 7)
  PrintNode (line 8)
    VarNode [id: sum] (line 8)
  PrintNode (line 9)
    VarNode [id: msg] (line 9)

--- Symbol Table ---
Identifier      Type       Scope      Line       Initialized
---------------------------------------------------------------
is_valid        bool       0          7          Yes
msg             string     0          6          Yes
pi              float      0          5          Yes
sum             int        0          4          Yes
b               int        0          3          Yes
a               int        0          2          Yes
```

---

## Team Members & Contributions

| Name | Student ID | Module / Responsibility |
| :--- | :--- | :--- |
| **Imtiyaz Ali** | 25524202055 | Lexical Analysis & Semantic Analysis |
| **S M Junaed Ahamed Jimon** | 25524204141 | Parser & Grammar Rules (`parser.y`) |
| **Md. Shihab Hossain** | 25524204083 | Abstract Syntax Tree Construction (`ast.h`, `ast.c`) |
| **Md. Udoy Hossain Joy** | 25524204125 | Symbol Table & Semantic Analysis (`symtab.h`, `symtab.c`) |