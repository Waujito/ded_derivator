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

	int ret = S_OK;

	struct tree_node
		*left_deriv = NULL,
		*right_deriv = NULL,
		*op_node = NULL;

	left_deriv = tnode_derive(expr, node->left);
	if (!left_deriv) {
		_CT_FAIL();
	}

	right_deriv = tnode_derive(expr, node->right);

	if (!right_deriv) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_PLUS), left_deriv, right_deriv);
	left_deriv = NULL;
	right_deriv = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(left_deriv, NULL);
	tnode_recursive_dtor(right_deriv, NULL);

	return op_node;
}

// d(u-v)/dx = du/dx - dv/dx
struct tree_node *expr_op_deriver_subtraction(struct expression *expr, struct tree_node *node) {
	assert (node);

	int ret = S_OK;

	struct tree_node
		*left_deriv = NULL,
		*right_deriv = NULL,
		*op_node = NULL;

	left_deriv = tnode_derive(expr, node->left);
	if (!left_deriv) {
		_CT_FAIL();
	}

	right_deriv = tnode_derive(expr, node->right);
	if (!right_deriv) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MINUS), left_deriv, right_deriv);
	left_deriv = NULL;
	right_deriv = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(left_deriv, NULL);
	tnode_recursive_dtor(right_deriv, NULL);

	return op_node;
}

// d(u*v)/dx = u*dv/dx + v*du/dx
struct tree_node *expr_op_deriver_multiplication(struct expression *expr, struct tree_node *node) {
	assert (node);

	int ret = S_OK;

	struct tree_node
		*u = NULL,
		*dv_dx = NULL,
		*left_product = NULL,
		*v = NULL,
		*du_dx = NULL,
		*right_product = NULL,
		*op_node = NULL;

	u = expr_copy_tnode(expr, node->left);
	dv_dx = tnode_derive(expr, node->right);

	if (!u || !dv_dx) {
		_CT_FAIL();
	}

	left_product = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), u, dv_dx);
	u = NULL;
	dv_dx = NULL;

	if (!left_product) {
		_CT_FAIL();
	}

	v = expr_copy_tnode(expr, node->right);
	du_dx = tnode_derive(expr, node->left);

	if (!v || !du_dx) {
		_CT_FAIL();
	}

	right_product = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v, du_dx);
	v = NULL;
	du_dx = NULL;

	if (!right_product) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_PLUS), left_product, right_product);
	left_product = NULL;
	right_product = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(u, NULL);
	tnode_recursive_dtor(dv_dx, NULL);
	tnode_recursive_dtor(left_product, NULL);
	tnode_recursive_dtor(v, NULL);
	tnode_recursive_dtor(du_dx, NULL);
	tnode_recursive_dtor(right_product, NULL);

	return op_node;
}

// d(u/v)/dx = (v*du/dx - u*dv/dx) / v^2
struct tree_node *expr_op_deriver_division(struct expression *expr, struct tree_node *node) {
	assert (node);

	int ret = S_OK;

	struct tree_node
		*product_der = NULL,
		*v = NULL,
		*two_node = NULL,
		*v_squared = NULL,
		*op_node = NULL;

	product_der = expr_op_deriver_multiplication(expr, node);
	if (!product_der) {
		_CT_FAIL();
	}

	product_der->value.ptr = DERIV_OP(DERIVATOR_IDX_MINUS);

	v = expr_copy_tnode(expr, node->right);
	two_node = expr_create_number_tnode(2);

	if (!v || !two_node) {
		_CT_FAIL();
	}

	v_squared = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_POW), v, two_node);
	v = NULL;
	two_node = NULL;

	if (!v_squared) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_DIVIDE), product_der, v_squared);
	product_der = NULL;
	v_squared = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(product_der, NULL);
	tnode_recursive_dtor(v, NULL);
	tnode_recursive_dtor(two_node, NULL);
	tnode_recursive_dtor(v_squared, NULL);

	return op_node;
}

