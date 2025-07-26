#include "loop_analysis.h"
#include "globals.h"

// Global variables for analysis
static int next_node_id = 0;
static int next_loop_id = 0;

// Helper function to create a dependency node
dep_node* create_dep_node(int id, char* operation, int type) {
    dep_node* node = (dep_node*)malloc(sizeof(dep_node));
    node->id = id;
    node->operation = strdup(operation);
    node->type = type;
    node->dependencies = NULL;
    node->dep_count = 0;
    node->dep_capacity = 0;
    return node;
}

// Helper function to add a dependency to a node
void add_dependency(dep_node* node, dep_node* dep) {
    if (node->dep_count >= node->dep_capacity) {
        node->dep_capacity = node->dep_capacity == 0 ? 4 : node->dep_capacity * 2;
        node->dependencies = (dep_node**)realloc(node->dependencies, 
                                               node->dep_capacity * sizeof(dep_node*));
    }
    node->dependencies[node->dep_count++] = dep;
}

// Convert a tree node to string representation
char* node_to_string(treenode* node) {
    if (!node) return strdup("NULL");
    
    char buffer[1024];
    buffer[0] = '\0';
    
    switch (node->hdr.which) {
        case LEAF_T: {
            leafnode* leaf = (leafnode*)node;
            switch (leaf->hdr.type) {
                case TN_IDENT:
                    sprintf(buffer, "%s", leaf->data.sval->str);
                    break;
                case TN_INT:
                    sprintf(buffer, "%d", leaf->data.ival);
                    break;
                default:
                    sprintf(buffer, "LEAF_%d", leaf->hdr.type);
                    break;
            }
            break;
        }
        case NODE_T:
            switch (node->hdr.type) {
                case TN_ASSIGN:
                    sprintf(buffer, "ASSIGN");
                    break;
                case TN_DEREF:
                    sprintf(buffer, "DEREF");
                    break;
                case TN_INDEX:
                    sprintf(buffer, "INDEX");
                    break;
                default:
                    sprintf(buffer, "NODE_%d", node->hdr.type);
                    break;
            }
            break;
        default:
            sprintf(buffer, "UNKNOWN_%d", node->hdr.which);
            break;
    }
    
    return strdup(buffer);
}

// Convert a condition to string representation
char* condition_to_string(treenode* condition) {
    if (!condition) return strdup("NULL");
    
    char buffer[1024];
    buffer[0] = '\0';
    
    switch (condition->hdr.which) {
        case LEAF_T: {
            leafnode* leaf = (leafnode*)condition;
            if (leaf->hdr.type == TN_IDENT) {
                sprintf(buffer, "%s", leaf->data.sval->str);
            } else {
                sprintf(buffer, "COND_LEAF_%d", leaf->hdr.type);
            }
            break;
        }
        case NODE_T:
            switch (condition->hdr.type) {
                case TN_EXPR:
                    sprintf(buffer, "EXPR_%d", condition->hdr.tok);
                    break;
                case TN_INDEX:
                    sprintf(buffer, "INDEX_COND");
                    break;
                default:
                    sprintf(buffer, "COND_NODE_%d", condition->hdr.type);
                    break;
            }
            break;
        default:
            sprintf(buffer, "COND_UNKNOWN_%d", condition->hdr.which);
            break;
    }
    
    return strdup(buffer);
}

// Analyze memory aliasing between two statements
void analyze_memory_aliasing(treenode* stmt1, treenode* stmt2, cpu_prediction*** predictions, int* prediction_count) {
    if (!stmt1 || !stmt2) return;
    
    // Extract memory addresses from statements
    char* addr1 = node_to_string(stmt1);
    char* addr2 = node_to_string(stmt2);
    
    // Create prediction condition
    char buffer[256];
    sprintf(buffer, "<%s != %s>", addr1, addr2);
    
    // Add to predictions
    if (*prediction_count == 0) {
        *predictions = (cpu_prediction**)malloc(sizeof(cpu_prediction*));
    } else {
        *predictions = (cpu_prediction**)realloc(*predictions, 
                                               (*prediction_count + 1) * sizeof(cpu_prediction*));
    }
    
    cpu_prediction* pred = (cpu_prediction*)malloc(sizeof(cpu_prediction));
    pred->condition = strdup(buffer);
    pred->type = 0; // Memory aliasing
    
    (*predictions)[*prediction_count] = pred;
    (*prediction_count)++;
    
    free(addr1);
    free(addr2);
}

// Analyze branch conditions
void analyze_branch_conditions(treenode* condition, cpu_prediction*** predictions, int* prediction_count) {
    if (!condition) return;
    
    char* cond_str = condition_to_string(condition);
    
    // Create prediction condition
    char buffer[256];
    sprintf(buffer, "<%s>", cond_str);
    
    // Add to predictions
    if (*prediction_count == 0) {
        *predictions = (cpu_prediction**)malloc(sizeof(cpu_prediction*));
    } else {
        *predictions = (cpu_prediction**)realloc(*predictions, 
                                               (*prediction_count + 1) * sizeof(cpu_prediction*));
    }
    
    cpu_prediction* pred = (cpu_prediction*)malloc(sizeof(cpu_prediction));
    pred->condition = strdup(buffer);
    pred->type = 1; // Branch condition
    
    (*predictions)[*prediction_count] = pred;
    (*prediction_count)++;
    
    free(cond_str);
}

