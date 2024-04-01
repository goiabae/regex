#ifndef REGEX_H
#define REGEX_H

#include <stdbool.h>
#include <stddef.h>

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

Regex* regex_parse(char* str, size_t str_len);
bool regex_match(const char* buf, size_t len, Regex* rx, size_t* read);

#endif
