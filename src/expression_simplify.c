#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "expression.h"

#include <math.h>

static const double deps = 1e-9;

#define EXPR_TNODE_IS_NUMBER(node) ((node->value.flags & DERIVATOR_F_OPERATOR) \
						== DERIVATOR_F_NUMBER)
#define EXPR_TNODE_IS_VARIABLE(node) ((node->value.flags & DERIVATOR_F_OPERATOR) \
						== DERIVATOR_F_VARIABLE)
#define EXPR_TNODE_IS_OPERATOR(node) ((node->value.flags & DERIVATOR_F_OPERATOR) \
						== DERIVATOR_F_OPERATOR)
#define EXPR_TNODE_IS_CONSTANT(node) (node->value.flags & DERIVATOR_F_CONSTANT)

struct tree_node *tnode_simplify(struct expression *expr, struct tree_node *node) {
	assert (node);

	if (EXPR_TNODE_IS_CONSTANT(node)) {
		double fnum = 0;

		if (tnode_evaluate(expr, node, &fnum)) {
			return NULL;
		}

		return expr_create_number_tnode(fnum);
	}

	if (EXPR_TNODE_IS_NUMBER(node)) {
		return expr_copy_tnode(expr, node);
	}

	if (EXPR_TNODE_IS_VARIABLE(node)) {
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
		if ((EXPR_TNODE_IS_NUMBER(lnode) && fabs(lnode->value.fnum) < deps) ||
		    (EXPR_TNODE_IS_NUMBER(rnode) && fabs(rnode->value.fnum) < deps)) {	
			if (lnode) tnode_recursive_dtor(lnode, NULL);
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return expr_create_number_tnode(0);
		}

		if (EXPR_TNODE_IS_NUMBER(lnode)) {
			struct expression_operator *rnop = rnode->value.ptr;

			if ((EXPR_TNODE_IS_OPERATOR(rnode) && 
			     rnop->idx == DERIVATOR_IDX_MULTIPLY &&
			     EXPR_TNODE_IS_NUMBER(rnode->left))) {
				double fnum = lnode->value.fnum * rnode->left->value.fnum;

				tnode_recursive_dtor(lnode, NULL);
				tnode_recursive_dtor(rnode->left, NULL);

				struct tree_node *real_rnode = rnode->right;
				tnode_dtor(rnode, NULL);
				rnode = real_rnode;

				lnode = expr_create_number_tnode(fnum);

				if (!lnode) {
					tnode_recursive_dtor(rnode, NULL);
					return NULL;
				}
			}
		}

		if ((EXPR_TNODE_IS_NUMBER(lnode) && fabs(lnode->value.fnum - 1) < deps)) {
			if (lnode) tnode_recursive_dtor(lnode, NULL);

			return rnode;
		}
		if ((EXPR_TNODE_IS_NUMBER(rnode) && fabs(rnode->value.fnum - 1) < deps)) {
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return lnode;
		}	
	}

	if (op->idx == DERIVATOR_IDX_DIVIDE) {
		if ((EXPR_TNODE_IS_NUMBER(lnode) && fabs(lnode->value.fnum) < deps)) {	
			if (lnode) tnode_recursive_dtor(lnode, NULL);
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return expr_create_number_tnode(0);
		}
	}

	if (op->idx == DERIVATOR_IDX_PLUS) {
		if ((EXPR_TNODE_IS_NUMBER(lnode) && fabs(lnode->value.fnum) < deps)) {
			if (lnode) tnode_recursive_dtor(lnode, NULL);

			return rnode;
		}
		if ((EXPR_TNODE_IS_NUMBER(rnode) && fabs(rnode->value.fnum) < deps)) {
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return lnode;
		}
	}

	if (op->idx == DERIVATOR_IDX_MINUS) {
		if ((EXPR_TNODE_IS_NUMBER(rnode) && fabs(rnode->value.fnum) < deps)) {
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return lnode;
		}
	}
	if (op->idx == DERIVATOR_IDX_POW) {
		if ((EXPR_TNODE_IS_NUMBER(rnode) && fabs(rnode->value.fnum) < deps)) {
			if (lnode) tnode_recursive_dtor(lnode, NULL);
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return expr_create_number_tnode(1);
		}

		if ((EXPR_TNODE_IS_NUMBER(rnode) && fabs(rnode->value.fnum - 1) < deps)) {
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return lnode;
		}
	}

	struct tree_node *new_node = expr_create_operator_tnode(op, lnode, rnode);
	if (!new_node) {
		if (lnode) tnode_recursive_dtor(lnode, NULL);
		if (rnode) tnode_recursive_dtor(rnode, NULL);

		return NULL;
	}

	if (EXPR_TNODE_IS_NUMBER(new_node)) {
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
	simplified->latex_file = expr->latex_file;

	if (pvector_clone(&simplified->variables, &expr->variables)) {
		expression_dtor(simplified);
		return S_FAIL;
	}

	if (expression_validate(simplified)) {
		return S_FAIL;
	}

	return S_OK;
}