// d(u^C)/dx = C*(u^(C-1))*du/dx
struct tree_node *expr_op_deriver_power_const(
	struct expression *expr, struct tree_node *node) {
	assert (expr);
	assert (node);

	int ret = S_OK;

	struct tree_node
		*u_cpy = NULL,
		*v_cpy = NULL,
		*v_pow_cpy = NULL,
		*one_scalar = NULL,
		*v_min_one = NULL,
		*u_pow_vm = NULL,
		*v_mul_upow = NULL,
		*u_derivative = NULL,
		*op_node = NULL;

	struct tree_node *u = node->left;
	struct tree_node *v = node->right;

	u_cpy = expr_copy_tnode(expr, u);
	v_cpy = expr_copy_tnode(expr, v);
	v_pow_cpy = expr_copy_tnode(expr, v);
	one_scalar = expr_create_number_tnode(1);

	if (!u_cpy || !v_cpy || !v_pow_cpy || !one_scalar) {
		_CT_FAIL();
	}

	v_min_one = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MINUS), v_pow_cpy, one_scalar);
	v_pow_cpy = NULL;
	one_scalar = NULL;

	if (!v_min_one) {
		_CT_FAIL();
	}

	u_pow_vm = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_POW), u_cpy, v_min_one);
	u_cpy = NULL;
	v_min_one = NULL;

	if (!u_pow_vm) {
		_CT_FAIL();
	}

	v_mul_upow = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v_cpy, u_pow_vm);
	v_cpy = NULL;
	u_pow_vm = NULL;

	if (!v_mul_upow) {
		_CT_FAIL();
	}

	u_derivative = tnode_derive(expr, u);
	if (!u_derivative) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v_mul_upow, u_derivative);
	v_mul_upow = NULL;
	u_derivative = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(u_cpy, NULL);
	tnode_recursive_dtor(v_cpy, NULL);
	tnode_recursive_dtor(v_pow_cpy, NULL);
	tnode_recursive_dtor(one_scalar, NULL);
	tnode_recursive_dtor(v_min_one, NULL);
	tnode_recursive_dtor(u_pow_vm, NULL);
	tnode_recursive_dtor(v_mul_upow, NULL);
	tnode_recursive_dtor(u_derivative, NULL);

	return op_node;
}

// d(u^v)/dx = (u^v)*(v*d(ln(u))/dx + ln(u)*dv/dx) = (u^v)*d(v*ln(u))/dx
struct tree_node *expr_op_deriver_power(struct expression *expr, struct tree_node *node) {
	assert (node);

	int ret = S_OK;

	struct tree_node
		*o_pow = NULL,
		*u_cpy = NULL,
		*v_cpy = NULL,
		*ln_u = NULL,
		*mul_op = NULL,
		*mul_derivative = NULL,
		*op_node = NULL;

	struct tree_node *u = node->left;
	struct tree_node *v = node->right;

	if ((v->value.flags & DERIVATOR_F_CONSTANT) == DERIVATOR_F_CONSTANT) {
		return expr_op_deriver_power_const(expr, node);
	}

	o_pow = expr_copy_tnode(expr, node);
	if (!o_pow) {
		_CT_FAIL();
	}

	u_cpy = expr_copy_tnode(expr, u);
	v_cpy = expr_copy_tnode(expr, v);
	if (!u_cpy || !v_cpy) {
		_CT_FAIL();
	}

	ln_u = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_LN), u_cpy, NULL);
	u_cpy = NULL;

	if (!ln_u) {
		_CT_FAIL();
	}

	mul_op = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), v_cpy, ln_u);
	v_cpy = NULL;
	ln_u = NULL;

	if (!mul_op) {
		_CT_FAIL();
	}

	mul_derivative = tnode_derive(expr, mul_op);
	tnode_recursive_dtor(mul_op, NULL);
	mul_op = NULL;

	if (!mul_derivative) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), o_pow, mul_derivative);
	o_pow = NULL;
	mul_derivative = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(o_pow, NULL);
	tnode_recursive_dtor(u_cpy, NULL);
	tnode_recursive_dtor(v_cpy, NULL);
	tnode_recursive_dtor(ln_u, NULL);
	tnode_recursive_dtor(mul_op, NULL);
	tnode_recursive_dtor(mul_derivative, NULL);

	return op_node;
}

// d(ln(u))/dx = (du/dx)*(1/u)
struct tree_node *expr_op_deriver_log(struct expression *expr, struct tree_node *node) {
	assert (node);

	int ret = S_OK;

	struct tree_node
		*du_dx = NULL,
		*u_cpy = NULL,
		*op_node = NULL;

