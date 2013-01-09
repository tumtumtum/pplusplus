P++
=========

P++ is a language based on PL/0 but borrows heavily from C, Haskell and other languages.

I wrote the compiler and interpreter for P++ at University back in 2000 as part of my second year programming languages paper.  It was only supposed to be a 200 line assignment but I enjoyed learning about compilers so much that I added many advanced (advanced compared to PL/0) features such as:

	* Strings
	* Reference counted GC
	* Lambda Expressions
	* Integrated large integer & float support
	* Inline assembly/bytecode
	* A modest library of core functions
	
As much as possible, most of the features and functions are written in the language itself. Big Integer support, math functions such as cos, sin, random and sorting functions were mostly an excercise in learning to implement the functions that almost always come for free with a language. There was a lot of reward in being able to implement complex libraries in a language I wrote myself.

Whilst going through very old disks, I discovered the code and have uploaded it here for posterity and in the hopes that it helps students and teachers learn and teach compiler construction.

Sample P++ source/programs are in the examples directory. Refer to the [P++ User Manual](https://github.com/tumtumtum/pplusplus/blob/master/docs/Users-Manual.pdf?raw=true) for a detailed language spec.

```
/* Hello World */
using "stream.p++";

function main() : integer
{
	println("Hello World");
};
```

```
/* Large integer factorial */
using "lang.p++";
using "string.p++";
using "bignumbers.p++";

function big_factorial(x : bignumber) : bignumber
{	
	return (x > 1) ? x * big_factorial(x - 1) : 1#;
};
```

```
Compiler/Interpreter usage:

> p++ helloworld.p++
Hello World!
```

