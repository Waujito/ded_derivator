#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "expression.h"

#include <math.h>

static const double deps = 1e-9;

static struct tree_node *tnode_derive(struct expression *expr, struct tree_node *node);

#define DERIV_OP(idx) expression_operators[idx]

// d(u+v)/dx = du/dx + dv/dx
struct tree_node *expr_op_deriver_addition(struct expression *expr, struct tree_node *node) {
	assert (node);

	struct tree_node *left_deriv = tnode_derive(expr, node->left);
	if (!left_deriv) {
		return NULL;
	}

	struct tree_node *right_deriv = tnode_derive(expr, node->right);

	if (!right_deriv) {
		tnode_recursive_dtor(left_deriv, NULL);
		return NULL;
	}

	struct tree_node * op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_PLUS), left_deriv, right_deriv);

	if (!op_node) {
		tnode_recursive_dtor(left_deriv, NULL);
		tnode_recursive_dtor(right_deriv, NULL);
		return NULL;
	}

	return op_node;
}

// d(u-v)/dx = du/dx - dv/dx
struct tree_node *expr_op_deriver_subtraction(struct expression *expr, struct tree_node *node) {
	assert (node);

	struct tree_node *left_deriv = tnode_derive(expr, node->left);
	if (!left_deriv) {
		return NULL;
	}

	struct tree_node *right_deriv = tnode_derive(expr, node->right);

	if (!right_deriv) {
		tnode_recursive_dtor(left_deriv, NULL);
		return NULL;
	}

	struct tree_node * op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MINUS), left_deriv, right_deriv);

	if (!op_node) {
		tnode_recursive_dtor(left_deriv, NULL);
		tnode_recursive_dtor(right_deriv, NULL);
		return NULL;
	}

	return op_node;
}

// d(u*v)/dx = u*dv/dx + v*du/dx
struct tree_node *expr_op_deriver_multiplication(struct expression *expr, struct tree_node *node) {
	assert (node);

	struct tree_node *u = expr_copy_tnode(expr, node->left);
	struct tree_node *dv_dx = tnode_derive(expr, node->right);

	if (!u || !dv_dx) {
		if (u) tnode_recursive_dtor(u, NULL);
		if (dv_dx) tnode_recursive_dtor(dv_dx, NULL);

		return NULL;
	}

	// u * dv_dx
	struct tree_node *left_product = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), u, dv_dx);

	if (!left_product) {
		tnode_recursive_dtor(u, NULL);
		tnode_recursive_dtor(dv_dx, NULL);

		return NULL;
	}


	struct tree_node *v = expr_copy_tnode(expr, node->right);
	struct tree_node *du_dx = tnode_derive(expr, node->left);

	if (!v || !du_dx) {
		tnode_recursive_dtor(left_product, NULL);

		if (v) tnode_recursive_dtor(v, NULL);
		if (du_dx) tnode_recursive_dtor(du_dx, NULL);

		return NULL;
	}
	
	// v * du_dx  
	struct tree_node *right_product = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v, du_dx);

	if (!right_product) {
		tnode_recursive_dtor(left_product, NULL);

		tnode_recursive_dtor(v, NULL);
		tnode_recursive_dtor(du_dx, NULL);
		return NULL;
	}

	struct tree_node * op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_PLUS), left_product, right_product);

	if (!op_node) {
		tnode_recursive_dtor(left_product, NULL);
		tnode_recursive_dtor(right_product, NULL);
		return NULL;
	}

	return op_node;


}

// d(u/v)/dx = (v*du/dx - u*dv/dx) / v^2
struct tree_node *expr_op_deriver_division(struct expression *expr, struct tree_node *node) {
	assert (node);

	struct tree_node *product_der = expr_op_deriver_multiplication(expr, node);

	if (!product_der) {
		return NULL;
	}

	product_der->value.ptr = DERIV_OP(DERIVATOR_IDX_MINUS);

	struct tree_node *v = expr_copy_tnode(expr, node->right);
	struct tree_node *two_node = expr_create_number_tnode(2);

