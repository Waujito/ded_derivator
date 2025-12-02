#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ctio.h"
#include "types.h"
#include "expression.h"
#include <stdbool.h>
#include <ctype.h>

#define S_CONTINUE (-2)

#define CALL_PARSER(parserName, s, node)			\
({								\
	int cpret_ = parserName(s, node);			\
	cpret_;							\
})

#define PARSER_RET_STATUS(status)				\
({								\
	typeof(status) prs_status_ = status;			\
	if ((int)prs_status_ && (int)prs_status_ != S_CONTINUE)	\
		eprintf("%s: ", __func__);			\
	(int)prs_status_;					\
})

struct pvector var_names = {0};

static void var_destructor(void *el) {
	struct expression_variable *ev = el;

	free(ev->name);
}

static int getPrimaryExpression(const char **s, struct tree_node **node);
static int getExpression(const char **s, struct tree_node **node);

static int getNumber(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	char *endptr = NULL;
	double fnum = strtod(*s, &endptr);

	if (*s != endptr) {
		*node = expr_create_number_tnode(fnum);
		*s = endptr;

		return PARSER_RET_STATUS(S_OK);
	}

	return PARSER_RET_STATUS(S_CONTINUE);
}

static int getOperator(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	const char *str_op_end = *s;

	while (isalpha(*str_op_end) || isdigit(*str_op_end)) {
		str_op_end++;
	}

	if (*str_op_end != '(') {
		return PARSER_RET_STATUS(S_CONTINUE);
	}

	size_t str_op_len = (size_t)(str_op_end - *s);

	str_op_end++;

	const struct expression_operator *const *derop_ptr = expression_operators;
	while (*derop_ptr != NULL) {
		if (!strncmp(*s, (*derop_ptr)->name, str_op_len) &&
			strlen((*derop_ptr)->name) == str_op_len) {

			*s = str_op_end;

			struct tree_node *lnode = NULL;
			if (CALL_PARSER(getExpression, s, &lnode)) {
				return PARSER_RET_STATUS(S_FAIL);
			}

			*node = expr_create_operator_tnode(*derop_ptr, lnode, NULL);
			if (!*node) {
				return PARSER_RET_STATUS(S_FAIL);
			}

			(*s)++;

			return PARSER_RET_STATUS(S_OK);
		}

		derop_ptr++;
	}

	return PARSER_RET_STATUS(S_CONTINUE);
}

static int getVariable(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	const char *str_op_end = *s;

	if (isdigit(*str_op_end)) {
		return PARSER_RET_STATUS(S_CONTINUE);
	}

	while (isalpha(*str_op_end) || isdigit(*str_op_end)) {
		str_op_end++;
	}

	size_t op_len = (size_t)(str_op_end - *s);

	if (op_len == 0) {
		return PARSER_RET_STATUS(S_CONTINUE);
	}

	for (size_t i = 0; i < var_names.len; i++) {
		struct expression_variable *var = NULL;
		if (pvector_get(&var_names, i, (void **)&var)) {
			continue;
		}

		if (!strncmp(var->name, *s, op_len)) {
			*s = str_op_end;

			*node = expr_create_variable_tnode(i);
			if (!(*node)) {
				return PARSER_RET_STATUS(S_FAIL);
			}
			return PARSER_RET_STATUS(S_OK);
		}
	}

	struct expression_variable new_var = {0};
	new_var.name = strndup(*s, op_len);

	*s = str_op_end;

	if (!new_var.name) {
		return PARSER_RET_STATUS(S_FAIL);
	}
	new_var.value = 0;

	*node = expr_create_variable_tnode(var_names.len);

	if (pvector_push_back(&var_names, &new_var)) {
		free(new_var.name);
		return PARSER_RET_STATUS(S_FAIL);
	}

	if (!(*node)) {
		return PARSER_RET_STATUS(S_FAIL);
	}

	return PARSER_RET_STATUS(S_OK);
}

