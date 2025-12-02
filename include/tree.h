#ifndef TREE_H
#define TREE_H

#include <stdint.h>

#include "data_structure.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int flags;

	union {
		void *ptr;
		size_t varidx;
		double  fnum;
	};
} tree_dtype;

typedef DSError_t (*value_deserializer)(tree_dtype *value, const char *str);
typedef DSError_t (*value_serializer)(tree_dtype value, FILE *out_stream);

struct tree_node {
	union {
		struct {
			struct tree_node *left;
			struct tree_node *right;
		};
	};
	
	tree_dtype value;
};

typedef void (*tree_node_value_dtor)(struct tree_node *node);

struct tree {
	struct tree_node *root;
	tree_node_value_dtor tree_node_dtor;
};

DSError_t tree_ctor(struct tree *tree);
DSError_t tree_set_node_value_dtor(struct tree *tree, tree_node_value_dtor vdtor);
DSError_t tree_dtor(struct tree *tree);

struct tree_node *tnode_ctor(void);
void tnode_dtor(struct tree_node *node, tree_node_value_dtor vdtor);
void tnode_recursive_dtor(struct tree_node *node, tree_node_value_dtor vdtor);

DSError_t tree_store(struct tree *tree, const char *filename, value_serializer serializer);
DSError_t tree_serialize_node(struct tree_node *node, FILE *file, value_serializer serializer);

DSError_t tree_load(struct tree *tree, const char *filename, value_deserializer deserializer);
DSError_t tree_deserialize_node(struct tree_node **node, char *buffer, size_t *pos, value_deserializer deserializer);

struct tree_dump_params {
	FILE *out_stream;
	const char *drawing_filename;
	int idx;
	value_serializer serializer;
};

DSError_t tree_dump(struct tree *tree,
		    struct tree_dump_params params);

DSError_t tree_graph_dump_dot(struct tree *tree, FILE *dot_file, value_serializer serializer);

#ifdef __cplusplus
}
#endif

#endif /* TREE_H */
