#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include "expression.h"
#include <stdbool.h>
#include <ctype.h>


int main() {
	struct expression expr = {0};
	if (expression_parse_file("expr.txt", &expr)) {
		log_error("error");
		return 1;
	}


	FILE *dump_file = fopen("tree.htm", "w");
	if (!dump_file) {
		log_error("Cannot open dump file!");
		return 1;
	}

	struct tree_dump_params dump_params = {
		.out_stream = dump_file,
		.idx = 0,
		.drawing_filename = "asdf.png",
		.serializer = expression_serializer,
	};

	tree_dump(&expr.tree, dump_params);

	FILE *latex_file = fopen("data.tex", "w");
	if (!latex_file) {
		log_error("cannot open latex file!");
		return 1;
	}
	write_latex_header(latex_file);

	fprintf(latex_file, "\\section{expression}\n");
	expression_to_latex(&expr, latex_file);

	struct expression derivative = {0};
	if (expression_derive_nth(&expr, &derivative, 10)) {
		eprintf("derivatingsodf\n");
	}

	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph1.png";
	// tree_dump(&derivative.tree, dump_params);
	fprintf(latex_file, "\\section{der full}\n");
	expression_to_latex(&derivative, latex_file);

	struct expression simpl_derivative = {0};
	expression_simplify(&derivative, &simpl_derivative);
	expression_dtor(&derivative);
	derivative = simpl_derivative;
	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph2.png";
	// tree_dump(&derivative.tree, dump_params);
	fprintf(latex_file, "\\section{der sipml}\n");
	expression_to_latex(&derivative, latex_file);
	/*

	struct expression derivative2 = {0};
	if (expression_derive(&derivative, &derivative2)) {
		eprintf("derivatingsodf\n");
	}

	fprintf(latex_file, "\\section{der2 full}\n");
	expression_to_latex(&derivative2, latex_file);

	expression_simplify(&derivative2, &simpl_derivative);
	expression_dtor(&derivative2);
	derivative2 = simpl_derivative;
	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph3.png";
	tree_dump(&derivative2.tree, dump_params);
	fprintf(latex_file, "\\section{der2 sipml}\n");
	expression_to_latex(&derivative2, latex_file);

	struct expression derivative3 = {0};
	if (expression_derive(&derivative2, &derivative3)) {
		eprintf("derivatingsodf\n");
	}
	fprintf(latex_file, "\\section{der3 full}\n");
	expression_to_latex(&derivative3, latex_file);

	expression_simplify(&derivative3, &simpl_derivative);
	expression_dtor(&derivative3);
	derivative3 = simpl_derivative;
	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph4.png";
	fprintf(latex_file, "\\section{der3 simpl}\n");
	expression_to_latex(&derivative3, latex_file);
*/


	write_latex_footer(latex_file);
	fclose(latex_file);
	fclose(dump_file);
	expression_dtor(&expr);
	expression_dtor(&derivative);
	// expression_dtor(&derivative2);
	// expression_dtor(&derivative3);

	return 0;
}

/*
int main() {
	struct expression expr = {0};
	if (expression_ctor(&expr)) {
		return 1;
	}

	FILE *latex_file = fopen("data.tex", "w");
	if (!latex_file) {
		log_error("cannot open latex file!");
		return 1;
	}
	fprintf(latex_file, "\\documentclass[12pt]{article}\n\\begin{document}");

	FILE *dump_file = fopen("tree.htm", "w");
	if (!dump_file) {
		log_error("Cannot open dump file!");
		return 1;
	}

	if (expression_load(&expr, "meow.tree")) {
		eprintf("uwu\n");
	};
	eprintf("expression loaded\n");

	struct tree_dump_params dump_params = {
		.out_stream = dump_file,
		.idx = 0,
		.drawing_filename = "asdf.png",
		.serializer = expression_serializer,
	};

	tree_dump(&expr.tree, dump_params);
	fprintf(latex_file, "\\section{expression}\n");
	expression_to_latex(&expr, latex_file);

	struct expression derivative = {0};
	if (expression_derive(&expr, &derivative)) {
		eprintf("derivatingsodf\n");
	}

	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph1.png";
	// tree_dump(&derivative.tree, dump_params);
	fprintf(latex_file, "\\section{der full}\n");
	expression_to_latex(&derivative, latex_file);

	struct expression simpl_derivative = {0};
	expression_simplify(&derivative, &simpl_derivative);
	expression_dtor(&derivative);
	derivative = simpl_derivative;
	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph2.png";
	tree_dump(&derivative.tree, dump_params);
	fprintf(latex_file, "\\section{der sipml}\n");
	expression_to_latex(&derivative, latex_file);

	struct expression derivative2 = {0};
	if (expression_derive(&derivative, &derivative2)) {
		eprintf("derivatingsodf\n");
	}

	// dump_params.idx++;
	// dump_params.drawing_filename = "tree_graph2.png";
	// tree_dump(&derivative2.tree, dump_params);
	fprintf(latex_file, "\\section{der2 full}\n");
	expression_to_latex(&derivative2, latex_file);

	expression_simplify(&derivative2, &simpl_derivative);
	expression_dtor(&derivative2);
	derivative2 = simpl_derivative;
	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph3.png";
	tree_dump(&derivative2.tree, dump_params);
	fprintf(latex_file, "\\section{der2 sipml}\n");
	expression_to_latex(&derivative2, latex_file);

	struct expression derivative3 = {0};
	if (expression_derive(&derivative2, &derivative3)) {
		eprintf("derivatingsodf\n");
	}
	fprintf(latex_file, "\\section{der3 full}\n");
	expression_to_latex(&derivative3, latex_file);

	expression_simplify(&derivative3, &simpl_derivative);
	expression_dtor(&derivative3);
	derivative3 = simpl_derivative;
	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph4.png";
	// tree_dump(&derivative3.tree, dump_params);
	fprintf(latex_file, "\\section{der3 simpl}\n");
	expression_to_latex(&derivative3, latex_file);

	// dump_params.idx++;
	// dump_params.drawing_filename = "tree_graph3.png";
	// tree_dump(&derivative3.tree, dump_params);


	fprintf(latex_file, "\\end{document}");
	fclose(latex_file);
	fclose(dump_file);
	expression_dtor(&expr);
	expression_dtor(&derivative);
	expression_dtor(&derivative2);
	expression_dtor(&derivative3);

	return 0;
}
*/
