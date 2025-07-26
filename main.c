#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "tree.h"

// Forward declaration for the parser entry point
treenode* parse_c_file(FILE* fp);

extern void print_tree(treenode* root, FILE* fp);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file.c>\n", argv[0]);
        return 1;
    }
    FILE* input_file = fopen(argv[1], "r");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
        return 1;
    }
    // Parse the C file to build the AST
    treenode* ast_root = parse_c_file(input_file);
    fclose(input_file);
    if (!ast_root) {
        fprintf(stderr, "Error: Failed to parse input file.\n");
        return 1;
    }
    // Print the tree and run analysis (print_tree now includes analysis)
    print_tree(ast_root, stdout);
    return 0;
} 