#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum RegexType {
	REGX_SEQ,
	REGX_CHOICE,
	REGX_CHAR,
	REGX_ANY,
	REGX_MANY,
	REGX_CLASS,
} RegexType;

typedef struct CharClass {
	size_t len;
	char (*ranges
	)[2]; // array of 2-element arrays representing beg and end of range
} CharClass;

typedef struct Regex {
	RegexType type;
	union {
		struct {
			struct Regex* fst;
			struct Regex* snd;
		} seq;
		struct {
			struct Regex* fst;
			struct Regex* snd;
		} choice;
		char ch;
		struct Regex* any;
		struct Regex* many;
		CharClass klass;
	};
} Regex;

bool regex_node_match(char* buf, size_t buf_len, Regex rx, size_t* read) {
	switch (rx.type) {
		case REGX_SEQ: {
			size_t fst = 0;
			size_t snd = 0;

			bool suc1 = regex_node_match(buf, buf_len, *rx.seq.fst, &fst);
			if (!suc1) return 0;
			bool suc2 = regex_node_match(buf + fst, buf_len - fst, *rx.seq.snd, &snd);
			if (!suc2) return 0;
			*read = fst + snd;
			return true;
			break;
		}
		case REGX_CHOICE: {
			size_t fst = 0;
			bool suc1 = regex_node_match(buf, buf_len, *rx.choice.fst, &fst);
			if (suc1) {
				*read = fst;
				return true;
			}

			size_t snd = 0;
			bool suc2 = regex_node_match(buf, buf_len, *rx.choice.snd, &snd);
			if (suc2) {
				*read = snd;
				return true;
			}

			return false;
		}
		case REGX_CHAR: {
			if (buf[0] == rx.ch && buf_len > 0) {
				*read = 1;
				return true;
			}

			return false;
		}
		case REGX_ANY: {
			size_t acc = 0;
			size_t acc2 = 0;
			while (regex_node_match(buf + acc, buf_len - acc, *rx.any, &acc2)) {
				acc += acc2;
			}
			*read = acc;
			return true;
		}
		case REGX_MANY: {
			size_t acc = 0;
			size_t acc2 = 0;
			bool suc = false;
			bool suc1 = false;
			while ((suc1 = regex_node_match(buf + acc, buf_len - acc, *rx.any, &acc2))
			) {
				acc += acc2;
				suc = suc || suc1;
			}
			if (suc) {
				*read = acc;
				return true;
			}

			return false;
		}
		case REGX_CLASS: {
			for (size_t i = 0; i < rx.klass.len; i++) {
				if (rx.klass.ranges[i][1] == '\0') {
					if (buf[0] == rx.klass.ranges[i][0]) {
						*read = 1;
						return true;
					}
				} else {
					if (rx.klass.ranges[i][0] <= buf[0] && buf[0] <= rx.klass.ranges[i][1]) {
						*read = 1;
						return true;
					}
				}
			}
			return false;
		}
	}
	assert(false);
}

bool regex_match(const char* buf, size_t len, Regex* rx, size_t* read) {
	return regex_node_match((char*)buf, len, *rx, read);
}

Regex* rx_char(char ch) {
	Regex* rx = malloc(sizeof(Regex));
	rx->type = REGX_CHAR;
	rx->ch = ch;
	return rx;
}

Regex* rx_any(Regex* any) {
	Regex* rx = malloc(sizeof(Regex));
	rx->type = REGX_ANY;
	rx->any = any;
	return rx;
}

Regex* rx_many(Regex* many) {
	Regex* rx = malloc(sizeof(Regex));
	rx->type = REGX_MANY;
	rx->many = many;
	return rx;
}

Regex* rx_choice(Regex* fst, Regex* snd) {
	Regex* rx = malloc(sizeof(Regex));
	rx->type = REGX_CHOICE;
	rx->choice.fst = fst;
	rx->choice.snd = snd;
	return rx;
}

Regex* rx_seq(Regex* fst, Regex* snd) {
	Regex* rx = malloc(sizeof(Regex));
	rx->type = REGX_SEQ;
	rx->seq.fst = fst;
	rx->seq.snd = snd;
	return rx;
}

Regex* rx_klass(size_t len, char (*ranges)[2]) {
	Regex* rx = malloc(sizeof(Regex));
	rx->type = REGX_CLASS;
	rx->klass.len = len;
	rx->klass.ranges = ranges;
	return rx;
}

