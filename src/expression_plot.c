#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "tree.h"

#include "expression.h"

static const char *const tmp_base_filename = "/tmp/derivator.XXXXXX";

static double evaluate_tnode_at_x(struct expression *expr, struct tree_node *tnode, double x) {
	assert (expr);
	assert (tnode);

	struct expression_variable *ev = NULL;
	double original_value = 0;

	if (pvector_get(&expr->variables, expr->differentiating_variable, 
						(void **)&ev)) {
		return NAN;	
	}

	original_value = ev->value;
	ev->value = x;

	double result = 0;
	if (tnode_evaluate(expr, tnode, &result) != S_OK) {
		result = NAN;
	}

	ev->value = original_value;

	return result;
}

#define GNUPLOT_MIN_POINTS (1000)

int expression_tnode_plot_pts(struct expression *expr, struct tree_node *tnode,
			FILE *out_file, double x_min, double x_max, int points) {
	assert(expr);
	assert(tnode);
	assert(out_file);
    
	if (points <= 1) {
		return S_FAIL;
	}

	fprintf(out_file, "\"<echo '");

	double step = (x_max - x_min) / (points - 1);
	for (int i = 0; i < points; i++) {
		double x = x_min + i * step;
		double y = evaluate_tnode_at_x(expr, tnode, x);

		if (!isnan(y) && !isinf(y)) {
			fprintf(out_file, "%f %f\\n", x, y);
		}
	}

	fprintf(out_file, "'\"");

	return S_OK;
}

int expression_derivative_plot(struct expression *expr, int nth_derivative) {
	struct expression_variable *ev = NULL;
	if (pvector_get(&expr->variables, expr->differentiating_variable, (void **)&ev)) {
		return S_FAIL;
	}
	double approx_pt = ev->value;

	struct tree *derivative_tree = NULL;
	if (nth_derivative == 0) {
		derivative_tree = &expr->tree;
	} else if (nth_derivative > 0) {
		if (pvector_get(&expr->derivatives, (size_t)nth_derivative - 1,
				(void **)&derivative_tree)) {
			return S_FAIL;
		}
	}

	char *tmp_filename = strdup(tmp_base_filename);
	close(mkstemp(tmp_filename));
	pvector_push_back(&expr->graph_files, &tmp_filename);

	double x_min = approx_pt - 10;
	double x_max = approx_pt + 10;

	FILE *gnuplot_stream = popen("gnuplot &>/dev/null", "w");
	fprintf(gnuplot_stream, "set terminal pngcairo enhanced font 'Arial,12'\n"
		"set output '%s.png'\n"
		"set grid\n"
		"set xlabel 'x'\n"
		"set ylabel 'y'\n"
		"set key top right\n",
		tmp_filename
	);	

	fprintf(gnuplot_stream, "plot ");
	expression_tnode_plot_pts(expr, derivative_tree->root,
			       gnuplot_stream, x_min, x_max, GNUPLOT_MIN_POINTS);
	fprintf(gnuplot_stream, " with lines lw 1 title ''\n");

	if (pclose(gnuplot_stream)) {
		log_error("gnuplot_stream");
	}

	latex_print_expression_function(expr, nth_derivative, expr->latex_file);
	latex_draw_image(expr->latex_file, tmp_filename);

	return S_OK;
}


int expression_taylor_plot(struct expression *expr, struct expression *taylor_expr) {

	struct expression_variable *ev = NULL;
	if (pvector_get(&expr->variables, expr->differentiating_variable, (void **)&ev)) {
		return S_FAIL;
	}
	double approx_pt = ev->value;

	char *tmp_filename = strdup(tmp_base_filename);
	close(mkstemp(tmp_filename));
	pvector_push_back(&expr->graph_files, &tmp_filename);

	double x_min = approx_pt - 2;
	double x_max = approx_pt + 2;

	FILE *gnuplot_stream = popen("gnuplot &>/dev/null", "w");
	fprintf(gnuplot_stream, "set terminal pngcairo enhanced font 'Arial,12'\n"
		"set output '%s.png'\n"
		"set grid\n"
		"set xlabel 'x'\n"
		"set ylabel 'y'\n"
		"set yrange [-2:2]\n"
		"set key top right\n",
		tmp_filename
	);	

	fprintf(gnuplot_stream, "plot ");
	expression_tnode_plot_pts(expr, expr->tree.root,
			       gnuplot_stream, x_min, x_max, GNUPLOT_MIN_POINTS);
	fprintf(gnuplot_stream, " with lines lw 1 title 'Real function', ");

	expression_tnode_plot_pts(taylor_expr, taylor_expr->tree.root,
			       gnuplot_stream, x_min, x_max, GNUPLOT_MIN_POINTS);
	fprintf(gnuplot_stream, " with lines lw 2 title 'Taylor approximation', ");


	double approx_y = evaluate_tnode_at_x(expr, expr->tree.root, approx_pt);
	fprintf(gnuplot_stream, " \"<echo '%f %f'\" with points pt 3 ps 2 lc rgb 'red' title 'Single Point', ", approx_pt, approx_y);

	struct tree *first_derivative = NULL;
	if (!pvector_get(&expr->derivatives, 0, (void **)&first_derivative)) {
		double k = evaluate_tnode_at_x(expr, first_derivative->root, approx_pt);
		// y = kx + b
		// b = y - kx
		double b = approx_y - approx_pt * k;

		fprintf(gnuplot_stream, " (%f)*x+(%f) with lines lw 2 title 'Tangent'\n",k, b);
	}

	if (pclose(gnuplot_stream)) {
		log_error("gnuplot_stream");
	}

	latex_draw_image(expr->latex_file, tmp_filename);

	return S_OK;
}

int expression_tnode_plot(struct expression *expr, struct tree_node *tnode,
			const char *filename, double x_min, double x_max) {
	assert(expr);
	assert(tnode);
	assert(filename);

	FILE *gnuplot_stream = popen("gnuplot &>/dev/null", "w");
	fprintf(gnuplot_stream, "set terminal pngcairo enhanced font 'Arial,12'\n"
		"set output '%s.png'\n"
		"set grid\n"
		"set xlabel 'x'\n"
		"set ylabel 'y'\n"
		"set key top right\n",
		filename
	 );

	fprintf(gnuplot_stream, "plot ");
	expression_tnode_plot_pts(expr, tnode, 
			       gnuplot_stream, x_min, x_max, GNUPLOT_MIN_POINTS);
	fprintf(gnuplot_stream, " with lines lw 1 title ''\n");

	if (pclose(gnuplot_stream)) {
		log_error("gnuplot_stream");
	}

	return S_OK;
}
