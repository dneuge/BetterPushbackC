/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * CDDL HEADER END
*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <acfutils/log.h>

#include <acf_outline.h>

bool first_log = true;

static bool is_separator(vect2_t point) {
	return isnan(point.x) && isnan(point.y);
}

static void print_points(const char *key, size_t *i, acf_outline_t *outline) {
	vect2_t point;
	bool first = true;

	printf("  \"%s\": [\n", key);

	while (*i < outline->num_pts) {
		point = outline->pts[(*i)++];

		if (is_separator(point)) {
			break;
		}

		if (!first) {
			printf(",\n");
		}

		printf("    [%f, %f]", point.x, point.y);
		first = false;
	}

	printf("\n  ]");
}

/**
 * Trims the given string from white-space at start and end.
 * The given string is not freed.
 */
static char* trim(const char *s) {
	size_t len = strlen(s);

	char *out = malloc(len+1);
	if (!out) {
		fprintf(stderr, "failed to allocate %ld bytes for trim", len);
		exit(2);
	}

	bool started = false;
	size_t first = -1;
	size_t last = -1;
	for (size_t i=0; i<len; i++) {
		char ch = s[i];

		if (ch == '\n' || ch == ' ') {
			continue;
		}

		if (!started) {
			first = i;
			started = true;
		}

		last = i;
	}

	if (last > 0) {
		memcpy(out, s+first, last-first+1);
	}

	out[last-first+1] = 0;

	return out;
} 

/**
 * Replace all characters that are not basic ASCII excluding control sequences or
 * potentially relevant to JSON string encoding.
 * The given string is not freed.
 */
static char* json_filter(const char *s) {
	size_t len = strlen(s);

	char *out = malloc(len+1);
	if (!out) {
		fprintf(stderr, "failed to allocate %ld bytes for json_filter", len);
		exit(2);
	}

	for (size_t i=0; i<len; i++) {
		char ch = s[i];

		if (ch == '"') {
			ch = '\'';
		} else if (ch == '\\' || ch < 0x20 || ch > 0x7E) {
			ch = '_';
		}

		out[i] = ch;
	}

	out[len] = 0;

	return out;
}

/**
 * libacfutils log callback, receives messages during outline parsing.
 * Only to be used while "log" JSON key is open.
 */
void print_log(const char *msg) {
	if (!first_log) {
		printf(",\n");
	}
	first_log = false;

	char *trimmed_msg = trim(msg);
	char *filtered_msg = json_filter(trimmed_msg);

	printf("    \"%s\"", filtered_msg);

	free(filtered_msg);
	free(trimmed_msg);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Call with the path to the .acf file to dump acf_outline for.\n");
		return 1;
	}

	const char *filepath = argv[1];

	log_init(&print_log, "");

	printf("{\n");
	char *filtered_filepath = json_filter(filepath);
	printf("  \"file\": \"%s\",\n", filtered_filepath);
	free(filtered_filepath);

	printf("  \"log\": [\n");
	acf_outline_t *outline = acf_outline_read(filepath);
	printf("\n  ],\n");

	printf("  \"success\": ");
	if (outline == NULL) {
		printf("false\n}\n");
		log_fini();
		return 1;
	}
	printf("true,\n");

	printf("  \"semispan\": %f,\n", outline->semispan);
	printf("  \"length\": %f,\n", outline->length);
	printf("  \"wingtip\": [%f, %f],\n", outline->wingtip.x, outline->wingtip.y);

	size_t i=0;

	print_points("fuselage", &i, outline);
	printf(",\n");

	print_points("main_wing", &i, outline);
	printf(",\n");

	print_points("stab_wing", &i, outline);
	printf("\n");

	printf("}\n");

	acf_outline_free(outline);
	log_fini();
}