	if (!v || !two_node) {
		tnode_recursive_dtor(product_der, NULL);

		if (v) tnode_recursive_dtor(v, NULL);
		if (two_node) tnode_recursive_dtor(two_node, NULL);

		return NULL;
	}

	struct tree_node *v_squared = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_POW), v, two_node
	);

	if (!v_squared) {
		tnode_recursive_dtor(product_der, NULL);
		tnode_recursive_dtor(v, NULL);
		tnode_recursive_dtor(two_node, NULL);

		return NULL;
	}

	struct tree_node * op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_DIVIDE), product_der, v_squared);

	if (!op_node) {
		tnode_recursive_dtor(product_der, NULL);
		tnode_recursive_dtor(v_squared, NULL);
		return NULL;
	}

	return op_node;
}

// d(u^v)/dx = (u^v)*(v*d(ln(u))/dx + ln(u)*dv/dx) = (u^v)*d(v*ln(u))/dx
struct tree_node *expr_op_deriver_power(struct expression *expr, struct tree_node *node) {
	assert (node);

	struct tree_node *u = node->left;
	struct tree_node *v = node->right;

	struct tree_node *o_pow = expr_copy_tnode(expr, node);
	if (!o_pow) {
		return NULL;
	}

	struct tree_node *u_cpy = expr_copy_tnode(expr, u);
	struct tree_node *v_cpy = expr_copy_tnode(expr, v);
	if (!u_cpy || !v_cpy) {
		tnode_recursive_dtor(o_pow, NULL);
		if (u_cpy) tnode_recursive_dtor(u_cpy, NULL);
		return NULL;
	}

	struct tree_node *ln_u = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_LN), u_cpy, NULL);

	if (!ln_u) {
		tnode_recursive_dtor(o_pow, NULL);
		tnode_recursive_dtor(u_cpy, NULL);
		tnode_recursive_dtor(v_cpy, NULL);
		return NULL;
	}
	u_cpy = NULL;

	struct tree_node *mul_op = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v_cpy, ln_u);
	if (!mul_op) {
		tnode_recursive_dtor(o_pow, NULL);
		tnode_recursive_dtor(ln_u, NULL);
		tnode_recursive_dtor(v_cpy, NULL);
		return NULL;
	}
	ln_u = NULL;
	v_cpy = NULL;

	struct tree_node *mul_derivative = tnode_derive(expr, mul_op);
	if (!mul_derivative) {
		tnode_recursive_dtor(o_pow, NULL);
		tnode_recursive_dtor(mul_op, NULL);
		return NULL;
	}
	tnode_recursive_dtor(mul_op, NULL);

	struct tree_node * op_node = expr_create_operator_tnode(
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
struct tree_node *expr_op_deriver_log(struct expression *expr, struct tree_node *node) {
	assert (node);

	if (node->right != NULL)
		return NULL;

	struct tree_node *u = node->left;

	struct tree_node *du_dx = tnode_derive(expr, u);
	if (!du_dx) {
		return NULL;
	}

	struct tree_node *u_cpy = expr_copy_tnode(expr, u);
	if (!u_cpy) {
		tnode_recursive_dtor(du_dx, NULL);
		return NULL;
	}

	struct tree_node * op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_DIVIDE), du_dx, u_cpy);

	if (!op_node) {
		tnode_recursive_dtor(du_dx, NULL);
		tnode_recursive_dtor(u_cpy, NULL);
		return NULL;
	}

	return op_node;
}

// dx/dx = 1
struct tree_node *expr_op_deriver_variable(struct expression *expr, struct tree_node *node) {
	(void)node;

	return expr_create_number_tnode(1);
}

// C/dx = 0
static struct tree_node *expr_op_deriver_constant(
	struct expression *expr, struct tree_node *node) {
	(void)node;

	return expr_create_number_tnode(0);
}

static struct tree_node *tnode_derive(struct expression *expr, struct tree_node *node) {
	assert (node);

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		return expr_op_deriver_constant(expr, node);
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		const struct expression_operator *op = 
			(const struct expression_operator *)node->value.ptr;

		return op->deriver(expr, node);
	}

	return NULL;
}

