#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "hash.h"
#include "ctio.h"

#include "tree.h"
#include "data_structure.h"

DSError_t tree_ctor(struct tree *tree) {
	assert (tree);

	tree->root = NULL;
	
	return DS_OK;
}

DSError_t tree_set_node_value_dtor(struct tree *tree,
				   tree_node_value_dtor vdtor) {
	tree->tree_node_dtor = vdtor;

	return DS_OK;
}

DSError_t tree_dtor(struct tree *tree) {
	assert (tree);

	tnode_recursive_dtor(tree->root, tree->tree_node_dtor);
	tree->root = NULL;
	tree->tree_node_dtor = NULL;

	return DS_OK;
}

struct tree_node *tnode_ctor(void) {
	return (struct tree_node *)
		calloc(1, sizeof(struct tree_node));
}

void tnode_dtor(struct tree_node *node, tree_node_value_dtor vdtor) {
	assert (node);

	if (vdtor != NULL) {
		vdtor(node);
	}
	free(node);
}

void tnode_recursive_dtor(struct tree_node *node, tree_node_value_dtor vdtor) {

	if (!node) {
		return;
	}

	if (node->left) {
		tnode_recursive_dtor(node->left, vdtor);
		node->left = NULL;
	}
	if (node->right) {
		tnode_recursive_dtor(node->right, vdtor);
		node->right = NULL;
	}
	
	tnode_dtor(node, vdtor);
}

DSError_t tree_store(struct tree *tree, const char *filename, value_serializer serializer) {
	assert (tree);
	assert (filename);

	FILE *file = fopen(filename, "w");
	if (!file) {
		return DS_INVALID_ARG;
	}

	DSError_t result = tree_serialize_node(tree->root, file, serializer);
	fclose(file);

	return result;
}

DSError_t tree_serialize_node(struct tree_node *node, FILE *file, value_serializer serializer) {
	assert (file);

	DSError_t ret = DS_OK;

	if (!node) {
		if (fprintf(file, "nil") < 0) {
			return DS_ALLOCATION;
		}

		return DS_OK;
	}

	if (fprintf(file, "(\"") < 0)
		return DS_ALLOCATION;

	if ((ret = serializer(node->value, file)) != DS_OK)
		return ret;

	if (fprintf(file, "\" ") < 0)
		return DS_ALLOCATION;

	if ((ret = tree_serialize_node(node->left, file, serializer)) != DS_OK) {
		return ret;
	}

	if (fputc(' ', file) == EOF) {
		return DS_ALLOCATION;
	}

	if ((ret = tree_serialize_node(node->right, file, serializer)) != DS_OK) {
		return ret;
	}

	if (fputc(')', file) == EOF) {
		return DS_ALLOCATION;
	}

	return DS_OK;
}

DSError_t tree_load(struct tree *tree, const char *filename, value_deserializer deserializer) {
	assert (tree);
	assert (filename);
	assert (deserializer);

	FILE *file = fopen(filename, "r");
	if (!file) {
		return DS_INVALID_ARG;
	}

	ssize_t file_size = get_file_size(file);

	if (file_size <= 0) {
		fclose(file);
		return DS_INVALID_ARG;
	}

	char *buffer = calloc((size_t)file_size + 1, 1);
	if (!buffer) {
		fclose(file);
		return DS_ALLOCATION;
	}

	size_t read_size = fread(buffer, 1, (size_t)file_size, file);
	fclose(file);

	buffer[read_size] = '\0';

	DSError_t result = tree_ctor(tree);
	if (result != DS_OK) {
		free(buffer);
		return result;
	}

	size_t pos = 0;
	result = tree_deserialize_node(&tree->root, buffer, &pos, deserializer);
	if (result != DS_OK) {
		free(buffer);
		tree_dtor(tree);
		return result;
	}

	free(buffer);

	return DS_OK;
}

