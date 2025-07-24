#ifndef   CODE_GENERATOR_H
#define   CODE_GENERATOR_H

#include    <stdio.h>
#include    "config.h"
#include    "tree.h"

BEGIN_HEADER

/*  ###############################################################  */
void	print_symbol_table	ARGS((treenode*, FILE *fp));
int		code_recur			ARGS((treenode*, int , FILE *fp, int));
/*  ###############################################################  */

END_HEADER

#endif   
