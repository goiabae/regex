#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

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
	Regex* rx = regex_parse(regex, strlen(regex));
	size_t read = 0;
	bool suc = regex_match(str, strlen(str), rx, &read);

	print_regex(rx);
	printf(": ");
	if (suc)
		printf("Success!!! Read %zu characters from \"%s\"\n", read, str);
	else
		printf("Not Success?\n");

	// FIXME deallocate regex
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
