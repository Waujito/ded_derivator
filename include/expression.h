#ifndef AKINATOR_H
#define AKINATOR_H

#include "tree.h"
#include "pvector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct expression_variable {
	char *name;
	int value;
};

struct expression {
	struct tree tree;
	struct pvector variables;
	size_t differentiating_variable;
};

int expression_ctor(struct expression *expr);
int expression_dtor(struct expression *expr);

int expression_validate(struct expression *expr);
int expression_load(struct expression *expr, const char *filename);
int expression_store(struct expression *expr, const char *filename);

int expression_derive(struct expression *expr, struct expression *derivative);

int expression_derive_nth(struct expression *expr,
			  struct expression *derivative,
			  int nth);

int expression_simplify(struct expression *expr, struct expression *derivative);

int tnode_evaluate(struct expression *expr,
				   struct tree_node *node, double *fnum);
int expression_evaluate(struct expression *expr, double *fnum);

int expression_parse_str(char *str, struct expression *expr);
int expression_parse_file(const char *filename, struct expression *expr);

// DSError_t expression_to_latex(struct expression *expr, struct expression *);

enum expression_indexes {
	DERIVATOR_IDX_PLUS,
	DERIVATOR_IDX_MINUS,
	DERIVATOR_IDX_MULTIPLY,
	DERIVATOR_IDX_DIVIDE,
	DERIVATOR_IDX_POW,
	DERIVATOR_IDX_LN,
	DERIVATOR_IDX_SIN,
	DERIVATOR_IDX_COS,
};

struct expression_operator {
	enum expression_indexes idx;
	const char *name;
	struct tree_node* (*deriver)(struct expression *expr, struct tree_node *node);
	int (*evaluator)(struct expression *expr, struct tree_node *node, double *fnum);
	const char *latex_name;
};

// Update the operators array with derivative functions
// extern const struct expression_operator expression_operators[];

enum {
	DERIVATOR_F_NUMBER	= 0x1,
	DERIVATOR_F_VARIABLE	= 0x2,
	DERIVATOR_F_OPERATOR	= 0x7,
	DERIVATOR_F_CONSTANT	= 1 << 3,
};

/**
 * implements value_deserializer
 */
DSError_t expression_deserializer(tree_dtype *value, const char *str);
DSError_t expression_deserializer_endp(tree_dtype *value, const char *str, const char **endptr);
/**
 * implements value_serializer
 */
DSError_t expression_serializer(tree_dtype value, FILE *out_stream);

DSError_t expression_to_latex(struct expression *expr, FILE *out_stream);
DSError_t write_latex_header(FILE *latex_file);
DSError_t write_latex_footer(FILE *latex_file);

struct tree_node *expr_create_number_tnode(double fnum);
struct tree_node *expr_create_variable_tnode(size_t idx);
struct tree_node *expr_create_operator_tnode(const struct expression_operator *op, 
                                              struct tree_node *left, 
                                              struct tree_node *right);
struct tree_node *expr_copy_tnode(struct expression *expr, struct tree_node *original);

#define DECLARE_EXPERSSION_OP(_idx, opname, opstring_name, oplatex)		\
	struct tree_node *expr_op_deriver_##opname(struct expression *expr,	\
				struct tree_node *node);			\
	int expr_op_evaluator_##opname(struct expression *expr,			\
				struct tree_node *node, double *fnum);		\
	static const struct expression_operator expr_operator_##opname = {	\
		.idx = _idx,							\
		.name = opstring_name,						\
		.deriver = expr_op_deriver_##opname,				\
		.evaluator = expr_op_evaluator_##opname,			\
		.latex_name = oplatex,						\
	}

DECLARE_EXPERSSION_OP(DERIVATOR_IDX_PLUS, addition, "+", "\\edplus");
DECLARE_EXPERSSION_OP(DERIVATOR_IDX_MINUS, subtraction, "-", "\\edminus");
DECLARE_EXPERSSION_OP(DERIVATOR_IDX_MULTIPLY, multiplication, "*", "\\edmultiply");
DECLARE_EXPERSSION_OP(DERIVATOR_IDX_DIVIDE, division, "/", "\\eddivide");
DECLARE_EXPERSSION_OP(DERIVATOR_IDX_POW, power, "^", "\\edpower");
DECLARE_EXPERSSION_OP(DERIVATOR_IDX_LN, log, "ln", "\\edln");
DECLARE_EXPERSSION_OP(DERIVATOR_IDX_SIN, sin, "sin", "\\edsin");
DECLARE_EXPERSSION_OP(DERIVATOR_IDX_COS, cos, "cos", "\\edcos");

#undef DECLARE_EXPERSSION_OP

#define REGISTER_EXPRESSION_OP(idx, opname)					\
	[idx] = &expr_operator_##opname

static const struct expression_operator *const expression_operators[] = {
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_PLUS, addition),
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_MINUS, subtraction),
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_MULTIPLY, multiplication),
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_DIVIDE, division),
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_POW, power),
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_LN, log),
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_SIN, sin),
	REGISTER_EXPRESSION_OP(DERIVATOR_IDX_COS, cos),
	NULL,
};

#ifdef __cplusplus
}
#endif

#endif /* AKINATOR_H */
