#!/bin/bash

clang -emit-llvm -O0 -c Fibonacci.c -o Fibo.bc

llvm-dis -f Fibo.bc  # create readable assembly code

opt -load MyLib.dylib -liveness Fibo.bc # run my liveness analysis pass

opt -dot-cfg Fibo.bc
dot -Tps cfg.main.dot  # get the cfg graph to validate my pass manually
