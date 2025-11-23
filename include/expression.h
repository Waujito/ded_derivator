#ifndef AKINATOR_H
#define AKINATOR_H

#include "tree.h"

#ifdef __cplusplus
extern "C" {
#endif


struct expression {
	struct tree tree;
};

int expression_ctor(struct expression *expr);
int expression_dtor(struct expression *expr);

int expression_load(struct expression *expr, const char *filename);
int expression_store(struct expression *expr, const char *filename);

int derivate(struct expression *expr, struct expression *derivative);
struct tree_node* expression_derive(struct tree_node *node);

struct expression_operator {
	const char *name;
	struct tree_node* (*deriver)(struct tree_node *node);
	const char *latex_name;
};

// Update the operators array with derivative functions
extern const struct expression_operator expression_operators[];

enum {
	DERIVATOR_F_NUMBER	= 0x01,
	DERIVATOR_F_OPERATOR	= 0x11,
};

/**
 * implements value_deserializer
 */
DSError_t expression_deserializer(tree_dtype *value, const char *str);
/**
 * implements value_serializer
 */
DSError_t expression_serializer(tree_dtype value, FILE *out_stream);

DSError_t expression_to_latex(struct expression *expr, FILE *out_stream);

#ifdef __cplusplus
}
#endif

#endif /* AKINATOR_H */
