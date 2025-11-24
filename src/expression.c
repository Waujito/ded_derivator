#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

#include "expression.h"

int expression_ctor(struct expression *expr) {
	assert (expr);

	if (tree_ctor(&expr->tree)) {
		return S_FAIL;
	}

	return S_OK;
}
int expression_dtor(struct expression *expr) {
	assert (expr);

	tree_dtor(&expr->tree);

	return S_OK;
}

static int tnode_validate(struct expression *expr, struct tree_node *node) {
	assert (expr);
	assert (node);

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		if (node->left || node->right) {
			eprintf("dick\n");
			return S_FAIL;
		}

		node->value.flags |= DERIVATOR_F_CONSTANT;
		return S_OK;
	}

	// not number nor operator
	if ((node->value.flags & DERIVATOR_F_OPERATOR) != DERIVATOR_F_OPERATOR) {
		return S_FAIL;
	}
	const struct expression_operator *op = node->value.ptr;
	//
	// if (op == &expr_operator_variable) {
	//
	// }
	// if (!node->left && !node->right) {
	// 	eprintf("dick\n");
	// 	return S_FAIL;
	// }

	int is_not_constant = 0;
	if (node->left) {
		if (tnode_validate(expr, node->left)) {
			return S_FAIL;
		}

		if (!(node->left->value.flags & DERIVATOR_F_CONSTANT)) {
			is_not_constant++;
		}
	}

	if (node->right) {
		if (tnode_validate(expr, node->right)) {
			return S_FAIL;
		}

		if (!(node->right->value.flags & DERIVATOR_F_CONSTANT)) {
			is_not_constant++;
		}
	}

	if (!is_not_constant && (node->left || node->right)) {
		node->value.flags |= DERIVATOR_F_CONSTANT;
	}

	return S_OK;
}

int expression_validate(struct expression *expr) {
	assert (expr);

	if (!expr->tree.root) {
		return S_FAIL;
	}

	int ret = tnode_validate(expr, expr->tree.root);

	return ret;
}

int expression_load(struct expression *expr, const char *filename) {
	assert (expr);
	assert (filename);

	tree_dtor(&expr->tree);

	if (tree_load(&expr->tree, filename, expression_deserializer)) {
		return S_FAIL;
	}

	if (expression_validate(expr)) {
		return S_FAIL;
	}

	return S_OK;
}

int expression_store(struct expression *expr, const char *filename) {
	assert (expr);
	assert (filename);

	if (tree_store(&expr->tree, filename, expression_serializer)) {
		return S_FAIL;
	}

	return S_OK;
}


DSError_t expression_deserializer(tree_dtype *value, const char *str) {
	assert (value);
	assert (str);

	char *endptr = NULL;
	double fnum = strtod(str, &endptr);

	if (*endptr == '\0' && *str != '\0') {
		value->fnum = fnum;
		value->flags = DERIVATOR_F_NUMBER;
		return DS_OK;
	}

	const struct expression_operator *const *derop_ptr = expression_operators;
	while (*derop_ptr != NULL) {
		if (!strcmp(str, (*derop_ptr)->name)) {
			value->ptr = (void *)(*derop_ptr);
			value->flags = DERIVATOR_F_OPERATOR;

			return DS_OK;
		}

		derop_ptr++;
	}

	return DS_INVALID_ARG;
}

DSError_t expression_serializer(tree_dtype value, FILE *out_stream) {
	if ((value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		fprintf(out_stream, "%g", value.fnum);
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
		fprintf(out_stream, "%g", node->value.fnum);
	}

	return DS_OK;
}

static const char *latex_command_header =
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

struct tree_node *expr_create_number_tnode(double fnum) {
	struct tree_node *node = tnode_ctor();

	if (!node)
		return NULL;

	node->value.fnum = fnum;
	node->value.flags = DERIVATOR_F_NUMBER | DERIVATOR_F_CONSTANT;
	node->left = NULL;
	node->right = NULL;

	return node;
}

struct tree_node *expr_create_operator_tnode(const struct expression_operator *op, 
                                              struct tree_node *left, 
                                              struct tree_node *right) {
	struct tree_node *node = tnode_ctor();
	if (!node)
		return NULL;

	node->value.ptr = (void*)op;
	node->value.flags = DERIVATOR_F_OPERATOR;
	node->left = left;
	node->right = right;

	if (left || right) {
		int is_not_constant = 0;
		if (left && (left->value.flags & DERIVATOR_F_CONSTANT) == 0)
			is_not_constant++;
		if (right && (right->value.flags & DERIVATOR_F_CONSTANT) == 0)
			is_not_constant++;

		if (!is_not_constant) {
			node->value.flags |= DERIVATOR_F_CONSTANT;
		}
	}

	return node;
}

struct tree_node *expr_copy_tnode(struct expression *expr, struct tree_node *original) {
	assert (original);

	struct tree_node *copy = tnode_ctor();
	if (!copy)
		return NULL;

	copy->value = original->value;

	if (original->left) {
		copy->left = expr_copy_tnode(expr, original->left);

		if (!copy->left) {
			tnode_recursive_dtor(copy, NULL);
			return NULL;
		}
	}

	if (original->right) {
		copy->right = expr_copy_tnode(expr, original->right);

		if (!copy->right) {
			tnode_recursive_dtor(copy, NULL);
			return NULL;
		}
	}

	return copy;
}