	if (node->right != NULL)
		return NULL;

	struct tree_node *u = node->left;

	du_dx = tnode_derive(expr, u);
	if (!du_dx) {
		_CT_FAIL();
	}

	u_cpy = expr_copy_tnode(expr, u);
	if (!u_cpy) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_DIVIDE), du_dx, u_cpy);
	du_dx = NULL;
	u_cpy = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(du_dx, NULL);
	tnode_recursive_dtor(u_cpy, NULL);

	return op_node;
}

// d(cos(u))/dx = (du/dx)*(-sin(u))
struct tree_node *expr_op_deriver_cos(struct expression *expr, struct tree_node *node) {
	assert (node);

	int ret = S_OK;

	struct tree_node
		*du_dx = NULL,
		*u_cpy = NULL,
		*sin_node = NULL,
		*min_one = NULL,
		*min_sin = NULL,
		*op_node = NULL;

	if (node->right != NULL)
		return NULL;

	struct tree_node *u = node->left;

	du_dx = tnode_derive(expr, u);
	if (!du_dx) {
		_CT_FAIL();
	}

	u_cpy = expr_copy_tnode(expr, u);
	if (!u_cpy) {
		_CT_FAIL();
	}

	sin_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_SIN), u_cpy, NULL);
	u_cpy = NULL;

	if (!sin_node) {
		_CT_FAIL();
	}

	min_one = expr_create_number_tnode(-1);
	if (!min_one) {
		_CT_FAIL();
	}

	min_sin = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), min_one, sin_node);
	min_one = NULL;
	sin_node = NULL;

	if (!min_sin) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), min_sin, du_dx);
	min_sin = NULL;
	du_dx = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(du_dx, NULL);
	tnode_recursive_dtor(u_cpy, NULL);
	tnode_recursive_dtor(sin_node, NULL);
	tnode_recursive_dtor(min_one, NULL);
	tnode_recursive_dtor(min_sin, NULL);

	return op_node;
}

// d(sin(u))/dx = (du/dx)*(cos(u))
struct tree_node *expr_op_deriver_sin(struct expression *expr, struct tree_node *node) {
	assert (node);

	int ret = S_OK;

	struct tree_node
		*du_dx = NULL,
		*u_cpy = NULL,
		*cos_node = NULL,
		*op_node = NULL;

	if (node->right != NULL)
		return NULL;

	struct tree_node *u = node->left;

	du_dx = tnode_derive(expr, u);
	if (!du_dx) {
		_CT_FAIL();
	}

	u_cpy = expr_copy_tnode(expr, u);
	if (!u_cpy) {
		_CT_FAIL();
	}

	cos_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_COS), u_cpy, NULL);
	u_cpy = NULL;

	if (!cos_node) {
		_CT_FAIL();
	}

	op_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_MULTIPLY), cos_node, du_dx);
	cos_node = NULL;
	du_dx = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(du_dx, NULL);
	tnode_recursive_dtor(u_cpy, NULL);
	tnode_recursive_dtor(cos_node, NULL);

	return op_node;
}

// d(o(x^n))/dx = o(x^n)
struct tree_node *expr_op_deriver_small_o(struct expression *expr, struct tree_node *node) {
	assert (node);

	return expr_copy_tnode(expr, node);
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

	struct tree_node *derivative_node = NULL;	

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER) {
		derivative_node = expr_op_deriver_constant(expr, node);
	} else if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_VARIABLE) {
		derivative_node = expr_op_deriver_variable(expr, node);
	} else if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		const struct expression_operator *op = 
			(const struct expression_operator *)node->value.ptr;

		derivative_node = op->deriver(expr, node);
	}

	if (derivative_node) {
		struct tree_node *nnode = tnode_simplify(expr, derivative_node);
		tnode_recursive_dtor(derivative_node, NULL);
		derivative_node = nnode;
	}

	if (derivative_node && expr->latex_file) {
		fprintf(expr->latex_file, "\\begin{equation}\n");
		fprintf(expr->latex_file, "\\frac{d}{dx}(");
		tnode_to_latex(expr, node, expr->latex_file);
		fprintf(expr->latex_file, ") = ");
		tnode_to_latex(expr, derivative_node, expr->latex_file);
		fprintf(expr->latex_file, "\\end{equation}\n\n");
	}

	return derivative_node;
}