static int getC(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	int ret = 0;

	if ((ret = CALL_PARSER(getNumber, s, node)) != S_CONTINUE) {
		return PARSER_RET_STATUS(ret);
	}

	if ((ret = CALL_PARSER(getOperator, s, node)) != S_CONTINUE) {
		return PARSER_RET_STATUS(ret);
	}

	if ((ret = CALL_PARSER(getVariable, s, node)) != S_CONTINUE) {
		return PARSER_RET_STATUS(ret);
	}

	eprintf("Expression item is not detected\n");

	return PARSER_RET_STATUS(S_FAIL);
}

static int getPow(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	int ret = 0;

	struct tree_node *lnode = NULL;

	if ((ret = CALL_PARSER(getPrimaryExpression, s, &lnode))) {
		return PARSER_RET_STATUS(ret);
	}

	if (**s == '^') {
		char operator = **s;
		(*s)++;

		struct tree_node *rnode = NULL;
		if ((ret = CALL_PARSER(getPow, s, &rnode))) {
			tnode_recursive_dtor(lnode, NULL);
			return PARSER_RET_STATUS(ret);
		}

		struct tree_node *mnode = expr_create_operator_tnode(
			expression_operators[DERIVATOR_IDX_POW], lnode, rnode);

		if (!mnode) {
			tnode_recursive_dtor(lnode, NULL);
			tnode_recursive_dtor(rnode, NULL);
			return PARSER_RET_STATUS(S_FAIL);
		}

		lnode = mnode;
	}

	*node = lnode;

	return PARSER_RET_STATUS(S_OK);

}


static int getTerm(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	int ret = 0;

	struct tree_node *lnode = NULL;

	if ((ret = CALL_PARSER(getPow, s, &lnode))) {
		return PARSER_RET_STATUS(ret);
	}

	while (**s == '*' || **s == '/') {
		char operator = **s;
		(*s)++;

		struct tree_node *rnode = NULL;
		if ((ret = CALL_PARSER(getPow, s, &rnode))) {
			tnode_recursive_dtor(lnode, NULL);
			return PARSER_RET_STATUS(ret);
		}

		if (operator == '*') {
			struct tree_node *mnode = expr_create_operator_tnode(
				expression_operators[DERIVATOR_IDX_MULTIPLY], lnode, rnode);

			if (!mnode) {
				tnode_recursive_dtor(lnode, NULL);
				tnode_recursive_dtor(rnode, NULL);
				return PARSER_RET_STATUS(S_FAIL);
			}

			lnode = mnode;
		} else if (operator == '/') {
			struct tree_node *mnode = expr_create_operator_tnode(
				expression_operators[DERIVATOR_IDX_DIVIDE], lnode, rnode);

			if (!mnode) {
				tnode_recursive_dtor(lnode, NULL);
				tnode_recursive_dtor(rnode, NULL);
				return PARSER_RET_STATUS(S_FAIL);
			}

			lnode = mnode;
		} else {
			return PARSER_RET_STATUS(S_FAIL);
		}
	}

	*node = lnode;

	return PARSER_RET_STATUS(S_OK);
}

static int getExpression(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	int ret = 0;

	struct tree_node *lnode = NULL;
	if ((ret = CALL_PARSER(getTerm, s, &lnode))) {
		return PARSER_RET_STATUS(ret);
	}

	while (**s == '+' || **s == '-') {
		char operator = **s;
		(*s)++;

		struct tree_node *rnode = NULL;
		if ((ret = CALL_PARSER(getTerm, s, &rnode))) {
			tnode_recursive_dtor(lnode, NULL);
			return PARSER_RET_STATUS(ret);
		}

		if (operator == '+') {
			struct tree_node *mnode = expr_create_operator_tnode(
				expression_operators[DERIVATOR_IDX_PLUS], lnode, rnode);

			if (!mnode) {
				tnode_recursive_dtor(lnode, NULL);
				tnode_recursive_dtor(rnode, NULL);
				return PARSER_RET_STATUS(S_FAIL);
			}

			lnode = mnode;
		} else if (operator == '-') {
			struct tree_node *mnode = expr_create_operator_tnode(
				expression_operators[DERIVATOR_IDX_MINUS], lnode, rnode);

			if (!mnode) {
				tnode_recursive_dtor(lnode, NULL);
				tnode_recursive_dtor(rnode, NULL);
				return PARSER_RET_STATUS(S_FAIL);
			}

			lnode = mnode;
		} else {
			return PARSER_RET_STATUS(S_FAIL);
		}
	}

	*node = lnode;

	return PARSER_RET_STATUS(S_OK);
}

