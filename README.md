# Compiler Loop Analysis

This directory contains the C parser and loop analysis infrastructure.

## How to Build

```
cd compiler
make
```

## How to Run Analysis

```
./main test_program.c > analysis_output.txt
cat analysis_output.txt
```

- `main.c` parses the input C file, builds the AST, and runs the loop analysis (outputting results to stdout).
- `test_program.c` is a sample C file with a loop for analysis.

## Output

The output will include:
- The printed AST
- Loop analysis results (CPU predictions, dependency graph, execution times, etc.)

## Notes
- Make sure your Makefile in this directory builds `main.c` and links all necessary sources.
- The analysis is integrated into `print_tree`. 