/*
int expression_derive(struct expression *expr, struct expression *derivative) {
	assert (expr);
	assert (derivative);

	if (!expr->tree.root) {
		return S_FAIL;
	}

	if (expression_validate(expr)) {
		log_error("invalid");
		expression_dtor(expr);
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
	derivative->latex_file = expr->latex_file;

	if (pvector_clone(&derivative->variables, &expr->variables)) {
		expression_dtor(derivative);
		return S_FAIL;
	}

	if (expression_validate(derivative)) {
		log_error("invalid");
		expression_dtor(derivative);
		return S_FAIL;
	}

	return S_OK;
}
*/

int expression_derive_nth(struct expression *expr, int nth) {
	assert (expr);

	if (nth < 0) {
		log_error("No integration yet!");
		return S_FAIL;
	}

	int counted_derivatives = (int) expr->derivatives.len;
	struct tree_node *latest_derivative = expr->tree.root;

	// Derivatives up to nth are already counted
	if (counted_derivatives >= nth) {
		return S_OK;
	}

	if (counted_derivatives > 0) {
		struct tree *derivative_tree = NULL;
		if (pvector_get(&expr->derivatives, (size_t) counted_derivatives - 1,
					(void **)&derivative_tree)) {
			return S_FAIL;
		}

		latest_derivative = derivative_tree->root;

		if (expr->latex_file) {
			fprintf(expr->latex_file,
				"\\section{More derivatives to the God of derivatives}\n");
		}
	} else {
		if (expr->latex_file) {
			fprintf(expr->latex_file,
				"\\section{Find the derivatives}\n");
		}
	}

	for (int i = counted_derivatives + 1; i <= nth; i++) {
		if (expr->latex_file) {
			fprintf(expr->latex_file,
				"\\subsection{Find the %dth derivative}\n\n", i);
		}

		struct tree_node *cur_derivative = tnode_derive(expr, latest_derivative);

		if (!cur_derivative) {
			return S_FAIL;
		}

		struct tree derivative_tree = {0};
		if (tree_ctor(&derivative_tree)) {
			tnode_recursive_dtor(cur_derivative, NULL);
			return S_FAIL;
		}
		derivative_tree.root = cur_derivative;

		if (pvector_push_back(&expr->derivatives, &derivative_tree)) {
			tree_dtor(&derivative_tree);
			return S_FAIL;
		}

		latest_derivative = cur_derivative;

		if (expr->latex_file) {
			fprintf(expr->latex_file, "So, the %dth derivative is: \n", i);
			latex_print_expression_function(expr, i, expr->latex_file);
		}
	}

	return S_OK;
}

double factorial(int nth) {
	double fact = 1;

	for (int i = 1; i <= nth; i++) {
		fact *= i;
	}

	return fact;
}