static int getPrimaryExpression(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	int ret = 0;

	if (**s == '(') {
		(*s)++;
		if ((ret = CALL_PARSER(getExpression, s, node))) {
			return PARSER_RET_STATUS(ret);
		}

		if (**s != ')') {
			return PARSER_RET_STATUS(S_FAIL);
		}

		(*s)++;

		return PARSER_RET_STATUS(S_OK);
	}

	ret = CALL_PARSER(getC, s, node);
	return PARSER_RET_STATUS(ret);
}

static int getTerminator(const char **s, struct tree_node **node) {
	assert (s);
	assert (node);

	if (**s == '$' || **s == '\0' || **s == '\n') {
		(*s)++;
		return PARSER_RET_STATUS(S_OK);
	}
	
	return PARSER_RET_STATUS(S_FAIL);
}

static int getG(const char **s, struct tree_node **node) {
	assert (s);
	assert (*s);
	assert (node);

	int ret = 0;

	if ((ret = CALL_PARSER(getExpression, s, node))) {
		return PARSER_RET_STATUS(ret);
	}

	return PARSER_RET_STATUS(CALL_PARSER(getTerminator, s, node));
}

static int log_str_neighborhood(const char *real_str,
			 const char *e_ptr, size_t side_len,
			 FILE *out_stream) {
	assert (real_str);
	assert (e_ptr);
	assert (out_stream);

	size_t fail_pos = (size_t)(e_ptr - real_str);
	size_t str_len = strlen(real_str);

	if (fail_pos > str_len) {
		return S_FAIL;
	}

	size_t left_logging = 0;
	if (fail_pos > side_len) {
		left_logging = fail_pos - side_len;
	}

	size_t right_logging = fail_pos + side_len + 1;
	if (right_logging > str_len) {
		right_logging = str_len;
	}

	for (size_t i = left_logging; i < right_logging; i++) {
		fprintf(out_stream, "%c", real_str[i]);
	}
	fprintf(out_stream, "\n");
	for (size_t i = left_logging; i < fail_pos; i++) {
		fprintf(out_stream, " ");
	}
	fprintf(out_stream, "^\n");

	return S_OK;
}

int expression_parse_str(char *str, struct expression *expr) {
	assert (str);
	assert (expr);

	*expr = (struct expression){0};
	if (expression_ctor(expr)) {
		return S_FAIL;
	};

	const char *s_copy = str;

	if (pvector_init(&var_names, sizeof(struct expression_variable))) {
		expression_dtor(expr);
		return S_FAIL;
	}

	if (pvector_set_element_destructor(&var_names, var_destructor)) {
		pvector_destroy(&var_names);
		expression_dtor(expr);
		return S_FAIL;
	}

	if (CALL_PARSER(getG, &s_copy, &expr->tree.root)) {
		size_t fail_pos = (size_t)(s_copy - str); 

		eprintf("\nExpression parsing failed in position %zu:\n", fail_pos);

		log_str_neighborhood(str, s_copy, 10, stdout);

		expression_dtor(expr);
		pvector_destroy(&var_names);

		return S_FAIL;
	}

	expr->variables = var_names;
	var_names = (struct pvector){0};

	return S_OK;
}


int expression_parse_file(const char *filename, struct expression *expr) {
	assert (filename);
	assert (expr);

	char *bufptr = NULL;
	size_t read_bytes = 0;

	int ret = 0;
	if ((ret = read_file(filename, &bufptr, &read_bytes))) {
		return ret;
	}

	ret = expression_parse_str(bufptr, expr);

	free(bufptr);

	return ret;
}
