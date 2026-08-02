#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "parser.h"
#include "../../include/cax.h"

char* cax_template_eval(ASTNode *ast, CAXObject *context);

#endif