Regex* parse_klass(char* str, size_t* str_len) {
	str = str + 1;
	*str_len -= 1;

	CharClass klass;
	klass.len = 0;
	klass.ranges = malloc(sizeof(char[2]) * 16); // FIXME hardcoded

	while (*str_len > 0) {
		if (*str_len >= 3 && str[1] == '-') {
			klass.ranges[klass.len][0] = str[0];
			klass.ranges[klass.len][1] = str[2];
			klass.len++;
			*str_len -= 3;
			str = str + 3;
		} else {
			klass.ranges[klass.len][0] = str[0];
			klass.ranges[klass.len][1] = '\0';
			klass.len++;
			*str_len -= 1;
			str = str + 1;
		}
	}

	return rx_klass(klass.len, klass.ranges);
}

Regex* parse_regex(char* str, size_t* str_len) {
	Regex* res = NULL;
	while (*str_len > 0) {
		if (*str == '(') {
			size_t i = 1;
			size_t j = 1;
			while (j != 0) {
				if (str[i] == '(')
					j++;
				else if (str[i] == ')')
					j--;
				i++;
			}
			size_t len = i - 2;
			if (!res) {
				res = parse_regex(str + 1, &len);
			} else
				res = rx_seq(res, parse_regex(str + 1, &len));
			*str_len = *str_len - i;
			str = str + i;
		} else if (*str == '[') {
			size_t i = 0;
			while (str[i] != ']') i++;
			size_t len = i;
			if (!res) {
				res = parse_klass(str, &len);
				*str_len = *str_len - i - 1;
				str = str + i + 1;
			} else
				res = rx_seq(res, parse_regex(str, str_len));
		} else if (*str == '|') {
			str = str + 1;
			*str_len -= 1;
			res = rx_choice(res, parse_regex(str, str_len));
		} else if (*str == '*') {
			str = str + 1;
			*str_len -= 1;
			res = rx_any(res);
		} else if (*str == '+') {
			str = str + 1;
			*str_len -= 1;
			res = rx_many(res);
		} else {
			if (!res) {
				res = rx_char(str[0]);
				str = str + 1;
				*str_len -= 1;
			} else {
				res = rx_seq(res, parse_regex(str, str_len));
			}
		}
	}

	return res;
}

void print_regex(Regex* rx) {
	switch (rx->type) {
		case REGX_SEQ: {
			print_regex(rx->seq.fst);
			print_regex(rx->seq.snd);
			break;
		}
		case REGX_CHOICE: {
			printf("(");
			print_regex(rx->choice.fst);
			printf("|");
			print_regex(rx->choice.snd);
			printf(")");
			break;
		}
		case REGX_CHAR: {
			printf("%c", rx->ch);
			break;
		}
		case REGX_ANY: {
			print_regex(rx->any);
			printf("*");
			break;
		}
		case REGX_MANY: {
			print_regex(rx->many);
			printf("+");
			break;
		}
		case REGX_CLASS: {
			printf("[");
			for (size_t i = 0; i < rx->klass.len; i++) {
				if (rx->klass.ranges[i][1] == '\0')
					printf("%c", rx->klass.ranges[i][0]);
				else
					printf("%c-%c", rx->klass.ranges[i][0], rx->klass.ranges[i][1]);
			}
			printf("]");
		}
	}
}

void test(char* str, char* regex) {
	size_t str_len = strlen(str);
	size_t regex_len = strlen(regex);
	Regex* rx = parse_regex(regex, &regex_len);
	print_regex(rx);
	printf(": ");
	size_t read = 0;
	bool suc = regex_match(str, str_len, rx, &read);
	if (suc)
		printf("Success!!! Read %zu characters from \"%s\"\n", read, str);
	else
		printf("Not Success?\n");
}

int main(void) {
	test("abbbbcd", "ab*(c)d+");
	test("abbbbfd", "ab*(c|f)d+");
	test("a", "[ab]");
	test("b", "[ab]");
	test("abbbbaaaa", "[ab]*");
	test("some_id69420", "[_a-zA-Z][_a-zA-Z0-9]*");
	test("a_", "[_a-z][_]");
	test("for var i from 0 to 10", "for");
	return 0;
}