int expression_derive(struct expression *expr, struct expression *derivative) {
	assert (expr);
	assert (derivative);

	if (!expr->tree.root) {
		return S_FAIL;
	}

	struct tree_node *derivative_root = tnode_derive(expr, expr->tree.root);
	if (!derivative_root) {
		return S_FAIL;
	}

	if (expression_ctor(derivative)) {
		tnode_recursive_dtor(derivative_root, NULL);
		return S_FAIL;
	}

	derivative->tree.root = derivative_root;

	if (expression_validate(derivative)) {
		log_error("invalid");
		expression_dtor(derivative);
		return S_FAIL;
	}

	return S_OK;
}

static struct tree_node *tnode_simplify(struct expression *expr, struct tree_node *node) {
	assert (node);

	if (node->value.flags & DERIVATOR_F_CONSTANT) {
		double fnum = 0;

		if (tnode_evaluate(expr, node, &fnum)) {
			return NULL;
		}

		return expr_create_number_tnode(fnum);
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		return expr_copy_tnode(expr, node);
	}

	struct expression_operator *op = node->value.ptr;

	struct tree_node *lnode = NULL, *rnode = NULL;
	if (node->left) {
		lnode = tnode_simplify(expr, node->left);
		if (!lnode) {
			return NULL;
		}
	}
	if (node->right) {
		rnode = tnode_simplify(expr, node->right);
		if (!rnode) {
			if (lnode) tnode_recursive_dtor(lnode, NULL);

			return NULL;
		}
	}

	if (op->idx == DERIVATOR_IDX_MULTIPLY) {
		if (((lnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(lnode->value.fnum) < deps) ||
		    ((rnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(rnode->value.fnum) < deps)) {
			
			if (lnode) tnode_recursive_dtor(lnode, NULL);
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return expr_create_number_tnode(0);
		}
	}

	if (op->idx == DERIVATOR_IDX_DIVIDE) {
		if (((lnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(lnode->value.fnum) < deps)) {
			
			if (lnode) tnode_recursive_dtor(lnode, NULL);
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return expr_create_number_tnode(0);
		}
	}

	if (op->idx == DERIVATOR_IDX_PLUS) {
		if (((lnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(lnode->value.fnum) < deps)) {
			if (lnode) tnode_recursive_dtor(lnode, NULL);

			return rnode;
		}
		if (((rnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(rnode->value.fnum) < deps)) {
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return lnode;
		}
	}

	if (op->idx == DERIVATOR_IDX_MINUS) {
		if (((rnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(rnode->value.fnum) < deps)) {
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return lnode;
		}
	}
	if (op->idx == DERIVATOR_IDX_POW) {
		if (((rnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(rnode->value.fnum) < deps)) {
			if (lnode) tnode_recursive_dtor(lnode, NULL);
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return expr_create_number_tnode(1);
		}
	}

	struct tree_node *new_node = expr_create_operator_tnode(op, lnode, rnode);
	if (!new_node) {
		if (lnode) tnode_recursive_dtor(lnode, NULL);
		if (rnode) tnode_recursive_dtor(rnode, NULL);

		return NULL;
	}

	if ((new_node->value.flags & DERIVATOR_F_CONSTANT) == DERIVATOR_F_CONSTANT) {
		double fnum = 0;

		if (tnode_evaluate(expr, new_node, &fnum)) {
			return NULL;
		}

		tnode_recursive_dtor(new_node, NULL);

		return expr_create_number_tnode(fnum);
	}

	return new_node;
}

int expression_simplify(struct expression *expr, struct expression *simplified) {
	assert (expr);
	assert (simplified);

	if (!expr->tree.root) {
		return S_FAIL;
	}

	struct tree_node *simplified_root = tnode_simplify(expr, expr->tree.root);
	if (!simplified_root) {
		return S_FAIL;
	}

	if (expression_ctor(simplified)) {
		tnode_recursive_dtor(simplified_root, NULL);
		return S_FAIL;
	}

	simplified->tree.root = simplified_root;

	if (expression_validate(simplified)) {
		return S_FAIL;
	}

	return S_OK;
}
