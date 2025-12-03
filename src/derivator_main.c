#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include "expression.h"
#include <stdbool.h>
#include <ctype.h>

/*
enum {
	AKINATOR_CONTINUE,
	AKINATOR_FAIL_ACTION,
	AKINATOR_PREDICT,
	AKINATOR_LOAD,
	AKINATOR_DUMP,
	AKINATOR_STORE,
	AKINATOR_QUIT,
};

int read_akinator_action(void) {
	printf(	"Enter an action of akinator: \n"
		"[p]redict - to predict the node\n"
		"[l]oad - to load the akinator from the file\n"
		"[s]tore - to store the akinator to file\n"
		"[d]ump - to graphically dump the akinator\n"
		"[q]uit - to quit an akinator\n"
	);

	char *linebuf = NULL;
	size_t linebuf_sz = 0;
	ssize_t buflen = 0;
	if ((buflen = getline(&linebuf, &linebuf_sz, stdin)) <= 0) {
		return AKINATOR_FAIL_ACTION;
	}

	linebuf[buflen - 1] = '\0';
	buflen--;
	if (buflen > 63) {
		return AKINATOR_FAIL_ACTION;
	}
	char buf[64] = "";
	memcpy(buf, linebuf, (size_t)buflen + 1);
	free(linebuf);


	if (buflen == 1) {
		switch (buf[0]) {
			case 'p': return AKINATOR_PREDICT;
			case 'l': return AKINATOR_LOAD;
			case 's': return AKINATOR_STORE;
			case 'd': return AKINATOR_DUMP;
			case 'q': return AKINATOR_QUIT;
			default: return AKINATOR_FAIL_ACTION;
		}
	}

	if (!strcmp(buf, "predict"))
		return AKINATOR_PREDICT;
	if (!strcmp(buf, "load"))
		return AKINATOR_LOAD;
	if (!strcmp(buf, "store"))
		return AKINATOR_STORE;
	if (!strcmp(buf, "dump"))
		return AKINATOR_DUMP;
	if (!strcmp(buf, "quit"))
		return AKINATOR_QUIT;

	return AKINATOR_FAIL_ACTION;
}

int akinator_action(struct akinator *aki, struct tree_dump_params *dump_params) {
	switch (read_akinator_action()) {
		case AKINATOR_PREDICT:
			if (akinator_ask(aki)) {
				eprintf("ask\n");
			}
			break;
		case AKINATOR_LOAD:
			if (akinator_load(aki, "meow.tree")) {
				eprintf("load\n");
			}
			break;
		case AKINATOR_DUMP: {
			char buf[64] = "";
			snprintf(buf, 64, "tree_graph%d.png", dump_params->idx);
			dump_params->idx += 1;
			dump_params->drawing_filename = buf;

			tree_dump(&aki->tree, *dump_params);

			fflush(dump_params->out_stream);
			break;
		}
		case AKINATOR_STORE:
			if (akinator_store(aki, "meow.tree")) {
				eprintf("store\n");
			}
			break;
		case AKINATOR_QUIT:
			return AKINATOR_QUIT;
		default:
			return AKINATOR_FAIL_ACTION;
	}

	return AKINATOR_CONTINUE;
}
*/

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

	FILE *latex_file = popen("pdflatex &>/dev/null", "w");
	write_latex_header(latex_file);

	fprintf(latex_file, "\\section{expression}\n");
	expression_to_latex(&expr, latex_file);

	struct expression derivative = {0};
	if (expression_derive_nth(&expr, &derivative, 1)) {
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


	struct expression tailor_series = {0};

	struct expression_variable *ev;
	pvector_get(&expr.variables, 0, (void **)&ev);
	ev->value = 0;

	expression_tailor_series_nth(&expr,
				 &tailor_series, 7);

	dump_params.idx++;
	dump_params.drawing_filename = "tree_graph3.png";
	tree_dump(&tailor_series.tree, dump_params);
	fprintf(latex_file, "\\section{tailor series}\n");
	expression_to_latex(&tailor_series, latex_file);

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
	if (pclose(latex_file)) {
		eprintf("Failed to write to latex\n");
	}
	fclose(dump_file);
	expression_dtor(&expr);
	expression_dtor(&derivative);
	expression_dtor(&tailor_series);
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
