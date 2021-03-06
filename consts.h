static char dayinmonths[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define DAYSECS 24*60*60
#define WEEKSECS 7*DAYSECS

static char digitalmap[10][15] = {
	/* 0 */
	{ 1, 1, 1,
	  1, 0, 1,
	  1, 0, 1,
	  1, 0, 1,
	  1, 1, 1 },
	/* 1 */
	{ 0, 0, 1,
	  0, 0, 1,
	  0, 0, 1,
	  0, 0, 1,
	  0, 0, 1 },
	/* 2 */
	{ 1, 1, 1,
	  0, 0, 1,
	  1, 1, 1,
	  1, 0, 0,
	  1, 1, 1 },
	/* 3 */
	{ 1, 1, 1,
	  0, 0, 1,
	  1, 1, 1,
	  0, 0, 1,
	  1, 1, 1 },
	/* 4 */
	{ 1, 0, 1,
	  1, 0, 1,
	  1, 1, 1,
	  0, 0, 1,
	  0, 0, 1 },
	/* 5 */
	{ 1, 1, 1,
	  1, 0, 0,
	  1, 1, 1,
	  0, 0, 1,
	  1, 1, 1 },
	/* 6 */
	{ 1, 1, 1,
	  1, 0, 0,
	  1, 1, 1,
	  1, 0, 1,
	  1, 1, 1 },
	/* 7 */
	{ 1, 1, 1,
	  0, 0, 1,
	  0, 1, 0,
	  0, 1, 0,
	  0, 1, 0 },
	/* 8 */
	{ 1, 1, 1,
	  1, 0, 1,
	  1, 1, 1,
	  1, 0, 1,
	  1, 1, 1 },
	/* 9 */
	{ 1, 1, 1,
	  1, 0, 1,
	  1, 1, 1,
	  0, 0, 1,
	  1, 1, 1 },
};

static char colon[] = {
	0, 0,
	1, 0,
	0, 0,
	1, 0,
	0, 0
};