int expression_taylor_series_nth(struct expression *expr,
				 struct expression *series, int nth) {
	assert (expr);
	assert (series);

	int ret = S_OK;

	struct expression_variable *diff_variable = NULL;
	struct tree_node
		*nth_tailor = NULL,
		*x_node = NULL,
		*x_power = NULL,
		*x_powered_node = NULL,
		*nth_tailor_sym = NULL,
		*new_tailor_op = NULL,
		*ox_node = NULL,
		*ox_power = NULL,
		*ox_powered_node = NULL,
		*small_o_node = NULL,
		*num0_node = NULL,
		*tailor_root = NULL,
		*last_tailor_node = NULL,
		*x0_node = NULL,
		*x_minus_x0_node = NULL,
		*simplified_root = NULL;
	double tailor0 = 0;

	if (nth < 0) {
		log_error("No integration yet!");
		_CT_FAIL();
	}

	if (expression_derive_nth(expr, nth)) {
		_CT_FAIL();
	}

	if (expression_evaluate(expr, &tailor0)) {
		log_error("0Th expr evaluate");
		_CT_FAIL();
	}

	ox_node = expr_create_variable_tnode(expr->differentiating_variable);
	ox_power = expr_create_number_tnode(nth);
	if (!ox_node || !ox_power) {
		_CT_FAIL();
	}

	ox_powered_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_POW), ox_node, ox_power
	);

	if (!ox_powered_node) {
		_CT_FAIL();	
	}
	ox_node = NULL;
	ox_power = NULL;

	small_o_node = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_SMALL_O), ox_powered_node, NULL
	);

	if (!small_o_node) {
		_CT_FAIL();	
	}
	ox_powered_node = NULL;

	num0_node = expr_create_number_tnode(tailor0);
	if (!num0_node) {
		_CT_FAIL();	
	}

	tailor_root = expr_create_operator_tnode(
		DERIV_OP(DERIVATOR_IDX_PLUS), num0_node, small_o_node
	);

	if (!tailor_root) {
		_CT_FAIL();	
		return S_FAIL;
	}
	small_o_node = NULL;

	last_tailor_node = num0_node;
	num0_node = NULL;

	if (pvector_get(
		&expr->variables, expr->differentiating_variable, (void **)&diff_variable)) {
		_CT_FAIL();	
	}

	for (int i = 1; i <= nth; i++) {
		struct tree *derivative_tree = NULL;
		if (pvector_get(&expr->derivatives, (size_t) (i - 1),
					(void **)&derivative_tree)) {
			_CT_FAIL();
		};

		double fnum = 0;
		if (tnode_evaluate(expr, derivative_tree->root, &fnum)) {
			_CT_FAIL();
		}

		fnum /= factorial(i);

		nth_tailor = expr_create_number_tnode(fnum);
		x_node = expr_create_variable_tnode(expr->differentiating_variable);
		x0_node = expr_create_number_tnode(diff_variable->value);
		x_power = expr_create_number_tnode(i);
		if (!nth_tailor || !x_node || !x0_node || !x_power) {
			_CT_FAIL();
		}

		x_minus_x0_node = expr_create_operator_tnode(
			DERIV_OP(DERIVATOR_IDX_MINUS), x_node, x0_node 
		);

		if (!x_minus_x0_node) {
			_CT_FAIL();
		}

		x_node = NULL;
		x0_node = NULL;

		x_powered_node = expr_create_operator_tnode(
			DERIV_OP(DERIVATOR_IDX_POW), x_minus_x0_node, x_power
		);

		if (!x_powered_node) {
			_CT_FAIL();	
		}

		x_minus_x0_node = NULL;
		x_power = NULL;

		nth_tailor_sym = expr_create_operator_tnode(
			DERIV_OP(DERIVATOR_IDX_MULTIPLY), nth_tailor, x_powered_node 
		);

		if (!nth_tailor_sym) {
			_CT_FAIL();	
		}

		nth_tailor = NULL;
		x_powered_node = NULL;

		new_tailor_op = expr_create_operator_tnode(
			DERIV_OP(DERIVATOR_IDX_PLUS), last_tailor_node, nth_tailor_sym 
		);

		if (!new_tailor_op) {
			_CT_FAIL();
		}

		nth_tailor_sym = NULL;

		tailor_root->left = new_tailor_op;
		last_tailor_node = new_tailor_op;

		new_tailor_op = NULL;
	}

	if (expression_ctor(series)) {
		_CT_FAIL();
	}

	if (pvector_clone(&series->variables, &expr->variables)) {
		_CT_FAIL();
	}

	simplified_root = tnode_simplify(series, tailor_root);
	if (!simplified_root) {
		_CT_FAIL();
	}
	tnode_recursive_dtor(tailor_root, NULL);
	tailor_root = simplified_root;
	simplified_root = NULL;

	series->differentiating_variable = expr->differentiating_variable;
	series->tree.root = tailor_root;
	tailor_root = NULL;

_CT_EXIT_POINT:
	tnode_recursive_dtor(ox_node, NULL);
	tnode_recursive_dtor(ox_power, NULL);
	tnode_recursive_dtor(ox_powered_node, NULL);
	tnode_recursive_dtor(small_o_node, NULL);
	tnode_recursive_dtor(num0_node, NULL);
	tnode_recursive_dtor(x_node, NULL);
	tnode_recursive_dtor(nth_tailor, NULL);
	tnode_recursive_dtor(x_power, NULL);
	tnode_recursive_dtor(x0_node, NULL);
	tnode_recursive_dtor(x_minus_x0_node, NULL);
	tnode_recursive_dtor(x_powered_node, NULL);
	tnode_recursive_dtor(nth_tailor_sym , NULL);

	tnode_recursive_dtor(tailor_root, NULL);

	return ret;
}
