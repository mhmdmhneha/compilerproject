#include    <string.h>
#include    <malloc.h>

#include    "prnttree.h"
#include	"code_generator.h"
#include    "token.h"
#include    "gram_parser.cpp.h"
#include    "symtab.h"
#include    "dsm_extension.h"
#include    "globals.h"

typedef struct symbol_table {
	char* name;
	char* typeName;
	int address;
	int size;
	int typeSizeof;
	int numDim;
	int* d;
	int offset;
	struct symbol_table* next;
	/*int nd = 1;
	char* static_link;
	char* v;
	*/
	
} Symbol_table;

typedef struct struct_list {
	char* name;
	int size;
	int numFields;
	Symbol_table* fieldslist;

	struct struct_list* next;
}Struct_list;

/*  ###############################################################  */
static Symbol_table* symboltable = NULL;
static Struct_list* structlist = NULL;
static char* currStructName;
static int isStruct = 0;
static char* currArrayName;
static char* lastVariableType;
static char* lastVariableName;
static char* prevVariableType;
enum loop_type {
	FOR_LOOP = 0,
	WHILE_LOOP = 1,
	DOWHILE_LOOP = 2
};
static int loopType[10] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
static int loopLabel[10] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
static int loopIndex = 0;

static int global_address = 5;
static int while_counter = 0;
static int dowhile_counter = 0;
static int for_counter = 0;
static int ifelse_counter = 0;
static int if_counter = 0;
static int cond_counter = 0;
static int switch_counter = 0;


/*  ###############################################################  */
void add_variable(
	Symbol_table **head, 
	char* name,
	char* typeName,
	int address,
	int size,
	int typeSizeof,
	int numDim,
	int* d,
	int offset) 
{
	/*Create variable*/
	Symbol_table *var,*temp;
	var = (Symbol_table*)malloc(sizeof(Symbol_table));
	if (var == NULL) {
		printf("Memory allocation error (in symtab)!");
		exit(1);
	}
	var->address = address;
	var->name = name;
	var->typeName = typeName;
	var->size = size;
	var->typeSizeof = typeSizeof;
	var->numDim = numDim;
	var->d = d;
	var->offset = offset;
	var->next = NULL;

	/*add the variable to symbol table*/
	if (*head == NULL){
		*head = var;
		return;
	}	
	temp = *head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = var;
	return;
}

