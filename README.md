# mehh
A bytecode virtual machine language based on part 2 of (https://craftinginterpreters.com/) by Robert Nystrom

Dependencies:
- Tracy
- FMT 10.2.0

## Goals
- Learn how compilers/vm interpreters work
- Create a playground for low latency optimizations

## Optimizations
The plan was to complete the book with a fairly naive implementation, without much attention to performance, and optimize it at the end.
Conveniently, the final chapter of the book is exactly that. However, as it currently stands, at chapter 25, the fibonacci example `fib(35)` takes 10-15 minutes to execute, compared to the couple of seconds that it takes using the C implementation.

Because of it, I will spend some time to bring my implementation more inline with the original before continuing with the chapters.