DSError_t tree_deserialize_node(struct tree_node **node, char *buffer, size_t *pos, value_deserializer deserializer) {
	assert (node);
	assert (buffer);
	assert (pos);
	assert (deserializer);

	while (buffer[*pos] == ' ') {
		(*pos)++;
	}

	if (strncmp(buffer + *pos, "nil", 3) == 0) {
		*node = NULL;
		*pos += 3;
		return DS_OK;
	}

	if (buffer[*pos] != '(') {
		return DS_INVALID_ARG;
	}

	(*pos)++;

	if (buffer[*pos] != '"') {
		return DS_INVALID_ARG;
	}

	(*pos)++;

	char *value_start = buffer + *pos;
	char *value_end = strchr(value_start, '"');
	if (!value_end) {
		return DS_INVALID_ARG;
	}

	*value_end = '\0';
	*pos = (size_t)(value_end - buffer + 1);

	*node = tnode_ctor();
	if (!*node) {
		*value_end = '"';
		return DS_INVALID_ARG;
	}

	DSError_t ret = DS_OK;
	if ((ret = deserializer(&((*node)->value), value_start)) != DS_OK) {
		*value_end = '"';
		tnode_recursive_dtor(*node, NULL);
		*node = NULL;
		return ret;
	}

	if ((ret = tree_deserialize_node(&(*node)->left, buffer, pos, deserializer)) != DS_OK) {
		*value_end = '"';
		tnode_recursive_dtor(*node, NULL);
		*node = NULL;
		return ret;
	}

	if ((ret = tree_deserialize_node(&(*node)->right, buffer, pos, deserializer)) != DS_OK) {
		*value_end = '"';
		tnode_recursive_dtor(*node, NULL);
		*node = NULL;
		return ret;
	}

	if (buffer[*pos] != ')') {
		*value_end = '"';
		tnode_recursive_dtor(*node, NULL);
		*node = NULL;
		return ret;
	}

	(*pos)++;

	return DS_OK;
}

static DSError_t dump_draw_dot(struct tree *tree, const char *drawing_filename,
			       value_serializer serializer);

static DSError_t dump_elements(struct tree *tree,
			       struct tree_dump_params *params) {
#define DUMP_LOG(...) fprintf(params->out_stream, __VA_ARGS__)

	assert (tree);
	assert (params);
	assert (params->out_stream);

	if (!tree->root) {
		return DS_INVALID_STATE;
	}

#undef DUMP_LOG

	return DS_OK;
}

DSError_t tree_dump(struct tree *tree,
		    struct tree_dump_params params) {
#define DUMP_LOG(...) fprintf(params.out_stream, __VA_ARGS__)

	assert (tree);

	if (!params.out_stream) {
		params.out_stream = stderr;
	}

	DSError_t ret = DS_OK;

	DUMP_LOG("<pre>\n");

	DUMP_LOG("<h3>Tree dump:</h3>\n");
	DUMP_LOG("\tRoot ptr: %p\n", tree->root);

	// DSError_t verifier_verdict = tree_verify(tree);
	// DUMP_LOG("\n\tVerifier verdict: <0x%x> \n\t\t", verifier_verdict);
	// fprint_DSError(params.out_stream, verifier_verdict);

	DUMP_LOG("\n\n");

	struct tree_node *root = tree->root;
	if (!root) {
		DUMP_LOG("\t[EMPTY TREE]\n");
	} else {
		_CT_CHECKED(dump_elements(tree, &params));
	}

	if (params.drawing_filename) {
		_CT_CHECKED(dump_draw_dot(tree, params.drawing_filename, params.serializer));
		DUMP_LOG("<img src=\"%s\" />\n", params.drawing_filename);
	}

	
_CT_EXIT_POINT:
	DUMP_LOG("</pre>\n");

	if (ret) {
		DUMP_LOG("<h3>DUMP ERROR!</h3>\n");
	}

#undef DUMP_LOG

	return DS_OK;
}

static DSError_t dump_draw_dot(struct tree *tree, const char *drawing_filename,
			       value_serializer serializer) {
	#define DOT_PREFIX "dot -Tpng -o %s"

	DSError_t ret = 0;

	size_t strsize = strlen(DOT_PREFIX) + strlen(drawing_filename);
	FILE *dot_file = NULL;
	int sn_written = 0;

	char *ct_string = (char *)calloc(strsize, sizeof(char));

	if (!ct_string) {
		_CT_CHECKED(DS_ALLOCATION);
	}

	sn_written = snprintf(ct_string, strsize, DOT_PREFIX, drawing_filename);

	if (sn_written < 0 || sn_written > (int)strsize) {
		_CT_CHECKED(DS_ALLOCATION);
	}


	dot_file = popen(ct_string, "w");
	if (!dot_file) {
		_CT_CHECKED(DS_ALLOCATION);
	}

	_CT_CHECKED(tree_graph_dump_dot(tree, dot_file, serializer));	

_CT_EXIT_POINT:
	if (dot_file && pclose(dot_file)) {
		ret |= DS_INVALID_STATE;
	}
	free(ct_string);
	return ret;
}

