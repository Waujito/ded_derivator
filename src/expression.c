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

	if (pvector_init(&(expr->variables), sizeof(struct expression_variable))) {
		tree_dtor(&expr->tree);
		return S_FAIL;
	}

	return S_OK;
}

int expression_dtor(struct expression *expr) {
	assert (expr);

	tree_dtor(&expr->tree);
	pvector_destroy(&expr->variables);

	return S_OK;
}

static int tnode_validate(struct expression *expr, struct tree_node *node) {
	assert (expr);
	assert (node);

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		if (node->left || node->right) {
			return S_FAIL;
		}

		if (!(node->value.flags & DERIVATOR_F_CONSTANT)) {
			eprintf("validator_constanted 1\n");
		}
		node->value.flags |= DERIVATOR_F_CONSTANT;
		return S_OK;
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_VARIABLE) {
		if (node->left || node->right) {
			return S_FAIL;
		}
		return S_OK;
	}

	// not number nor operator
	if ((node->value.flags & DERIVATOR_F_OPERATOR) != DERIVATOR_F_OPERATOR) {
		return S_FAIL;
	}
	const struct expression_operator *op = node->value.ptr;

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
		if (!(node->value.flags & DERIVATOR_F_CONSTANT)) {
			eprintf("validator_constanted 2\n");
		}
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

	if ((value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_VARIABLE) {
		fprintf(out_stream, "x");
		return DS_OK;
	}

	if ((value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		fprintf(out_stream, "%s", ((struct expression_operator *)value.ptr)->name);
		return DS_OK;
	}

	return DS_INVALID_ARG;
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

struct tree_node *expr_create_variable_tnode(size_t idx) {
	struct tree_node *node = tnode_ctor();

	if (!node)
		return NULL;

	node->value.varidx = idx;
	node->value.flags = DERIVATOR_F_VARIABLE;
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
