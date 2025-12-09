#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

#include "expression.h"

DSError_t tnode_to_latex(struct expression *expr,
				struct tree_node *node, FILE *out_stream) {
	assert (expr);
	assert (node);
	assert (out_stream);

	if (node->right && !node->left) {
		return DS_INVALID_ARG;
	}

	if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_OPERATOR) {
		struct expression_operator *expr_op = node->value.ptr;
		fprintf(out_stream, "%s", expr_op->latex_name);

		if (node->left) {
			int brackets = 0;
			if ((node->left->value.flags & DERIVATOR_F_OPERATOR)
					== DERIVATOR_F_OPERATOR) {
				struct expression_operator *inl_op =  node->left->value.ptr;
				if (inl_op->priority > expr_op->priority) {
					brackets = 1;
				}
			}

			fprintf(out_stream, "{");
			if (brackets) {
				fprintf(out_stream, "(");
			}
			tnode_to_latex(expr, node->left, out_stream);
			if (brackets) {
				fprintf(out_stream, ")");
			}
			fprintf(out_stream, "}");
		}

		if (node->right) {
			int brackets = 0;
			if ((node->right->value.flags & DERIVATOR_F_OPERATOR)
					== DERIVATOR_F_OPERATOR) {
				struct expression_operator *inl_op =  node->right->value.ptr;
				if (inl_op->priority > expr_op->priority) {
					brackets = 1;
				}
			}

			fprintf(out_stream, "{");
			if (brackets) {
				fprintf(out_stream, "(");
			}
			tnode_to_latex(expr, node->right, out_stream);
			if (brackets) {
				fprintf(out_stream, ")");
			}
			fprintf(out_stream, "}");
		}
	} else if ((node->value.flags & DERIVATOR_F_OPERATOR) == DERIVATOR_F_VARIABLE) {
		struct expression_variable *ev = NULL;
		pvector_get(&expr->variables, node->value.varidx, (void **)&ev);

		fprintf(out_stream, "\\textit{%s}", ev->name);
	} else {
		fprintf(out_stream, "%g", node->value.fnum);
	}

	return DS_OK;
}

static const char *latex_command_header =
"\\newcommand{\\edplus}[2]{#1 \\mathbin{+} #2}\n"
"\\newcommand{\\edminus}[2]{#1 \\mathbin{-} #2}\n"
"\\newcommand{\\edmultiply}[2]{#1 \\cdot #2}\n"
"\\newcommand{\\eddivide}[2]{\\frac{#1}{#2}}\n"
"\\newcommand{\\edpower}[2]{{#1}^{#2}}\n"
"\\newcommand{\\edln}[1]{\\mathop{\\mathrm{ln}} #1}\n"
"\\newcommand{\\edcos}[1]{\\mathop{\\mathrm{cos}} #1}\n"
"\\newcommand{\\edsin}[1]{\\mathop{\\mathrm{sin}} #1}\n"
"\\newcommand{\\edsmallo}[1]{\\mathop{\\mathrm{o}} (#1)}\n";

DSError_t tnode_write_latex_eq(struct expression *expr, struct tree_node *tnode,
			       FILE *out_stream) {
	assert (expr);
	assert (tnode);
	assert (out_stream);

	fprintf(out_stream, "\\begin{equation}\n");
	DSError_t ret = tnode_to_latex(expr, tnode, out_stream);
	fprintf(out_stream, "\n");
	fprintf(out_stream, "\\end{equation}\n");
	fprintf(out_stream, "\n");

	return ret;
}

DSError_t latex_print_expression_function(struct expression *expr, int nth_derivative,
					  FILE *out_stream) {
	assert (expr);
	assert (out_stream);

	fprintf(out_stream, "\\begin{equation}\n");
	fprintf(out_stream, "f");
	if (nth_derivative > 3) {
		fprintf(out_stream, "^{(%d)}", nth_derivative);
	} else {
		for (int i = 0; i < nth_derivative; i++) {
			fprintf(out_stream, "'");
		}
	}
	fprintf(out_stream, "(x) = ");

	struct tree *derivative_tree = NULL;
	if (nth_derivative == 0) {
		derivative_tree = &expr->tree;
	} else if (nth_derivative > 0) {
		if (pvector_get(&expr->derivatives, (size_t)nth_derivative - 1,
				(void **)&derivative_tree)) {
			return DS_ALLOCATION;
		}
	}

	DSError_t ret = tnode_to_latex(expr, derivative_tree->root, out_stream);

	fprintf(out_stream, "\n");
	fprintf(out_stream, "\\end{equation}\n");
	fprintf(out_stream, "\n");

	return ret;
}

DSError_t expression_to_latex(struct expression *expr, FILE *out_stream) {
	assert (expr);
	assert (out_stream);

	DSError_t ret = tnode_write_latex_eq(expr, expr->tree.root, out_stream);

	return ret;
}

DSError_t latex_draw_image(FILE *latex_file, const char *image_filename) {
	assert (latex_file);
	assert (image_filename);

	fprintf(latex_file,
		"\\begin{figure}[H]\n"
		"\\centering\n"
		"\\includegraphics[width=0.8\\textwidth]{%s}\n"
		"\\end{figure}\n",
		
		image_filename
	);

	return DS_OK;
}


DSError_t write_latex_header(FILE *latex_file) {
	assert (latex_file);

	fprintf(latex_file, "\\documentclass[12pt]{article}\n"
		"\\usepackage[hidelinks]{hyperref}\n"
		"\\usepackage{blindtext}\n"
		"\\usepackage{titlesec}\n"
		"\\usepackage{float}\n"
		"\\title{I'm Derivating}\n"
		"\\author{Vadim Vetrov}\n"
		"\\date{\\today}\n"
		"\\usepackage{graphicx}\n"
	);
	fprintf(latex_file, "%s", latex_command_header);

	fprintf(latex_file, "\\begin{document}\n");
	fprintf(latex_file, "\\maketitle\n");
	fprintf(latex_file, "\\tableofcontents\n");

	return DS_OK;
}

DSError_t write_latex_footer(FILE *latex_file) {
	assert (latex_file);

	fprintf(latex_file, 
		"\\section{Citiation}\n"
		"\\begin{enumerate}\n"
		"\\item Derivator on GitHub: \\url{https://github.com/Waujito/ded_derivator} \n"
		"\\item Derivating (Levitating Calculus Parody) \\url{https://www.youtube.com/watch?v=2kyQ1YuzOBM}\n"
		"\\end{enumerate}\n"
	);
	
	fprintf(latex_file, "\\end{document}\n");

	return DS_OK;
}