// Build dependency graph from loop body
void build_dependency_graph(treenode* loop_body, dep_node*** nodes, int* node_count) {
    if (!loop_body) return;
    
    *nodes = NULL;
    *node_count = 0;
    
    // Simple implementation: create nodes for each statement
    // In a full implementation, you would analyze data dependencies
    
    switch (loop_body->hdr.which) {
        case NODE_T:
            switch (loop_body->hdr.type) {
                case TN_ASSIGN: {
                    char* op_str = node_to_string(loop_body);
                    dep_node* node = create_dep_node(next_node_id++, op_str, TN_ASSIGN);
                    
                    if (*node_count == 0) {
                        *nodes = (dep_node**)malloc(sizeof(dep_node*));
                    } else {
                        *nodes = (dep_node**)realloc(*nodes, (*node_count + 1) * sizeof(dep_node*));
                    }
                    (*nodes)[*node_count] = node;
                    (*node_count)++;
                    
                    free(op_str);
                    break;
                }
                case TN_IF: {
                    if_node* ifn = (if_node*)loop_body;
                    
                    // Create node for condition
                    char* cond_str = condition_to_string(ifn->cond);
                    dep_node* cond_node = create_dep_node(next_node_id++, cond_str, TN_IF);
                    
                    if (*node_count == 0) {
                        *nodes = (dep_node**)malloc(sizeof(dep_node*));
                    } else {
                        *nodes = (dep_node**)realloc(*nodes, (*node_count + 1) * sizeof(dep_node*));
                    }
                    (*nodes)[*node_count] = cond_node;
                    (*node_count)++;
                    
                    // Recursively process then and else branches
                    build_dependency_graph(ifn->then_n, nodes, node_count);
                    if (ifn->else_n) {
                        build_dependency_graph(ifn->else_n, nodes, node_count);
                    }
                    
                    free(cond_str);
                    break;
                }
                case TN_STEMNT_LIST: {
                    // Process left and right statements
                    build_dependency_graph(loop_body->lnode, nodes, node_count);
                    build_dependency_graph(loop_body->rnode, nodes, node_count);
                    break;
                }
                case TN_BLOCK: {
                    // Process block contents
                    build_dependency_graph(loop_body->lnode, nodes, node_count);
                    build_dependency_graph(loop_body->rnode, nodes, node_count);
                    break;
                }
                default:
                    // Recursively process children
                    build_dependency_graph(loop_body->lnode, nodes, node_count);
                    build_dependency_graph(loop_body->rnode, nodes, node_count);
                    break;
            }
            break;
        default:
            break;
    }
}

// Extract CPU predictions from loop body
void extract_cpu_predictions(treenode* loop_body, cpu_prediction*** predictions, int* prediction_count) {
    if (!loop_body) return;
    
    *predictions = NULL;
    *prediction_count = 0;
    
    switch (loop_body->hdr.which) {
        case NODE_T:
            switch (loop_body->hdr.type) {
                case TN_IF: {
                    if_node* ifn = (if_node*)loop_body;
                    
                    // Extract branch condition
                    analyze_branch_conditions(ifn->cond, predictions, prediction_count);
                    
                    // Recursively process branches
                    extract_cpu_predictions(ifn->then_n, predictions, prediction_count);
                    if (ifn->else_n) {
                        extract_cpu_predictions(ifn->else_n, predictions, prediction_count);
                    }
                    break;
                }
                case TN_ASSIGN: {
                    // For assignments, we would analyze memory aliasing with other statements
                    // This is a simplified version
                    break;
                }
                case TN_STEMNT_LIST: {
                    // Process left and right statements
                    extract_cpu_predictions(loop_body->lnode, predictions, prediction_count);
                    extract_cpu_predictions(loop_body->rnode, predictions, prediction_count);
                    break;
                }
                case TN_BLOCK: {
                    // Process block contents
                    extract_cpu_predictions(loop_body->lnode, predictions, prediction_count);
                    extract_cpu_predictions(loop_body->rnode, predictions, prediction_count);
                    break;
                }
                default:
                    // Recursively process children
                    extract_cpu_predictions(loop_body->lnode, predictions, prediction_count);
                    extract_cpu_predictions(loop_body->rnode, predictions, prediction_count);
                    break;
            }
            break;
        default:
            break;
    }
}

// Calculate execution time based on dependency graph and predictions
int calculate_execution_time(dep_node** nodes, int node_count, int* prediction_values) {
    if (node_count == 0) return 0;
    
    // Simple implementation: count nodes
    // In a full implementation, you would use topological sort and longest path
    return node_count;
}

// Find longest path in dependency graph (simplified)
int find_longest_path(dep_node** nodes, int node_count) {
    if (node_count == 0) return 0;
    
    // Simple implementation: return node count
    // In a full implementation, you would use dynamic programming
    return node_count;
}