Symbol_table *getVarByName(Symbol_table* head, char* name) {
	Symbol_table* tmp = head;
	while (tmp != NULL) {
		if (strcmp(tmp->name, name) == 0) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

void print_symtab(Symbol_table* head) {
	int i;
	if (head == NULL)
		return;
	printf("---------------------------\n");
	printf("name		= %s\n", head->name);
	printf("typeName	= %s\n", head->typeName);
	printf("address		= %d\n", head->address);
	printf("size		= %d\n", head->size);
	printf("typeSizeof	= %d\n", head->typeSizeof);
	printf("numDim		= %d\n", head->numDim);
	printf("offset		= %d\n", head->offset);
	for (i = 0; i < head->numDim; i++) {
		printf("d[%d]		= %d\n",i, head->d[i]);
	}
	
	print_symtab(head->next);
}

void free_symboltab(Symbol_table* head) {
	if (head == NULL)
		return;
	free_symboltab(head->next);
	free(head);
}
/*  ###############################################################  */
void add_struct(
	Struct_list **head,
	char* name,
	int size,
	int numFields,
	Symbol_table* fieldslist)
{
	/*Create variable*/
	Struct_list *str, *temp;
	str = (Struct_list*)malloc(sizeof(Struct_list));
	if (str == NULL) {
		printf("Memory allocation error (in struct)!");
		exit(1);
	}
	str->size = size;
	str->name = name;
	str->numFields = numFields;
	str->fieldslist = fieldslist;
	str->next = NULL;

	/*add the variable to symbol table*/
	if (*head == NULL) {
		*head = str;
		return;
	}
	temp = *head;
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = str;
	return;
}

Struct_list *getStructByName(Struct_list* head, char* name) {
	Struct_list* tmp = head;
	while (tmp != NULL) {
		if (strcmp(tmp->name, name) == 0) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

void print_structlist(Struct_list* head) {
	int i;
	if (head == NULL)
		return;
	printf("****************************\n");
	printf("name		= %s\n", head->name);
	printf("size		= %d\n", head->size);
	printf("numFields	= %d\n", head->numFields);
	printf("fieldslist	=\n");
	print_symtab(head->fieldslist);

	print_structlist(head->next);
}

void free_structlist(Struct_list* head) {
	if (head == NULL)
		return;
	free_structlist(head->next);
	free_symboltab(head->fieldslist);
	free(head);
}
/*  ###############################################################  */


int  code_recur(treenode *root, int level, FILE *fp, int is_Value)
{
	if_node  *ifn;
	for_node *forn;
	leafnode *leaf;
	int curr_counter = -1;

    if (!root)
        return 0;

    switch (root->hdr.which){
		case LEAF_T:
			leaf = (leafnode *) root;
			switch (leaf->hdr.type) {
				case TN_LABEL:
					break;

				case TN_IDENT:
					if (strcmp(leaf->data.sval->str, "true") == 0)
						printf("ldc 1\n");
					else if (strcmp(leaf->data.sval->str, "false") == 0)
						printf("ldc 0\n");
					else { /* variable case */
						if(!getVarByName(symboltable, leaf->data.sval->str))
							printf("ldc %s\n", leaf->data.sval->str);
						else {
							printf("ldc %d\n", getVarByName(symboltable, leaf->data.sval->str)->address);
							prevVariableType = lastVariableType;
							lastVariableType = getVarByName(symboltable, leaf->data.sval->str)->typeName;
							lastVariableName = getVarByName(symboltable, leaf->data.sval->str)->name;
						}

						if (is_Value == 1)
							printf("ind\n");
					}
					break;

				case TN_COMMENT:
					break;

				case TN_ELLIPSIS:
					break;

				case TN_STRING:
					break;

				case TN_TYPE:
					break;

				case TN_INT:
					printf("ldc %d\n", leaf->data.ival);
					break;

				case TN_REAL:
					printf("ldc %.2f\n", leaf->data.dval);
					break;

				default:
					printf("Error: Unknown leaf value!\n");
					exit(1);
			}
			break;

		case IF_T:
			ifn = (if_node *) root;
			switch (ifn->hdr.type) {

			case TN_IF:
				if (ifn->else_n == NULL) {
					/* if case */
					curr_counter = if_counter++;
					code_recur(ifn->cond, level, fp, 1);
					printf("fjp if_end%d\n", curr_counter);
					code_recur(ifn->then_n, level + 1, fp, 1);
					printf("if_end%d:\n", curr_counter);
				}
				else {
					/* if - else case*/ 
					curr_counter = ifelse_counter++;
					code_recur(ifn->cond, level, fp, 1);
					printf("fjp ifelse_else%d\n", curr_counter);
					code_recur(ifn->then_n, level + 1, fp, 1);
					printf("ujp ifelse_end%d\n", curr_counter);
					printf("ifelse_else%d:\n", curr_counter);
					code_recur(ifn->else_n, level + 1, fp, 1);
					printf("ifelse_end%d:\n", curr_counter);
				}
				return 0;

			case TN_COND_EXPR:
				/* (cond)?(exp):(exp); */
				curr_counter = cond_counter++;
				code_recur(ifn->cond, level, fp, 1);
				printf("fjp cond_else%d\n", curr_counter);
				code_recur(ifn->then_n, level + 1, fp, 1);
				printf("ujp condLabel_end%d\n", curr_counter);
				printf("cond_else%d:\n", curr_counter);
				code_recur(ifn->else_n, level + 1, fp, 1);
				printf("cond_end%d:\n", curr_counter);
				break;

			default:
				printf("Error: Unknown type of if node!\n");
				exit(1);
			}
			break;

		case FOR_T:
			forn = (for_node *) root;
			switch (forn->hdr.type) {

			case TN_FUNC_DEF:
				code_recur(forn->init, level, fp, 1);
				code_recur(forn->test, level, fp, 1);
				code_recur(forn->incr, level, fp, 1);
				code_recur(forn->stemnt, level, fp, 1);
				break;

			case TN_FOR:
				curr_counter = for_counter++;
				code_recur(forn->init, level, fp, 1);
				printf("for_loop%d:\n", curr_counter);
				code_recur(forn->test, level, fp, 1);
				printf("fjp for_end%d\n", curr_counter);

				loopLabel[loopIndex] = curr_counter;
				loopType[loopIndex++] = FOR_LOOP;
				code_recur(forn->stemnt, level + 1, fp, 1);
				loopType[--loopIndex] = -1;
				loopLabel[loopIndex] = -1;

				code_recur(forn->incr, level + 1, fp, 1);
				printf("ujp for_loop%d\n", curr_counter);
				printf("for_end%d:\n", curr_counter);
				return 0;

			default:
				printf("Error: Unknown type of for node!\n");
				exit(1);
			}
			break;

		case NODE_T:
			switch (root->hdr.type) {
				case TN_PARBLOCK:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;
				
				case TN_PARBLOCK_EMPTY:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;
					
				case TN_TRANS_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_FUNC_DECL:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_FUNC_CALL:
					if (strcmp(((leafnode*)root->lnode)->data.sval->str, "printf") == 0) {
						code_recur(root->rnode->rnode, level, fp, 1);
						printf("print\n");
					}
					else {
						code_recur(root->lnode, level, fp, 1);
						code_recur(root->rnode, level, fp, 1);
					}
					break;

				case TN_BLOCK:
					code_recur(root->lnode, level+1, fp, 1);
					code_recur(root->rnode, level+1, fp, 1);
					return 0;

				case TN_ARRAY_DECL:
					if (root->lnode->hdr.type == TN_ARRAY_DECL)
						code_recur(root->lnode, level, fp, 1);
					
					Symbol_table* st;
					if (isStruct) {
						Struct_list* sl = getStructByName(structlist, currStructName);
						if (sl == NULL) {
							printf("Error10!\n");
							exit(1);
						}
						st = getVarByName(sl->fieldslist, currArrayName);
					}
					else
						st = getVarByName(symboltable, currArrayName);

					if (st == NULL) {
						printf("Error in array declaration!\n");
						exit(1);
					}
					int i,flag = 1;
					for (i = 0; i < st->numDim && flag; i++) {
						if (st->d[i] == 0) {
							st->d[i] = ((leafnode*)root->rnode)->data.ival;
							flag = 0;
						}
					}
					st->size *= ((leafnode*)root->rnode)->data.ival;
					break;

				case TN_EXPR_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_NAME_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_ENUM_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_FIELD_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_PARAM_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_IDENT_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_TYPE_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_COMP_DECL:
				case TN_DECL:
					if (root->hdr.type != TN_COMP_DECL && root->lnode->lnode->hdr.type == TN_OBJ_DEF) {
						/*definition of struct*/
						leaf = (leafnode*)root->lnode->lnode->lnode;
						currStructName = nmestr(leaf->data.sval);
						isStruct = 1;
						add_struct(&structlist, currStructName,0,0,NULL);

						/* add the fields here */
						code_recur(root->lnode->lnode->rnode, level, fp, 1);
						isStruct = 0;
					}
					else {
						leaf = (leafnode*)root->lnode->lnode;
						int typeSizeof = -1;
						int size = -1;
						char *varName, *typeName;

						if (root->lnode->lnode->hdr.type == TN_OBJ_REF) {
							/*struct variable reference case*/
							leaf = (leafnode*)root->lnode->lnode->lnode;
							typeName = nmestr(leaf->data.sval);
							Struct_list* s = getStructByName(structlist, typeName);
							if(s!= NULL)
								typeSizeof = s->size;
						}
						else if (
							leaf->hdr.tok == INT ||
							leaf->hdr.tok == FLOAT ||
							leaf->hdr.tok == CHAR ||
							leaf->hdr.tok == DOUBLE)
						{
							typeName = toksym(leaf->hdr.tok, 0);
							typeSizeof = 1;
						}
						/* ################################# */

						if (((leafnode*)root->rnode)->hdr.type == TN_IDENT) {
							/*primitive variable case*/
							leaf = (leafnode*)root->rnode;
							varName = nmestr(leaf->data.sval);
							size = typeSizeof;
							Symbol_table* head;
							if (root->hdr.type != TN_COMP_DECL) {
								/* add to symbol table list */
								add_variable(&symboltable, varName, typeName, global_address, size, typeSizeof, 0, NULL, 0);
								global_address += size;
							}
							else {
								/* add to struct list */
								Struct_list* sl = getStructByName(structlist, currStructName);
								if (sl == NULL) {
									printf("Error11!\n");
									exit(1);
								}
								

								add_variable(&sl->fieldslist, varName, typeName, -1, size, typeSizeof, 0, NULL, sl->size);
								sl->size += size;
								sl->numFields++;
								
							}
						}
						else if (root->rnode->hdr.type == TN_ARRAY_DECL) {
							/*array variable case*/
							treenode* tmp = root->rnode;
							int arrSize = 0;
							while (tmp->hdr.type == TN_ARRAY_DECL) {
								arrSize++;
								tmp = tmp->lnode;
							}
							currArrayName = nmestr(((leafnode*)tmp)->data.sval);
							int *dim = (int*)malloc(arrSize * sizeof(int));
							if (dim == NULL) {
								printf("Array allocation error!\n");
								exit(1);
							}
							
							for (int i = 0; i < arrSize; i++) {
								dim[i] = 0;
							}

							if (root->hdr.type != TN_COMP_DECL) {
								add_variable(&symboltable, currArrayName, typeName, global_address, typeSizeof, typeSizeof, arrSize, dim, 0);
								code_recur(root->rnode, level, fp, 1);
								Symbol_table* st = getVarByName(symboltable, currArrayName);
								if (st == NULL) {
									printf("Error1!\n");
									exit(1);
								}
								global_address += st->size;
							}
							else {
								Struct_list* sl = getStructByName(structlist, currStructName);
								if (sl == NULL) {
									printf("Error12!\n");
									exit(1);
								}
								add_variable(&sl->fieldslist, currArrayName, typeName, -1, typeSizeof, typeSizeof, arrSize, dim, sl->size);
								code_recur(root->rnode, level, fp, 1);

								Symbol_table* st = getVarByName(sl->fieldslist, currArrayName);
								if (st == NULL) {
									printf("Error2!\n");
									exit(1);
								}
								sl->size += st->size;
								sl->numFields++;
							}

						}
						else if (root->rnode->lnode->hdr.type == TN_PNTR) {
							/*pointer variable case*/
							leaf = (leafnode*)root->rnode->rnode;
							varName = nmestr(leaf->data.sval);
							
							if (root->hdr.type != TN_COMP_DECL) {
								add_variable(&symboltable, varName, typeName, global_address, 1, 1, 0, NULL, 0);
								global_address += 1;
							}
							else {
								Struct_list* sl = getStructByName(structlist, currStructName);
								if (sl == NULL) {
									printf("Error13!\n");
									exit(1);
								}
								add_variable(&sl->fieldslist, varName, typeName, -1, 1, 1, 0, NULL, sl->size);
								sl->size += 1;
								sl->numFields++;
							}
						}
						else {
							printf("Error: unknown variable declaration type!\n");
							exit(1);
						}

					}
					break;

				case TN_DECL_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_DECLS:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_STEMNT_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_STEMNT:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_BIT_FIELD:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_PNTR:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					printf("ind\n");
					break;

				case TN_TYPE_NME:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_INIT_LIST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_INIT_BLK:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_OBJ_DEF:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level+1, fp, 1);
					break;

				case TN_OBJ_REF:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_CAST:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				case TN_JUMP:
					if ((root->hdr.tok == RETURN) || (root->hdr.tok == GOTO))
					{
						code_recur(root->lnode, level, fp, 1);
					}
					if(root->hdr.tok == BREAK) {
						if (loopIndex < 1 || loopIndex > 10) {
							printf("Error22!");
							exit(1);
						}
						else if (loopType[loopIndex - 1] == DOWHILE_LOOP) {
							printf("ujp dowhile_end%d\n", loopLabel[loopIndex - 1]);
						}
						else if (loopType[loopIndex - 1] == WHILE_LOOP) {
							printf("ujp while_end%d\n", loopLabel[loopIndex - 1]);
						}
						else if (loopType[loopIndex - 1] == FOR_LOOP) {
							printf("ujp for_end%d\n", loopLabel[loopIndex - 1]);
						}
						else {
							printf("Error23!");
							exit(1);
						}
					}
					break;

				case TN_SWITCH:
					curr_counter = switch_counter++;
					code_recur(root->lnode, level, fp, 1);
					printf("neg\n"); 
					printf("ixj switch_L%d\n", curr_counter);
					code_recur(root->rnode, level + 1, fp, 1);
					printf("switch_L%d:\n", curr_counter);
					break;

				case TN_INDEX: 
					/* call for array */
					0;
					treenode* tmp = root;
					while (tmp->lnode->hdr.type == TN_INDEX) {
						tmp = tmp->lnode;
					}
					leaf = (leafnode*)tmp->lnode;
					code_recur((treenode*)leaf, level, fp, 0);

					Struct_list* structVar = getStructByName(structlist, prevVariableType);
					Symbol_table* arrayData;
					if (structVar == NULL) {
						/*The array is not struct field*/
						arrayData = getVarByName(symboltable, lastVariableName);
					}
					else {
						arrayData = getVarByName(structVar->fieldslist, lastVariableName);
					}
					
					if (arrayData == NULL) {
						printf("Error5!\n");
						exit(1);
					}

					
					
					
					for (int j = 0; j < (arrayData->numDim - 1); j++) { 
						int di = 1;
						for (int i = j + 1; i < arrayData->numDim; i++) {
							di = di * arrayData->d[i];
						}
						tmp = root;
						for (int l = 0; l < (arrayData->numDim - 1 - j); l++) {
							tmp = tmp->lnode;
						}
						code_recur(tmp->rnode, level, fp, 1);
						printf("ixa %d\n", di*arrayData->typeSizeof);
					}
					code_recur(root->rnode, level, fp, 1);
					printf("ixa %d\n", arrayData->typeSizeof);
					printf("dec 0\n");
					if (is_Value)
						printf("ind\n");
					break;

				case TN_DEREF:
					code_recur(root->rnode, level, fp, is_Value);
					printf("ind\n");
					break;

				case TN_SELECT:
					code_recur(root->lnode, level, fp, 0);
					char* FieldName = ((leafnode*)root->rnode)->data.sval->str;
					Struct_list* sl = getStructByName(structlist, lastVariableType);
					if (sl == NULL) {
						printf("Error8!\n");
						exit(1);
					}
					Symbol_table* var = getVarByName(sl->fieldslist, FieldName);
					if (var == NULL) {
						printf("Error!!\n");
						exit(1);
					}
					if (root->hdr.tok == ARROW)
						printf("ind\n");
					printf("inc %d\n", var->offset);
					prevVariableType = lastVariableType;
					lastVariableType = var->typeName;
					lastVariableName = var->name;
					if (is_Value)
						printf("ind\n");
					break;

				case TN_ASSIGN:
					code_recur(root->lnode, level, fp, 0);/* address */
					if(root->hdr.tok != EQ)
						code_recur(root->lnode, level, fp, 1);/* value */
					code_recur(root->rnode, level, fp, 1);/* value */
					if (root->hdr.tok == PLUS_EQ)
						printf("add\n");
					if (root->hdr.tok == MINUS_EQ)
						printf("sub\n");
					if (root->hdr.tok == STAR_EQ)
						printf("mul\n");
					if (root->hdr.tok == DIV_EQ)
						printf("div\n");

					printf("sto\n");
					break;

				case TN_EXPR:
					switch (root->hdr.tok) {
					  case CASE:
						code_recur(root->lnode, level, fp, 1);
						code_recur(root->rnode, level, fp, 1);
						break;

					  case SIZEOF:
						code_recur(root->lnode, level, fp, 1);
						code_recur(root->rnode, level, fp, 1);
						break;

					  case INCR:
						  code_recur(root->lnode, level, fp, 1);
						  /* ######################### */
						  /* update the variable */
						  if (root->lnode == NULL)
							  code_recur(root->rnode, level, fp, 0);
						  else
							  code_recur(root->lnode, level, fp, 0);

						  if (root->lnode == NULL)
							  code_recur(root->rnode, level, fp, 1);
						  else
							  code_recur(root->lnode, level, fp, 1);

						  printf("inc 1\n");
						  printf("sto\n");

						  /* ################################### */
						  code_recur(root->rnode, level, fp, 1);
						  break;

					  case DECR:
						code_recur(root->lnode, level, fp, 1);
						/* ################################### */
						/* update the variable */
						if(root->lnode == NULL)
							code_recur(root->rnode, level, fp, 0);
						else
							code_recur(root->lnode, level, fp, 0);

						if (root->lnode == NULL)
							code_recur(root->rnode, level, fp, 1);
						else
							code_recur(root->lnode, level, fp, 1);

						printf("dec 1\n");
						printf("sto\n");

						/* ################################### */
						code_recur(root->rnode, level, fp, 1);
						break;

					  case PLUS:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("add\n");
						  break;

					  case MINUS:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  if (root->lnode == NULL)
							printf("neg\n");
						  else
							printf("sub\n");
						  break;

					  case DIV:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("div\n");
						  break;

					  case STAR:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("mul\n");
						  break;

					  case AND:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("and\n");
						  break;

					  case OR:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("or\n");
						  break;
						
					  case NOT:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("not\n");
						  break;

					  case GRTR:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("grt\n");
						  break;

					  case LESS:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("les\n");
						  break;
						  
					  case EQUAL:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("equ\n");
						  break;

					  case NOT_EQ:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("neq\n");
						  break;

					  case LESS_EQ:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("leq\n");
						  break;

					  case GRTR_EQ:
						  code_recur(root->lnode, level, fp, 1);
						  code_recur(root->rnode, level, fp, 1);
						  printf("geq\n");
						  break;

					  default:
						printf("Error: another token %d ", root->hdr.tok);
						fputs(toksym(root->hdr.tok, 1), fp);
						printf("\n");

						code_recur(root->lnode, level, fp, 1);
						fputs(toksym(root->hdr.tok,1), fp);
						code_recur(root->rnode, level, fp, 1);
						break;
					}
					break;

				case TN_WHILE:
					curr_counter = while_counter++;
					printf("while_loop%d:\n", curr_counter);
					code_recur(root->lnode, level, fp, 1);
					printf("fjp while_end%d\n", curr_counter);

					loopLabel[loopIndex] = curr_counter;
					loopType[loopIndex++] = WHILE_LOOP;
					code_recur(root->rnode, level + 1, fp, 1);
					loopType[--loopIndex] = -1;
					loopLabel[loopIndex] = -1;

					printf("ujp while_loop%d\n", curr_counter);
					printf("while_end%d:\n", curr_counter);
					return 0;

				case TN_DOWHILE:
					curr_counter = dowhile_counter++;
					printf("dowhile_loop%d:\n", curr_counter);

					loopLabel[loopIndex] = curr_counter;
					loopType[loopIndex++] = DOWHILE_LOOP;
					code_recur(root->rnode, level + 1, fp, 1);
					loopType[--loopIndex] = -1;
					loopLabel[loopIndex] = -1;

					code_recur(root->lnode, level, fp, 1);
					printf("fjp dowhile_end%d\n", curr_counter);
					printf("ujp dowhile_loop%d\n", curr_counter);
					printf("dowhile_end%d:\n", curr_counter);
					break;

				case TN_LABEL:
					code_recur(root->lnode, level, fp, 1);
					code_recur(root->rnode, level, fp, 1);
					break;

				default:
					fprintf(fp, "Error: Unknown type of tree node (%d)!\n", root->hdr.type);
					exit(1);
			}
			break;

		case NONE_T:
			printf("Error: Unknown node type!\n");
			exit(1);

		default:
			printf("Error9!\n");
			exit(1);		
    }

    return 1;
}

void print_symbol_table(treenode *root, FILE *fp) {
	code_recur(root, 0, fp, 1);

	printf("\nShowing Symbol Table:\n");
	print_symtab(symboltable);
	free_symboltab(symboltable);

	printf("\n\nShowing Struct list:\n");
	print_structlist(structlist);
	free_structlist(structlist);
}