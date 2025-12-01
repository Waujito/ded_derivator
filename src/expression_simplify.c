#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "expression.h"

#include <math.h>

static const double deps = 1e-9;

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

		if (((lnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(lnode->value.fnum - 1) < deps)) {
			if (lnode) tnode_recursive_dtor(lnode, NULL);

			return rnode;
		}
		if (((rnode->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_NUMBER &&
			fabs(rnode->value.fnum - 1) < deps)) {
			if (rnode) tnode_recursive_dtor(rnode, NULL);

			return lnode;
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
