#include "types.h"

int getN(const char **s, int *n) {
	*n = 0;

	const char *scopy = *s;

	while ('0' <= **s && **s <= '9') {
		*n = *n * 10 + (**s - '0');
		(*s)++;
	}

	if (*s != scopy) {
		return S_OK;
	}

	return S_FAIL;
}

int getP(const char **s, int *n);

int getT(const char **s, int *n) {
	int ret = 0;

	if ((ret = getP(s, n))) {
		return ret;
	}

	while (**s == '*' || **s == '/') {
		char operator = **s;
		(*s)++;

		int val2 = 0;
		if ((ret = getP(s, &val2))) {
			return ret;
		}

		if (operator == '*') {
			*n *= val2;
		} else if (operator == '/') {

			if (val2 == 0) {
				log_error("division by zero");
				return S_FAIL;
			}

			*n /= val2;
		} else {
			return S_FAIL;
		}
	}

	return S_OK;
}

int getE(const char **s, int *n) {
	int ret = 0;

	if ((ret = getT(s, n))) {
		return ret;
	}

	while (**s == '+' || **s == '-') {
		char operator = **s;
		(*s)++;

		int val2 = 0;
		if ((ret = getT(s, &val2))) {
			return ret;
		}

		if (operator == '+') {
			*n += val2;
		} else if (operator == '-') {
			*n -= val2;
		} else {
			return S_FAIL;
		}
	}

	return S_OK;
}

int getP(const char **s, int *n) {
	int ret = 0;

	if (**s == '(') {
		(*s)++;
		if ((ret = getE(s, n))) {
			return ret;
		}

		if (**s != ')') {
			return S_FAIL;
		}

		(*s)++;

		return S_OK;
	}

	return getN(s, n);
}

int getTerm(const char **s, int *n) {
	if (**s == '$') {
		(*s)++;
		return S_OK;
	}
	
	return S_FAIL;
}

int getG(const char *s, int *n) {
	int ret = 0;

	if ((ret = getE(&s, n))) {
		return ret;
	}

	if ((ret = getTerm(&s, n))) {
		return ret;
	}

	return S_OK;
}

int evaluate_expr_str(char *str, size_t str_sz) {
	eprintf("meow\n");
	return 0;
}

int main() {
	char str[] = "1300/(500/10+100*6)$";
	int n = 0;
	if (getG(str, &n)) {
		log_error("error");
		return 1;
	}

	printf("%d\n", n);
	// size_t str_sz = sizeof(str) - 1;
	// evaluate_expr_str(str, str_sz);
	return 0;
}
