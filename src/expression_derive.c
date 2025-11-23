#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "expression.h"

static struct tree_node *create_number_node(int64_t snum) {
	struct tree_node *node = tnode_ctor();

	if (!node)
		return NULL;

	node->value.snum = snum;
	node->value.flags = DERIVATOR_F_NUMBER;
	node->left = NULL;
	node->right = NULL;

	return node;
}

static struct tree_node *create_operator_node(const struct expression_operator *op, 
                                              struct tree_node *left, 
                                              struct tree_node *right) {
	struct tree_node *node = tnode_ctor();
	if (!node)
		return NULL;

	node->value.ptr = (void*)op;
	node->value.flags = DERIVATOR_F_OPERATOR;
	node->left = left;
	node->right = right;

	return node;
}

static struct tree_node *copy_tree_node(struct tree_node *original) {
	assert (original);

	struct tree_node *copy = tnode_ctor();
	if (!copy)
		return NULL;

	copy->value = original->value;

	if (original->left) {
		copy->left = copy_tree_node(original->left);

		if (!copy->left) {
			tnode_recursive_dtor(copy, NULL);
			return NULL;
		}
	}

	if (original->right) {
		copy->right = copy_tree_node(original->right);

		if (!copy->right) {
			tnode_recursive_dtor(copy, NULL);
			return NULL;
		}
	}

	return copy;
}

enum expression_indexes {
	DERIVATOR_IDX_PLUS,
	DERIVATOR_IDX_MINUS,
	DERIVATOR_IDX_MULTIPLY,
	DERIVATOR_IDX_DIVIDE,
	DERIVATOR_IDX_POW,
	DERIVATOR_IDX_LN,
	DERIVATOR_IDX_X,
};

#define DERIV_OP(idx) &expression_operators[idx]

// d(u+v)/dx = du/dx + dv/dx
static struct tree_node *derive_addition(struct tree_node *node) {
	assert (node);

	struct tree_node *left_deriv = expression_derive(node->left);
	if (!left_deriv) {
		return NULL;
	}

	struct tree_node *right_deriv = expression_derive(node->right);

	if (!right_deriv) {
		tnode_recursive_dtor(left_deriv, NULL);
		return NULL;
	}

	struct tree_node * op_node = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_PLUS), left_deriv, right_deriv);

	if (!op_node) {
		tnode_recursive_dtor(left_deriv, NULL);
		tnode_recursive_dtor(right_deriv, NULL);
		return NULL;
	}

	return op_node;
}

// d(u-v)/dx = du/dx - dv/dx
static struct tree_node *derive_subtraction(struct tree_node *node) {
	assert (node);

	struct tree_node *left_deriv = expression_derive(node->left);
	if (!left_deriv) {
		return NULL;
	}

	struct tree_node *right_deriv = expression_derive(node->right);

	if (!right_deriv) {
		tnode_recursive_dtor(left_deriv, NULL);
		return NULL;
	}

	struct tree_node * op_node = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_MINUS), left_deriv, right_deriv);

	if (!op_node) {
		tnode_recursive_dtor(left_deriv, NULL);
		tnode_recursive_dtor(right_deriv, NULL);
		return NULL;
	}

	return op_node;
}

// d(u*v)/dx = u*dv/dx + v*du/dx
static struct tree_node *derive_multiplication(struct tree_node *node) {
	assert (node);

	struct tree_node *u = copy_tree_node(node->left);
	struct tree_node *dv_dx = expression_derive(node->right);

	if (!u || !dv_dx) {
		if (u) tnode_recursive_dtor(u, NULL);
		if (dv_dx) tnode_recursive_dtor(dv_dx, NULL);

		return NULL;
	}

	// u * dv_dx
	struct tree_node *left_product = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), u, dv_dx);

	if (!left_product) {
		tnode_recursive_dtor(u, NULL);
		tnode_recursive_dtor(dv_dx, NULL);

		return NULL;
	}


	struct tree_node *v = copy_tree_node(node->right);
	struct tree_node *du_dx = expression_derive(node->left);

	if (!v || !du_dx) {
		tnode_recursive_dtor(left_product, NULL);

		if (v) tnode_recursive_dtor(v, NULL);
		if (du_dx) tnode_recursive_dtor(du_dx, NULL);

		return NULL;
	}
	
	// v * du_dx  
	struct tree_node *right_product = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v, du_dx);

	if (!right_product) {
		tnode_recursive_dtor(left_product, NULL);

		tnode_recursive_dtor(v, NULL);
		tnode_recursive_dtor(du_dx, NULL);
		return NULL;
	}

	struct tree_node * op_node = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_PLUS), left_product, right_product);

	if (!op_node) {
		tnode_recursive_dtor(left_product, NULL);
		tnode_recursive_dtor(right_product, NULL);
		return NULL;
	}

	return op_node;


}

// d(u/v)/dx = (v*du/dx - u*dv/dx) / v^2
static struct tree_node *derive_division(struct tree_node *node) {
	assert (node);

	struct tree_node *product_der = derive_multiplication(node);

	if (!product_der) {
		return NULL;
	}

	product_der->value.ptr = DERIV_OP(DERIVATOR_IDX_MINUS);

	struct tree_node *v = copy_tree_node(node->right);
	struct tree_node *two_node = create_number_node(2);

	if (!v || !two_node) {
		tnode_recursive_dtor(product_der, NULL);

		if (v) tnode_recursive_dtor(v, NULL);
		if (two_node) tnode_recursive_dtor(two_node, NULL);

		return NULL;
	}

