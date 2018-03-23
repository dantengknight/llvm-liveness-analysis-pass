# Include
New **pass** source code that's used to compute the **liveness** set of each basic block of a CFG(control flow graph) 

Script that I use to do experiment and analyze our passes

Example C file I experiment on.

Platform: mac OS, llvm version: 7.0.0 svn

# Abstract:
The goal of this project is to learn how program analysis and optimizations work in real-world industrial compilers, which in our case, the LLVM compiler. Then program analysis and or optimizations will be performed on IR by one or multiple passes. Finally, the back end translates the result IR into target machine code. 
I built a data-flow analysis pass in the middle end, that do liveness analysis.

# Liveness
Results of liveness analysis can be used for register allocation, SSA
construction, useless-store elimination etc.

A variable v is **live** at point p iff there is a path from p to a use of v
along which v is not redefined.
Stated simply, a variable is live if it holds a value that may be needed in the
future.
