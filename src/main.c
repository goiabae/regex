#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef enum RegexType {
	REGX_SEQ,
	REGX_CHOICE,
	REGX_CHAR,
	REGX_ANY,
	REGX_MANY,
} RegexType;

typedef struct RegexNode {
	RegexType type;
	union {
		struct {
			struct RegexNode* fst;
			struct RegexNode* snd;
		} seq;
		struct {
			struct RegexNode* fst;
			struct RegexNode* snd;
		} choice;
		char ch;
		struct RegexNode* any;
		struct RegexNode* many;
	};
} RegexNode;

typedef struct Regex {
	size_t len;
	RegexNode* rxs;
} Regex;

bool regex_node_match(char* buf, size_t buf_len, RegexNode rx, size_t* read) {
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
		while(regex_node_match(buf + acc, buf_len - acc, *rx.any, &acc2)) {
			acc += acc2;
		}
		*read = acc;
		return true;
	}
	case REGX_MANY: {
		size_t acc = 0;
		size_t acc2 = 0;
		bool suc = false;
		while((suc = suc || regex_node_match(buf + acc, buf_len - acc, *rx.any, &acc2))) {
			acc += acc2;
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

bool regex_match(const char* buf, size_t len, Regex rx, size_t* read) {
	return regex_node_match((char*)buf, len, rx.rxs[0], read);
}

int main(void) {
	RegexNode rxs[20];
	rxs[0] = (RegexNode) {REGX_SEQ, .seq = {&rxs[1], &rxs[2]}};
	rxs[1] = (RegexNode) {REGX_CHAR, .ch = 'a'};
	rxs[2] = (RegexNode) {REGX_SEQ, .seq = {&rxs[3], &rxs[4]}};
	rxs[3] = (RegexNode) {REGX_CHAR, .ch = 'b'};
	rxs[4] = (RegexNode) {REGX_CHAR, .ch = 'c'};

	Regex rx = { 20, rxs };
	char* buf = "abcd";
	size_t read = 0;
	bool suc = regex_match(buf, strlen(buf), rx, &read);
	if (suc)
		printf("Success!!! Read %zu characters from \"%s\"\n", read, buf);
	else
		printf("Not Success?\n");
	return 0;
}
