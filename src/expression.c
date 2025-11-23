#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

#include "expression.h"



int expression_ctor(struct expression *expr) {
	assert (expr);

	if (tree_ctor(&expr->tree)) {
		return 1;
	}

	return 0;
}
int expression_dtor(struct expression *expr) {
	assert (expr);

	tree_dtor(&expr->tree);

	return 0;
}


int expression_load(struct expression *expr, const char *filename) {
	assert (expr);
	assert (filename);

	tree_dtor(&expr->tree);

	if (tree_load(&expr->tree, filename, expression_deserializer)) {
		return 1;
	}

	return 0;
}

int expression_store(struct expression *expr, const char *filename) {
	assert (expr);
	assert (filename);

	if (tree_store(&expr->tree, filename, expression_serializer)) {
		return 1;
	}

	return 0;
}


DSError_t expression_deserializer(tree_dtype *value, const char *str) {
	assert (value);
	assert (str);

	char *endptr = NULL;
	long snum = strtoll(str, &endptr, 10);

	if (*endptr == '\0' && *str != '\0') {
		value->snum = snum;
		value->flags = DERIVATOR_F_NUMBER;
		return DS_OK;
	}

	const struct expression_operator *derop_ptr = expression_operators;
	while (derop_ptr->name != NULL) {
		if (!strcmp(str, derop_ptr->name)) {
			value->ptr = (void *)derop_ptr;
			value->flags = DERIVATOR_F_OPERATOR;

			return DS_OK;
		}

		derop_ptr++;
	}

	return DS_INVALID_ARG;
}

DSError_t expression_serializer(tree_dtype value, FILE *out_stream) {
	if ((value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		fprintf(out_stream, "%ld", value.snum);
		return DS_OK;
	}

	if ((value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		fprintf(out_stream, "%s", ((struct expression_operator *)value.ptr)->name);
		return DS_OK;
	}

	return DS_INVALID_ARG;
}

static DSError_t tnode_to_latex(struct tree_node *node, FILE *out_stream) {
	assert (node);
	assert (out_stream);

	if (node->right && !node->left) {
		return DS_INVALID_ARG;
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		struct expression_operator *expr_op = node->value.ptr;
		fprintf(out_stream, "%s", expr_op->latex_name);

		if (node->left) {
			fprintf(out_stream, "{");
			tnode_to_latex(node->left, out_stream);
			fprintf(out_stream, "}");
		}

		if (node->right) {
			fprintf(out_stream, "{");
			tnode_to_latex(node->right, out_stream);
			fprintf(out_stream, "}");
		}
	} else {
		fprintf(out_stream, "%ld", node->value.snum);
	}

	return DS_OK;
}

const char *latex_command_header =
"\\newcommand{\\edplus}[2]{(#1 \\mathbin{+} #2)}\n"
"\\newcommand{\\edminus}[2]{(#1 \\mathbin{-} #2)}\n"
"\\newcommand{\\edmultiply}[2]{(#1 \\cdot #2)}\n"
"\\newcommand{\\eddivide}[2]{\\frac{#1}{#2}}\n"
"\\newcommand{\\edpower}[2]{{#1}^{#2}}\n"
"\\newcommand{\\edln}[1]{\\mathop{\\mathrm{ln}}\\left(#1\\right)}\n"
"\\newcommand{\\edx}{\\mathord{x}}\n";


DSError_t expression_to_latex(struct expression *expr, FILE *out_stream) {
	assert (expr);
	assert (out_stream);

	fprintf(out_stream, "\\begin{center}\n");
	fprintf(out_stream, "%s", latex_command_header);
	fprintf(out_stream, "\\begin{math}\n");
	DSError_t ret = tnode_to_latex(expr->tree.root, out_stream);
	fprintf(out_stream, "\n");
	fprintf(out_stream, "\\end{math}\n");
	fprintf(out_stream, "\\end{center}\n");

	return ret;
}
