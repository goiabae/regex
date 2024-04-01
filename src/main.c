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
} RegexType;

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

#if 0
Regex* parse_regex(char* str, size_t str_len) {
	Regex* fst = NULL;
	if (str[0] == '(') {
		size_t i = 0;
		size_t j = 1;
		while (str[i] != ')' && j == 0) {
			if (str[0] == '(')
				j++;
			else if (str[0] == ')')
				j--;
			i++;
		}
		fst = parse_regex(str + 1, str_len - i - 1);
	} else {
		fst = rx_char(str[0]);
	}

	Regex* res = NULL;
	if (str_len > 1) {
		if (str[1] == '|')
			res = rx_choice(fst, parse_regex(str + 2, str_len - 2));
		else if (str[1] == '+')
			res = rx_many(fst);
		else if (str[1] == '*')
			res = rx_any(fst);
		else
			res = rx_seq(fst, parse_regex(str + 1, str_len - 1));
	} else {
		res = fst;
	}
	return res;
}
#endif

Regex* parse_regex(char* str, size_t* str_len) {
	Regex* res = NULL;
	if (str[0] == '(') {
		size_t i = 0;
		size_t j = 1;
		while (str[i] != ')' && j == 0) {
			if (str[0] == '(')
				j++;
			else if (str[0] == ')')
				j--;
			i++;
		}
		*str_len = *str_len - i - 1;
		str = str + 1;
		res = parse_regex(str, str_len);
	} else {
		res = rx_char(str[0]);
		str = str + 1;
		*str_len -= 1;
	}

	while (*str_len > 0) {
		if (*str == '(') {
			size_t i = 0;
			size_t j = 1;
			while (str[i] != ')' && j == 0) {
				if (str[0] == '(')
					j++;
				else if (str[0] == ')')
					j--;
				i++;
			}
			*str_len = *str_len - i - 1;
			str = str + 1;
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
			res = rx_seq(res, parse_regex(str, str_len));
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
			print_regex(rx->choice.fst);
			printf("|");
			print_regex(rx->choice.snd);
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
	}
}

int main(void) {
	char* regex = "ab*cd+";
	size_t regex_len = strlen(regex);
	Regex* rx = parse_regex(regex, &regex_len);

	print_regex(rx);

	char* buf = "abbbbcd";
	size_t read = 0;
	bool suc = regex_match(buf, strlen(buf), rx, &read);
	if (suc)
		printf("Success!!! Read %zu characters from \"%s\"\n", read, buf);
	else
		printf("Not Success?\n");
	return 0;
}
