/*
 * no3inline -- generate 2N points on a NxN grid, but not 3 on a line
 */

/*
 * Copyright (C) 2026 Alexios Zavras
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_RANDOM_ATTEMPTS 1000

typedef struct {
    int x;
    int y;
} POINT;

typedef enum { RANDOM_SEARCH, EXACT_SEARCH } SEARCH_MODE;

static
int
gcd(int a, int b)
{
    while (b != 0) {
	int t = a % b;
	a     = b;
	b     = t;
    }
    return a;
}

static
void
mark_lines(size_t       n,
           uint32_t    *blocked,
           const POINT *points,
           size_t       count,
           POINT        p,
           int          delta)
{
    int size = (int) n;
    for (size_t i = 0; i < count; ++i) {
	int dx  = points[i].x - p.x;
	int dy  = points[i].y - p.y;
	int g   = gcd(abs(dx), abs(dy));
	dx     /= g;
	dy     /= g;

	int x = p.x;
	int y = p.y;
	while (x - dx >= 0 && x - dx < size && y - dy >= 0 && y - dy < size) {
	    x -= dx;
	    y -= dy;
	}
	while (x >= 0 && x < size && y >= 0 && y < size) {
	    blocked[(size_t) x * n + (size_t) y] += (uint32_t) delta;
	    x                                    += dx;
	    y                                    += dy;
	}
    }
}

static
void
print_grid(const unsigned char *grid, size_t n)
{
    for (size_t row = 0; row < n; ++row) {
	for (size_t column = 0; column < n; ++column) {
	    putchar(grid[row * n + column] ? 'X' : '.');
	}
	putchar('\n');
    }
}

static
int
randomized_search(size_t         n,
                  size_t         grid_size,
                  size_t         target,
                  size_t         attempts,
                  unsigned char *grid,
                  uint32_t      *blocked,
                  POINT         *points)
{
    size_t *order = malloc(grid_size * sizeof(*order));
    if (order == NULL) {
	perror("malloc");
	return -1;
    }

    for (size_t i = 0; i < grid_size; ++i) {
	order[i] = i;
    }

    srand((unsigned int) time(NULL));

    for (size_t restart = 0; restart < attempts; ++restart) {
	for (size_t i = grid_size - 1; i > 0; --i) {
	    size_t j         = (size_t) rand() % (i + 1);
	    size_t temporary = order[i];
	    order[i]         = order[j];
	    order[j]         = temporary;
	}

	memset(grid, 0, grid_size);
	memset(blocked, 0, grid_size * sizeof(*blocked));
	size_t count = 0;

	for (size_t i = 0; i < grid_size && count < target; ++i) {
	    size_t cell      = order[i];
	    POINT  candidate = {
		.x = (int) (cell / n),
		.y = (int) (cell % n),
	    };

	    if (blocked[cell] == 0) {
		mark_lines(n, blocked, points, count, candidate, 1);
		points[count++] = candidate;
		grid[cell]      = 1;
	    }
	}

	if (count == target) {
	    free(order);
	    return 1;
	}
    }

    free(order);
    return 0;
}

static
int
exact_search(size_t         n,
             size_t         grid_size,
             size_t         target,
             size_t         start,
             size_t         count,
             unsigned char *grid,
             uint32_t      *blocked,
             POINT         *points)
{
    if (count == target) {
	return 1;
    }

    if (grid_size - start < target - count) {
	return 0;
    }

    size_t last = grid_size - (target - count);
    for (size_t cell = start; cell <= last; ++cell) {
	POINT candidate = {
	    .x = (int) (cell / n),
	    .y = (int) (cell % n),
	};

	/* each row can hold at most 2 points, so all earlier rows need 2 */
	if (count < 2 * (size_t) candidate.x) {
	    break;
	}

	if (blocked[cell] != 0) {
	    continue;
	}

	mark_lines(n, blocked, points, count, candidate, 1);
	points[count] = candidate;
	grid[cell]    = 1;
	if (exact_search(n, grid_size, target, cell + 1, count + 1, grid,
	                 blocked, points)) {
	    return 1;
	}
	grid[cell] = 0;
	mark_lines(n, blocked, points, count, candidate, -1);
    }

    return 0;
}