static uint32_t tree_pointer_hash(void *ptr) {
	uint32_t hsh = hash_crc32((uint8_t *)&ptr, sizeof(ptr));
	hsh >>= 8;

	uint32_t ncolor = 0x0;

	ncolor |= ((hsh & 0xFF) / 2 + 0x80);
	ncolor |= (((hsh >> 8) & 0xFF) / 2 + 0x80) << 8;
	ncolor |= (((hsh >> 16) & 0xFF) / 2 + 0x80) << 16;

	return ncolor;
}

static uint32_t inverse_color(uint32_t color) {
	uint32_t ncolor = 0x00;

	return ncolor;
}

static DSError_t tree_dump_node(struct tree_node *node, size_t *node_idx,
				FILE *dot_file, value_serializer serializer) {
	assert (node);
	assert (node_idx);

	DSError_t ret = DS_OK;

#define DOT_PRINTF(...) fprintf(dot_file, __VA_ARGS__)

	size_t cnode_idx = *node_idx;

	const char *box_color = "lightgreen";

	if (cnode_idx == 0) {
		box_color = "lightblue";
	}

	DOT_PRINTF("node%zu [label=<"
		"<TABLE><TR><TD>value=", cnode_idx);

	if (serializer) {
		serializer(node->value, dot_file);
	} else {
		DOT_PRINTF("snum: %ld, ptr: %p", node->value.snum, node->value.ptr);
	}

	DOT_PRINTF("</TD></TR>"
		"<TR><TD BGCOLOR=\"#%06x\"><FONT  COLOR=\"#%06x\">self=%p</FONT></TD></TR>"
		"<TR><TD BGCOLOR=\"#%06x\"><FONT  COLOR=\"#%06x\">left=%p</FONT></TD></TR>"
		"<TR><TD BGCOLOR=\"#%06x\"><FONT  COLOR=\"#%06x\">right=%p</FONT></TD></TR>"
		"</TABLE>>, fillcolor=\"%s\", shape=Mrecord];\n", 
		tree_pointer_hash(node), inverse_color(tree_pointer_hash(node)), node,
		tree_pointer_hash(node->left), inverse_color(tree_pointer_hash(node->left)), node->left,
		tree_pointer_hash(node->right), inverse_color(tree_pointer_hash(node->right)), node->right,
		box_color
	);

	(*node_idx)++;

	if (node->left) {
		size_t lnode_idx = *node_idx;

		DOT_PRINTF("node%zu -> node%zu [color=blue];", cnode_idx, lnode_idx);

		_CT_CHECKED(tree_dump_node(node->left, node_idx, dot_file, serializer));
	}

	if (node->right) {
		size_t rnode_idx = *node_idx;

		DOT_PRINTF("node%zu -> node%zu [color=red];", cnode_idx, rnode_idx);

		_CT_CHECKED(tree_dump_node(node->right, node_idx, dot_file, serializer));
	}

#undef DOT_PRINTF

_CT_EXIT_POINT:
	return ret;
}

DSError_t tree_graph_dump_dot(struct tree *tree, FILE *dot_file, value_serializer serializer) {
	assert(tree);


#define DOT_PRINTF(...) fprintf(dot_file, __VA_ARGS__)

	DOT_PRINTF("digraph LinkedList {\n");
	DOT_PRINTF("rankdir=LC;\n");
	DOT_PRINTF("node [shape=tripleoctagon, style=filled, fillcolor=red];\n");
	DOT_PRINTF("edge [shape=inv, arrowsize=1.0];\n\n");

	if (tree->root) {
		size_t node_idx = 0;
		tree_dump_node(tree->root, &node_idx, dot_file, serializer);
	}

	DOT_PRINTF("}\n");

#undef DOT_PRINTF
    
	return DS_OK;
}