	struct tree_node *v_squared = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_POW), v, two_node
	);

	if (!v_squared) {
		tnode_recursive_dtor(product_der, NULL);
		tnode_recursive_dtor(v, NULL);
		tnode_recursive_dtor(two_node, NULL);

		return NULL;
	}

	struct tree_node * op_node = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_DIVIDE), product_der, v_squared);

	if (!op_node) {
		tnode_recursive_dtor(product_der, NULL);
		tnode_recursive_dtor(v_squared, NULL);
		return NULL;
	}

	return op_node;
}

// d(u^v)/dx = (u^v)*(v*d(ln(u))/dx + ln(u)*dv/dx) = (u^v)*d(v*ln(u))/dx
static struct tree_node *derive_power(struct tree_node *node) {
	assert (node);

	struct tree_node *u = node->left;
	struct tree_node *v = node->right;

	struct tree_node *o_pow = copy_tree_node(node);
	if (!o_pow) {
		return NULL;
	}

	struct tree_node *u_cpy = copy_tree_node(u);
	struct tree_node *v_cpy = copy_tree_node(v);
	if (!u_cpy || !v_cpy) {
		tnode_recursive_dtor(o_pow, NULL);
		if (u_cpy) tnode_recursive_dtor(u_cpy, NULL);
		return NULL;
	}

	struct tree_node *ln_u = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_LN), u_cpy, NULL);

	if (!ln_u) {
		tnode_recursive_dtor(o_pow, NULL);
		tnode_recursive_dtor(u_cpy, NULL);
		tnode_recursive_dtor(v_cpy, NULL);
		return NULL;
	}
	u_cpy = NULL;

	struct tree_node *mul_op = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v_cpy, ln_u);
	if (!mul_op) {
		tnode_recursive_dtor(o_pow, NULL);
		tnode_recursive_dtor(ln_u, NULL);
		tnode_recursive_dtor(v_cpy, NULL);
		return NULL;
	}
	ln_u = NULL;
	v_cpy = NULL;

	struct tree_node *mul_derivative = expression_derive(mul_op);
	if (!mul_derivative) {
		tnode_recursive_dtor(o_pow, NULL);
		tnode_recursive_dtor(mul_op, NULL);
		return NULL;
	}
	tnode_recursive_dtor(mul_op, NULL);

	struct tree_node * op_node = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), o_pow, mul_derivative);

	if (!op_node) {
		tnode_recursive_dtor(o_pow, NULL);
		tnode_recursive_dtor(mul_derivative, NULL);
		return NULL;
	}

	return op_node;

	return NULL;
}

// d(ln(u))/dx = (du/dx)*(1/u)
static struct tree_node *derive_log(struct tree_node *node) {
	assert (node);

	if (node->right != NULL)
		return NULL;

	struct tree_node *u = node->left;

	struct tree_node *du_dx = expression_derive(u);
	if (!du_dx) {
		return NULL;
	}

	struct tree_node *u_cpy = copy_tree_node(u);
	if (!u_cpy) {
		tnode_recursive_dtor(du_dx, NULL);
		return NULL;
	}

	struct tree_node * op_node = create_operator_node(
		DERIV_OP(DERIVATOR_IDX_DIVIDE), du_dx, u_cpy);

	if (!op_node) {
		tnode_recursive_dtor(du_dx, NULL);
		tnode_recursive_dtor(u_cpy, NULL);
		return NULL;
	}

	return op_node;
}

// dx/dx = 1
static struct tree_node *derive_variable(struct tree_node *node) {
	(void)node;

	return create_number_node(1);
}

// C/dx = 0
static struct tree_node *derive_constant(struct tree_node *node) {
	(void)node;

	return create_number_node(0);
}

#define DECLARE_DERIVE_STRUCTURE(idx, _name, _deriver, _latex)	\
	[idx] = { .name = _name, .deriver = _deriver, .latex_name = _latex}

const struct expression_operator expression_operators[] = {
	DECLARE_DERIVE_STRUCTURE(DERIVATOR_IDX_PLUS, "+", derive_addition, "\\edplus"),
	DECLARE_DERIVE_STRUCTURE(DERIVATOR_IDX_MINUS, "-", derive_subtraction, "\\edminus"),
	DECLARE_DERIVE_STRUCTURE(DERIVATOR_IDX_MULTIPLY, "*", derive_multiplication, "\\edmultiply"),
	DECLARE_DERIVE_STRUCTURE(DERIVATOR_IDX_DIVIDE, "/", derive_division, "\\eddivide"),
	DECLARE_DERIVE_STRUCTURE(DERIVATOR_IDX_POW, "^", derive_power, "\\edpower"),
	DECLARE_DERIVE_STRUCTURE(DERIVATOR_IDX_LN, "ln", derive_log, "\\edln"),
	DECLARE_DERIVE_STRUCTURE(DERIVATOR_IDX_X, "x", derive_variable, "\\edx"),
	{ 0 },
};

struct tree_node *expression_derive(struct tree_node *node) {
	assert (node);

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		return derive_constant(node);
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		const struct expression_operator *op = 
			(const struct expression_operator *)node->value.ptr;

		return op->deriver(node);
	}

	return NULL;
}

int derivate(struct expression *expr, struct expression *derivative) {
	assert (expr);
	assert (derivative);

	if (!expr->tree.root) {
		return 1;
	}

	struct tree_node *derivative_root = expression_derive(expr->tree.root);
	if (!derivative_root) {
		return 1;
	}

	if (expression_ctor(derivative)) {
		tnode_recursive_dtor(derivative_root, NULL);
		return 1;
	}

	derivative->tree.root = derivative_root;

	return 0;
}
