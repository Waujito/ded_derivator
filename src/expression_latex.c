#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

#include "expression.h"

static DSError_t tnode_to_latex(struct tree_node *node, FILE *out_stream) {
	assert (node);
	assert (out_stream);

	if (node->right && !node->left) {
		return DS_INVALID_ARG;
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		struct expression_operator *expr_op = node->value.ptr;
		fprintf(out_stream, "%s", expr_op->latex_name);

		if (node->left) {
			fprintf(out_stream, "{");
			tnode_to_latex(node->left, out_stream);
			fprintf(out_stream, "}");
		}

		if (node->right) {
			fprintf(out_stream, "{");
			tnode_to_latex(node->right, out_stream);
			fprintf(out_stream, "}");
		}
	} else {
		fprintf(out_stream, "%g", node->value.fnum);
	}

	return DS_OK;
}

static const char *latex_command_header =
"\\newcommand{\\edplus}[2]{(#1 \\mathbin{+} #2)}\n"
"\\newcommand{\\edminus}[2]{(#1 \\mathbin{-} #2)}\n"
"\\newcommand{\\edmultiply}[2]{(#1 \\cdot #2)}\n"
"\\newcommand{\\eddivide}[2]{\\frac{#1}{#2}}\n"
"\\newcommand{\\edpower}[2]{{#1}^{#2}}\n"
"\\newcommand{\\edln}[1]{\\mathop{\\mathrm{ln}}\\left(#1\\right)}\n"
"\\newcommand{\\edx}{\\mathord{x}}\n";


DSError_t expression_to_latex(struct expression *expr, FILE *out_stream) {
	assert (expr);
	assert (out_stream);

	fprintf(out_stream, "\\begin{center}\n");
	fprintf(out_stream, "%s", latex_command_header);
	fprintf(out_stream, "\\begin{math}\n");
	DSError_t ret = tnode_to_latex(expr->tree.root, out_stream);
	fprintf(out_stream, "\n");
	fprintf(out_stream, "\\end{math}\n");
	fprintf(out_stream, "\\end{center}\n");

	return ret;
}


DSError_t write_latex_header(FILE *latex_file) {
	assert (latex_file);

	fprintf(latex_file, 
		"\\documentclass[12pt]{article}\n\\begin{document}");

	return DS_OK;
}

DSError_t write_latex_footer(FILE *latex_file) {
	assert (latex_file);

	fprintf(latex_file, "\\end{document}");

	return DS_OK;
}