static
void
print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s [--random | --exact] [--attempts COUNT] N\n",
            program);
}

static
int
parse_positive_integer(const char *text, uintmax_t *value)
{
    errno            = 0;
    char     *end    = NULL;
    uintmax_t parsed = strtoumax(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed == 0) {
	return 0;
    }

    *value = parsed;
    return 1;
}

int
main(int argc, char **argv)
{
    SEARCH_MODE mode              = RANDOM_SEARCH;
    const char *number            = NULL;
    size_t      attempts          = DEFAULT_RANDOM_ATTEMPTS;
    int         mode_selected     = 0;
    int         attempts_selected = 0;

    for (int i = 1; i < argc; ++i) {
	if (strcmp(argv[i], "--random") == 0
	    || strcmp(argv[i], "--exact") == 0) {
	    if (mode_selected) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	    }
	    if (strcmp(argv[i], "--random") == 0) {
		mode = RANDOM_SEARCH;
	    } else {
		mode = EXACT_SEARCH;
	    }
	    mode_selected = 1;
	} else if (strcmp(argv[i], "--attempts") == 0) {
	    uintmax_t parsed_attempts;
	    if (attempts_selected
	        || i + 1 >= argc
	        || !parse_positive_integer(argv[++i], &parsed_attempts)
	        || parsed_attempts > SIZE_MAX) {
		fprintf(stderr, "--attempts requires a positive integer.\n");
		return EXIT_FAILURE;
	    }
	    attempts          = (size_t) parsed_attempts;
	    attempts_selected = 1;
	} else if (argv[i][0] == '-') {
	    print_usage(argv[0]);
	    return EXIT_FAILURE;
	} else if (number == NULL) {
	    number = argv[i];
	} else {
	    print_usage(argv[0]);
	    return EXIT_FAILURE;
	}
    }

    if (number == NULL || (mode == EXACT_SEARCH && attempts_selected)) {
	print_usage(argv[0]);
	return EXIT_FAILURE;
    }

    uintmax_t parsed;
    if (!parse_positive_integer(number, &parsed)
        || parsed < 2
        || parsed > INT_MAX) {
	fprintf(stderr, "N must be an integer between 2 and %d.\n", INT_MAX);
	return EXIT_FAILURE;
    }

    size_t n = (size_t) parsed;
    if (n > SIZE_MAX / n) {
	fprintf(stderr, "N is too large.\n");
	return EXIT_FAILURE;
    }
    size_t grid_size = n * n;
    if (grid_size > SIZE_MAX / sizeof(size_t)) {
	fprintf(stderr, "N is too large.\n");
	return EXIT_FAILURE;
    }

    size_t target = 2 * n;
    if (target > SIZE_MAX / sizeof(POINT)) {
	fprintf(stderr, "N is too large.\n");
	return EXIT_FAILURE;
    }

    unsigned char *grid    = calloc(grid_size, sizeof(*grid));
    uint32_t      *blocked = calloc(grid_size, sizeof(*blocked));
    POINT         *points  = malloc(target * sizeof(*points));
    if (grid == NULL || blocked == NULL || points == NULL) {
	perror("memory allocation");
	free(grid);
	free(blocked);
	free(points);
	return EXIT_FAILURE;
    }

    int result;
    if (mode == EXACT_SEARCH) {
	result =
	    exact_search(n, grid_size, target, 0, 0, grid, blocked, points);
    } else {
	result = randomized_search(n, grid_size, target, attempts, grid,
	                           blocked, points);
    }

    if (result == 1) {
	print_grid(grid, n);
    } else if (result == 0) {
	fprintf(stderr, "Could not find a placement of %zu points", target);
	if (mode == RANDOM_SEARCH) {
	    fprintf(stderr, " in %zu randomized attempts", attempts);
	}
	fputs(".\n", stderr);
    }

    free(grid);
    free(blocked);
    free(points);
    return result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
