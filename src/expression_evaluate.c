#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "expression.h"
#include <math.h>

static const double deps = 1e-9;

#define EXPR_BINARY_OP(expr_name, ...)							\
	int expr_op_evaluator_##expr_name(struct expression *expr,			\
					struct tree_node *node, double *fnum) {		\
		assert (expr);								\
		assert (node);								\
		assert (fnum);								\
											\
		if (!node->left || !node->right) {					\
			return S_FAIL;							\
		}									\
											\
		double lnum = 0;							\
		double rnum = 0;							\
											\
		if (	tnode_evaluate(expr, node->left, &lnum) ||			\
			tnode_evaluate(expr, node->right, &rnum)) {			\
			return S_FAIL;							\
		}									\
		__VA_ARGS__								\
		return S_OK;								\
	}										\

#define EXPR_UNARY_OP(expr_name, ...)							\
	int expr_op_evaluator_##expr_name(struct expression *expr,			\
					struct tree_node *node, double *fnum) {		\
		assert (expr);								\
		assert (node);								\
		assert (fnum);								\
											\
		if (!node->left) {							\
			return S_FAIL;							\
		}									\
											\
		double src_num = 0;							\
											\
		if (	tnode_evaluate(expr, node->left, &src_num)) {			\
			return S_FAIL;							\
		}									\
		__VA_ARGS__								\
		return S_OK;								\
	}										\

EXPR_BINARY_OP(addition,
	*fnum = lnum + rnum;
)

EXPR_BINARY_OP(subtraction,
	*fnum = lnum - rnum;
)

EXPR_BINARY_OP(multiplication,
	*fnum = lnum * rnum;
)
EXPR_BINARY_OP(division,
	if (fabs(rnum) < deps) {
	       log_error("Division by zero.");
	       return S_FAIL;
	}

	*fnum = lnum / rnum;
)

EXPR_BINARY_OP(power,
	*fnum = pow(lnum, rnum);
)

EXPR_UNARY_OP(log,
	*fnum = log(src_num);
)

EXPR_UNARY_OP(sin,
	*fnum = sin(src_num);
)

EXPR_UNARY_OP(cos,
	*fnum = cos(src_num);
)

int expr_op_evaluator_variable(struct expression *expr,
				   struct tree_node *node, double *fnum) {
	assert (expr);
	assert (node);
	assert (fnum);

	return S_FAIL;
}

int tnode_evaluate(struct expression *expr,
				   struct tree_node *node, double *fnum) {
	assert (expr);
	assert (node);
	assert (fnum);

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		*fnum = node->value.fnum;
		return S_OK;
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_VARIABLE) {
		size_t var_idx = node->value.varidx;

		struct expression_variable *variable = NULL;
		if (pvector_get(&expr->variables, var_idx, (void *)&variable)) {
			log_error("pvector_get error");
			return S_FAIL;
		}

		*fnum = variable->value;
		return S_OK;
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		const struct expression_operator *op = node->value.ptr;
		return op->evaluator(expr, node, fnum);
	}

	return S_FAIL;
}

int expression_evaluate(struct expression *expr, double *fnum) {
	assert (expr);
	assert (fnum);
	assert (expr->tree.root);

	if (tnode_evaluate(expr, expr->tree.root, fnum))
		return S_FAIL;

	return S_OK;
}