// Print analysis results
void print_analysis_results(loop_analysis* analysis, FILE* output_file) {
    fprintf(output_file, "\n=== LOOP ANALYSIS %d ===\n", analysis->loop_id);
    fprintf(output_file, "Loop: %s\n", analysis->loop_info);
    
    fprintf(output_file, "\nCPU Predictions:\n");
    for (int i = 0; i < analysis->prediction_count; i++) {
        cpu_prediction* pred = analysis->predictions[i];
        fprintf(output_file, "  %s (type: %s)\n", 
                pred->condition, 
                pred->type == 0 ? "memory aliasing" : "branch condition");
    }
    
    fprintf(output_file, "\nDependency Graph Nodes:\n");
    for (int i = 0; i < analysis->node_count; i++) {
        dep_node* node = analysis->nodes[i];
        fprintf(output_file, "  Node %d: %s\n", node->id, node->operation);
    }
    
    fprintf(output_file, "\nExecution Times:\n");
    fprintf(output_file, "  Original: %d cycles\n", analysis->original_time);
    fprintf(output_file, "  Minimum: %d cycles\n", analysis->min_time);
    fprintf(output_file, "  Maximum: %d cycles\n", analysis->max_time);
    
    fprintf(output_file, "\nPossible Parallel Executions:\n");
    
    // Generate all possible prediction combinations
    int num_combinations = 1 << analysis->prediction_count;
    for (int i = 0; i < num_combinations; i++) {
        fprintf(output_file, "  Combination %d: ", i);
        
        int* prediction_values = (int*)malloc(analysis->prediction_count * sizeof(int));
        for (int j = 0; j < analysis->prediction_count; j++) {
            prediction_values[j] = (i >> j) & 1;
            fprintf(output_file, "%s=%s ", 
                    analysis->predictions[j]->condition,
                    prediction_values[j] ? "true" : "false");
        }
        
        int exec_time = calculate_execution_time(analysis->nodes, analysis->node_count, prediction_values);
        fprintf(output_file, "-> %d cycles\n", exec_time);
        
        free(prediction_values);
    }
    
    fprintf(output_file, "\n");
}

// Analyze a single loop
void analyze_loop(for_node* loop, int loop_id, FILE* output_file) {
    loop_analysis analysis;
    analysis.loop_id = loop_id;
    analysis.loop_info = strdup("for loop"); // Simplified
    analysis.nodes = NULL;
    analysis.node_count = 0;
    analysis.predictions = NULL;
    analysis.prediction_count = 0;
    
    // Build dependency graph
    build_dependency_graph(loop->stemnt, &analysis.nodes, &analysis.node_count);
    
    // Extract CPU predictions
    extract_cpu_predictions(loop->stemnt, &analysis.predictions, &analysis.prediction_count);
    
    // Calculate execution times
    analysis.original_time = find_longest_path(analysis.nodes, analysis.node_count);
    
    // Calculate min/max times based on predictions
    int num_combinations = 1 << analysis.prediction_count;
    analysis.min_time = analysis.original_time;
    analysis.max_time = analysis.original_time;
    
    for (int i = 0; i < num_combinations; i++) {
        int* prediction_values = (int*)malloc(analysis.prediction_count * sizeof(int));
        for (int j = 0; j < analysis.prediction_count; j++) {
            prediction_values[j] = (i >> j) & 1;
        }
        
        int exec_time = calculate_execution_time(analysis.nodes, analysis.node_count, prediction_values);
        if (exec_time < analysis.min_time) analysis.min_time = exec_time;
        if (exec_time > analysis.max_time) analysis.max_time = exec_time;
        
        free(prediction_values);
    }
    
    // Print results
    print_analysis_results(&analysis, output_file);
    
    // Cleanup
    for (int i = 0; i < analysis.node_count; i++) {
        free(analysis.nodes[i]->operation);
        free(analysis.nodes[i]->dependencies);
        free(analysis.nodes[i]);
    }
    free(analysis.nodes);
    
    for (int i = 0; i < analysis.prediction_count; i++) {
        free(analysis.predictions[i]->condition);
        free(analysis.predictions[i]);
    }
    free(analysis.predictions);
    
    free(analysis.loop_info);
}

// Main function to analyze the entire program
void analyze_program(treenode* root, FILE* output_file) {
    if (!root) return;
    fprintf(output_file, "=== PROGRAM ANALYSIS ===\n");
    analyze_program_recursive(root, output_file);
}

// Recursive helper function to find and analyze loops
static void analyze_program_recursive(treenode* root, FILE* output_file) {
    if (!root) return;
    
    switch (root->hdr.which) {
        case FOR_T: {
            for_node* forn = (for_node*)root;
            if (forn->hdr.type == TN_FOR) {
                analyze_loop(forn, next_loop_id++, output_file);
            }
            break;
        }
        case NODE_T:
            // Recursively process children
            analyze_program_recursive(root->lnode, output_file);
            analyze_program_recursive(root->rnode, output_file);
            break;
        default:
            break;
    }
} 