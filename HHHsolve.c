/* Copyright 1991-1998 Lawrence Leinweber Cleveland, Ohio All rights reserved */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#ifdef __MSDOS__
#include <fcntl.h>
#define	OPEN	O_BINARY
#else
#define	OPEN	0
#endif

#ifdef __MSDOS__
int setmode(int, int);
void search(int);
void test(long);
int strlen(char *);
char *ftoc(struct cube *, int *);
void readsol(void);
void solve(struct cube *, char *, long *, char **, int);
int mtoc(struct cube *, char **, int);
void initcube(struct cube *);
void move(struct cube *, struct cube *, int);
int movesol(struct cube *);
int nrnd(int);
void ctof(int *, struct cube *);
void picturecube(int *, char **);
int nstate(int *, int, int);
int ncombine(int *, int, int, int, int);
int ncorner(int *);
int npermute(int *, int);
void error(char *);
void init(void);
#define CUBARG	struct cube *
#else
#define CUBARG
#endif

/* This program is dedicated to Gene Roddenberry who died during its writing */

#define	MOVENAME	" rRq lLk fFe bBa uUt dDc\n"	 /* single char moves */

struct cube {	/* working cube info */
	int c_eplace[12], c_estate[12], c_cplace[8], c_cstate[8];
	/* location and flip/twist for edges and corners */
};

static char *name = { "coset??.HHH" };	/* filename prototype */
static int fac[54];			/* facelet string prototype */

static long coset0(), coset1(), coset2(), coset3();

static struct group {	/* info for mapping to next subgroup */
	long (*g_coset)(CUBARG);/* coset function */
	long g_len;		/* number of cosets */
	int g_max;		/* maximum number of moves (or bust!) */
} group[] = {
	{coset0,  2048L,  7}, {coset1, 1082565L, 10},
	{coset2, 29400L, 13}, {coset3,  663552L, 15} };

static int verbose = { 0 };	/* Shall I expound? */

static char *diamond[] = {	/* diamond shaped views for picturecube() */
 "      d            m            V            m            m            m    ",
 "     beh          opq          RUX          qnk          kji          ilo   ",
 "    +---+        +---+        +---+        +---+        +---+        +---+  ",
 "   O|ijk|F      Q|TWZ|H      K|xyz|B      Z|HGF|h      F|heb|O      b|OPQ|T ",
 "  MP|lmn|GD    MN|SVY|ED    MJ|uvw|AD    VY|EDC|gd    DC|gda|LM    da|LMN|SV",
 "   Q|opq|H      K|RUX|B      I|rst|@      X|BA@|f      @|fc`|I      `|IJK|R ",
 "    +---+        +---+        +---+        +---+        +---+        +---+  ",
 "     TWZ          xyz          `cf          zwt          tsr          rux   ",
 "      V            v            d            v            v            v    ",
 0 };	/* any resemblance to Kubrick's spaceships is purely coincidental */

static int l3tab[256][5];	/* l3tab[i][j] is the j'th tri-digit of i */
static char *mem[4][8];		/* solution tables */

main(argc, argv)
int argc;
char **argv;
{
	int c, i, j;
	char *s, **vd, *ftoc();
	long atol();
	struct cube cube, cube1;

	init();
#ifdef	SHOW
	if (argc == 1) readsol(), show(), exit(0);
#endif
	if (argc < 2 || *argv[1] != '-' || !argv[1][1]) c = 0;
	else {
		for (s = argv[1]; *s; s++)
			if (*s >= 'A' && *s <= 'Z') *s -= 'A', *s += 'a';
		for (i = 0; (c = "stfmi"[i]) && c != argv[1][1]; i++);
	}
	if (c && ((verbose = argv[1][2] == 'v')? argv[1][3]: argv[1][2])) c = 0;
	vd = verbose? diamond: (char **) 0;
	switch (c) {
	case's':if (argc != 3) fprintf(stderr, "Usage: %s -s[v] group\n",*argv),
					exit(1);
#ifdef __MSDOS__
		if (!verbose) setmode(1, O_BINARY);
#endif
		search(atoi(argv[2])); break;
	case't':if (argc != 3) fprintf(stderr,"Usage: %s -t[v] number\n",*argv),
					exit(1);
		test(atol(argv[2])); break;
	case'f':if (argc != 8) fprintf(stderr, "Usage: %s -f[v] U F D R B L\n",
					*argv), exit(1);
		for (i = 0; i < 6; i++)
			if (strlen(argv[i + 2]) != 9) fprintf(stderr,
				"Wrong number of facelets\n", *argv), exit(1);
		for (i = 0; i < 3; i++) for (j = 0; j < 3; j++)
			fac[ 3*i+j+36] = *argv[2]++,fac[ 3*j-i+20] = *argv[3]++,
			fac[-3*i+j+51] = *argv[4]++,fac[-3*i-j+ 8] = *argv[5]++,
			fac[-3*j-i+35] = *argv[6]++,fac[-3*i+j+15] = *argv[7]++;
		if (s = ftoc(&cube, fac)) fprintf(stderr,
				"Cube specification error: %s\n", s), exit(1);
		readsol(), solve(&cube, fac, (long *) 0, vd, 1); break;
	case'm':if (mtoc(&cube, argv + 2, argc - 2))
			fprintf(stderr, "Bad move list\n"), exit(1);
		readsol(), solve(&cube, fac, (long *) 0, vd, 1); break;
	case'i':if (mtoc(&cube1, argv + 2, argc - 2))
			fprintf(stderr, "Bad move list\n"), exit(1);
		for (i = 0; i < 12; i++)
			cube.c_eplace[j = cube1.c_eplace[i]] = i,
			cube.c_estate[j] = (2 - cube1.c_estate[i]) % 2;
		for (i = 0; i <  8; i++)
			cube.c_cplace[j = cube1.c_cplace[i]] = i,
			cube.c_cstate[j] = (3 - cube1.c_cstate[i]) % 3;
		readsol(), solve(&cube, fac, (long *) 0, vd, 1); break;
	default:fprintf(stderr, "Usages:\n");
		fprintf(stderr, "\t%s -s[v] group\n", *argv);
		fprintf(stderr, "\t%s -t[v] number\n", *argv);
		fprintf(stderr, "\t%s -f[v] U F D R B L\n", *argv);
		fprintf(stderr, "\t%s -m[v] [moves]...\n", *argv);
		fprintf(stderr, "\t%s -i[v] [moves]...\n", *argv);
		exit(1); break;
	}
	exit(0);
	return (0);
}

/* initialize tables */
static void init()
{
	int i, j, k;

	for (i = 0, k = 1; i < 5; i++, k *= 3) for (j = 0; j < 256; j++)
		l3tab[j][i] = j / k % 3;
	for (i = 0; i < 54; i++) fac[i] = "RLFBUD"[i/9];
}

/* perform search from subgroup g; results on standard output */
static void search(g)
int g;
{
	static char *meml[8], *memf[5];
	int i, j, k, f, a, len, l3, *m, *mm, moves[20];
	long n, t;
	struct cube cubes[20], *c, *l;
	char *p;

	len = (int) ((group[g].g_len - 1L) / 40L) + 1;
	for (i = 0; i < 13; i++) {
		if (!(p = malloc(len)))
			fprintf(stderr, "Out of memory!\n"), exit(1);
		if (i < 8) meml[i] = p; else memf[i - 8] = p;
		for (j = len; j--; *p++ = 0);
	}
	initcube(l = c = cubes), m = moves, memf[0][0] = 1;
	if (verbose) putchar('\n');
	for (n = 1; n; l++) {
		fprintf(stderr, "%2d %7ld\n", (int) (l - cubes), n);
		a = (l - cubes + 1) % 3? 1: -2;
		for (i = 0; i < len; i++, a /= 243)
			for (j = 0; j < 5; j++, a *= 3) {
				p = memf[j] + i, f = *p, *p = 0;
				for (k = 0; k < 8; k++, f >>= 1)
					if (~f & 1) meml[k][i] += a;
			}
		memf[0][0] = 1, l3 = 1;
		for (c++, *m = 24, *++m = 0, n = 0; c > cubes; ++*m)
			if (*m == 24) c--, m--, l3++, l3++, l3 %= 3;
			else if (*m & 1? *m >> 3 < 3 - g: *m & 2) {
				move(c, c - 1, *m), t = (*group[g].g_coset)(c);
				i = (int) (t / 40L), j = (int) (t % 40L);
				k = j >> 3, j &= 7, f = 1 << j, p = memf[k] + i;
				if (~*p & f && l3tab[meml[j][i]&255][k] == l3) {
					*p |= f;
					if (verbose && c > l)
						for (mm = m; mm >= moves; mm--)
							putchar(MOVENAME[*mm]);
					if (c > l) n++;
					else c++, *++m = -1, l3++, l3 %= 3;
				}
			}
	}
	if (!verbose) for (i = 0; i < 8; i++) if (write(1, meml[i], len) != len)
		fprintf(stderr, "Write error\n");
}

static int goflag = { 0 }, stat = { 46 };

/* search solution tables from face list f.  return move list. */
char *backsearch(f, func, cycles)
int *f;
void (*func)(void);
int cycles;
{
	static char best[46];
	struct {
		struct cube s_c;	/* current cube */
		int s_m;		/* move */
		int s_n;		/* coalesced move count */
		int s_b;		/* back moves left */
		int s_g;		/* group */
		long s_t;		/* coset */
		int s_3;		/* moves required in coset % 3 */
	} stack[46], *sp;
	int nbest, i, j, n;

	n = cycles;
	stat = nbest = 46;
	*best = 0;
	sp = stack, sp->s_m = 24, sp->s_n = 0, sp->s_b = 0;
	if (ftoc(&sp->s_c, f)) return (best);
	for (sp->s_g = 0; sp->s_g < 4; sp->s_g++)
		if (sp->s_t = (*group[sp->s_g].g_coset)(&sp->s_c)) break;
	if (sp->s_g == 4) return (best);
	i = (int) (sp->s_t / 40L), j = (int) (sp->s_t % 40L);
	sp->s_3 = l3tab[mem[sp->s_g][j & 7][i] & 255][j >> 3];
	do for ((++sp)->s_m = 0, stat = stack->s_b << 8 | nbest;
			stack->s_b < nbest &&
			sp > stack && (goflag || nbest == 46); sp->s_m++) {
		if (func && !n--) n = cycles, (*func)();
		if (sp->s_m == 24) { sp--; continue; }
		if (!(sp->s_m & 3)) continue;
		sp->s_g = sp[-1].s_g, sp->s_b = sp[-1].s_b;
		if ((sp->s_m & 1) && (i = 2 - (sp->s_m >> 3)) < sp->s_g)
			if ((sp->s_b -= 2) < 0) continue;
			else sp->s_g = i;
		move(&sp->s_c, &sp[-1].s_c, sp->s_m);
		for (; sp->s_g < 4; sp->s_g++)
			if (sp->s_t = (*group[sp->s_g].g_coset)(&sp->s_c))
				break;
		if (sp->s_g < 4)
			i = (int) (sp->s_t / 40L), j = (int) (sp->s_t % 40L),
			sp->s_3 = l3tab[mem[sp->s_g][j & 7][i] & 255][j >> 3];
		if (sp->s_g == sp[-1].s_g &&
				(sp->s_b -= (4 + sp->s_3 - sp[-1].s_3) % 3) < 0)
			continue;
		sp->s_n = sp[-1].s_n;
		i = (sp->s_m ^ sp[-1].s_m) >> 2;
		if (i && (i != 1 || (sp->s_m ^ sp[-2].s_m) >> 2)) sp->s_n++;
		if (sp->s_n >= nbest) continue;
		if (sp->s_g < 4) { (++sp)->s_m = 0; continue; }
		stat = stack->s_b << 8 | (nbest = sp->s_n);
		for (i = 0; i < sp - stack; i++) best[i] = stack[i+1].s_m;
		best[i] = 0;
	} while (++(stack->s_b) < nbest && goflag);
	for (i = 0, j = 0; best[j]; j++)
		if (i > 0 && !((best[i-1] ^ best[j]) >> 2))
			best[i-1] = best[i-1] & ~3 | best[i-1] + best[j] & 3;
		else if (i > 1 && ((best[i-1] ^ best[j]) >> 2) == 1 &&
				!((best[i-2] ^ best[j]) >> 2))
			best[i-2] = best[i-2] & ~3 | best[i-2] + best[j] & 3;
		else best[i++] = best[j];
	best[i] = 0;
	return (best);
}

/* report back search status asynchronously.
	best solution move count in low byte, back search level in hi byte. */
int backstat(void)
{
	return (stat);
}

/* set go flag asynchronously and reset status if !flag */
void backgo(flag)
int flag;
{
	if (!(goflag = flag)) stat = 46;
}

/* return next move for solving cube from face list f, or -1 */
int getsol(f)
int *f;
{
	struct cube cube;
	char *ftoc();

	return (ftoc(&cube, f)? -1: movesol(&cube));
}

/* return next move for solving cube c, or -1 */
static int movesol(c)
struct cube *c;
{
	struct cube cube;
	int g, i, j, l3, m;
	long t;

	for (g = 0; g < 4; g++) if (t = (*group[g].g_coset)(c)) break;
	if (g == 4) return (-1);
	i = (int) (t / 40L), j = (int) (t % 40L);
	l3 = (l3tab[mem[g][j & 7][i] & 255][j >> 3] + 2) % 3;
	for (m = 0; m < 24; m++) if (m & 1? m >> 3 < 3 - g: m & 2) {
		move(&cube, c, m), t = (*group[g].g_coset)(&cube);
		i = (int) (t / 40L), j = (int) (t % 40L);
		if (l3 == l3tab[mem[g][j & 7][i] & 255][j >> 3]) return (m);
	}
	return (-1);
}

/* read solution tables */
void readsol()
{
	int g, i, fd, len;
	char buf[1024];

	init();
	for (g = 0; g < 4; g++) {
		name[6] = (name[5] = g + '0') + 1;
		if ((fd = open(name, OPEN)) == -1)
			sprintf(buf, "Can't open %s", name), error(buf);
		len = (int) ((group[g].g_len - 1L) / 40L) + 1;
		for (i = 0; i < 8; i++)
			if (!(mem[g][i] = malloc(len)))
				error("Out of memory!");
			else if (read(fd, mem[g][i], len) != len)
				error("Read error");
		if (close(fd)) error("Close error");
	}
}

#if !_Windows
void error(s)
char *s;
{
	fprintf(stderr, "%s\n", s), exit(1);
}
#endif

/* perform self-test n times, output sample if verbose */
static void test(n)
long n;
{
	long l, nn, tot[5][46], cnt[4], time();
	int i, g;
	struct cube *c, *cn, *ct, cube1, cube2;

	readsol();
	if (verbose)
		do for (l = n, nn = 0; l; nn <<= 1, nn |= nrnd(2), l >>= 1);
		while (nn >= n);
	if (verbose) printf("Sample %d of %d\n", nn + 1, n);
	for (g = 0; g <= 4; g++) for (i = 0; i <= 45; i++) tot[g][i] = 0;
	while (n--) {
		initcube(c = &cube1), cn = &cube2;
		for (i = 0; i < 100; i++, ct = c, c = cn, cn = ct)
			move(cn, c, nrnd(3) + 1 + 4 * nrnd(6));
		solve(c, fac, cnt, !nn? diamond: (char **) 0, !nn), nn--;
		for (g = 0, i = 0; g < 4; g++) tot[g][cnt[g]]++, i += cnt[g];
		tot[4][i]++;
	}
	printf("      H0->H1    H1->H2    H2->H3    H3->H4    H0->H4\n");
	for (i = 0; i <= 45; i++) {
		printf("%2d", i);
		for (g = 0; g <= 4; g++) printf(g < 4 && i > group[g].g_max?
					"          ": "%10ld", tot[g][i]);
		printf("%5d\n", i);
	}
}

/* linear distribution [0-i) */
int nrnd(i)
int i;
{
	static int flag = { 0 };
	static unsigned long seed;

	if (!flag) flag = 1, seed = time((long *) 0);
	return (((seed = seed * 1103515245L + 12345L) & 077777) * i >> 15);
}

/* solve cube cc, from face list s (for center colors); if stat,
	return number of moves in cnt[group]; if pict show cube with pict
	using picturecube(); if pflag, print moves */
static void solve(cc, f, cnt, pict, pflag)
struct cube *cc;
char *f, **pict;
long *cnt;
int pflag;
{
	struct cube *c, *cn, *ct, cube1, cube2;
	int g, i, a, fd, m, n;
	long lseek();
	char mm;

	c = cc, cn = &cube1, ct = &cube2;
	if (pict) ctof(f, c), picturecube(f, pict);
	for (g = 0, n = 0; g < 4; g++, n += i) {
		if (pflag) printf("H%d->H%d:", g, g + 1);
		for (i = 0; (*group[g].g_coset)(c); i++) {
			if (i == group[g].g_max) printf("Oops!\n"), ctof(f, cc),
				picturecube(f, diamond), exit(1);
			m = movesol(c);
			if (pflag) printf(" %c%c",MOVENAME[m&~1|2],"  2'"[m&3]);
			move(cn, c, m), c = cn, cn = ct, ct = c;
		}
		if (cnt) cnt[g] = i;
		if (pflag) for (a = i; a++ < 15; printf("   "));
		if (pflag && i < 10) putchar(' ');
		if (pflag) printf(" (%d move%s)\n", i, "s" + (i==1));
		if (pict) ctof(f, c), picturecube(f, pict);
	}
	if (pflag) printf("Total of %d move%s.\n", n, "s" + (n==1));
	for (i = 0; i < 12; i++) if (c->c_eplace[i] != i || c->c_estate[i])
		printf("Oops!\n"), ctof(f, cc), picturecube(f, diamond),exit(1);
	for (i = 0; i <  8; i++) if (c->c_cplace[i] != i || c->c_cstate[i])
		printf("Oops!\n"), ctof(f, cc), picturecube(f, diamond),exit(1);
}

static int edgemap[12][2] = {	 5,25, 14,19, 12,28,   3,34,  7,41,  1,50,
				10,48, 16,39, 43,23,  37,32, 46,30, 52,21 };
static int cornmap[ 8][3] = {	 8,44,26,  15,36,29,   0,47,33,  11,51,18,
				17,20,42,   6,35,38,   9,27,45,   2,24,53 };
	/* map edge and corner cube place and state onto facelets */

/* return in face list f, cube c; in s, centers and trailing null required */
static void ctof(f, c)
int *f;
struct cube *c;
{
	int i, j;

	for (i = 0; i < 12; i++) for (j = 0; j < 2; j++)
		f[edgemap[i][j]] =
			f[edgemap[c->c_eplace[i]][(c->c_estate[i]+j)%2]/9*9+4];
	for (i = 0; i <  8; i++) for (j = 0; j < 3; j++)
		f[cornmap[i][j]] =
			f[cornmap[c->c_cplace[i]][(c->c_cstate[i]+j)%3]/9*9+4];
}

/* return cube in c from face list f, return error message or null */
char *ftoc(c, f)
struct cube *c;
int *f;
{
	int i, j, t, u, v, p, pp, map[128], tab[12];

	for (i = 0; i < 54 && f[i]; i++)
		if (f[i] & ~127) return ("Bad character");
	for (i = 0; i < 128; i++) map[i] = 0;
	for (i = 4; i < 54; i += 9)
		if (map[f[i]]) return ("Center facelet color duplicate");
		else map[f[i]]++;
	for (i = 0; i < 54; i++)
		if (!map[f[i]]) return ("Too many colors");
		else if (map[f[i]]++ > 9)
			return ("Too many facelets of one color");
	for (i = 4; i < 54; i += 9) map[f[i]] = i / 9;
	for (i = 0, p = 0; i < 12; i++) {
		t = map[f[edgemap[i][0]]], u = map[f[edgemap[i][1]]];
		for (j = 0; j < 24; j++)
			if (edgemap[j/2][j%2]/9 == t &&
				edgemap[j/2][(j+1)%2]/9 == u) break;
		if (j == 24) return ("Edge cubie colors do not match centers");
		tab[i] = j/2, p += j%2;
		if (c) c->c_eplace[i] = j/2, c->c_estate[i] = j%2;
	}
	for (i = 0, pp = 0; i < 12; i++) while (tab[i] != i)
		t = tab[i], tab[i] = tab[t], tab[t] = t, pp ^= 1;
	if (p % 2) return ("Cube unsolvable: Flip parity");
	for (i = 0, p = 0; i <  8; i++) {
		t = map[f[cornmap[i][0]]], u = map[f[cornmap[i][1]]];
		v = map[f[cornmap[i][2]]];
		for (j = 0; j < 24; j++)
			if (cornmap[j/3][j%3]/9 == t &&
				cornmap[j/3][(j+1)%3]/9 == u &&
				cornmap[j/3][(j+2)%3]/9 == v) break;
		if (j == 24) return("Corner cubie colors do not match centers");
		tab[i] = j/3, p += j%3;
		if (c) c->c_cplace[i] = j/3, c->c_cstate[i] = j%3;
	}
	if (p % 3) return ("Cube unsolvable: Twist parity");
	for (i = 0; i < 12; i++) while (tab[i] != i)
		t = tab[i], tab[i] = tab[t], tab[t] = t, pp ^= 1;
	return (pp? "Cube unsolvable: Swap parity": 0);
}

/* print display mapped strings ps substituting facelets from face list f */
static void picturecube(f, ps)
int *f;
char **ps;
{
	char *t;
	int c;

	for (; t = *ps++; putchar('\n')) while ((c = *t++) & 0xff)
		putchar(c >= '@' && c <= 'Z'? f[c - '@']:
				c >= '`' && c <= 'z'? f[c - '`' + 27]: c);
}

/* return in cube, a cube moved by list ps, length n; return non-zero on error*/
static int mtoc(cc, ps, n)
struct cube *cc;
char **ps;
int n;
{
	struct cube *c, *cn, *ct, cube1, cube2;
	char *s;
	int m;

	initcube(c = &cube1), cn = &cube2;
	while (n--) for (s = *ps++; *s; s++) {
		for (m = 0; m < 12 && *s != MOVENAME[m*2+1-(m&1)]; m++);
		if (m == 12) return (-1); else m = m<<1&~3|1;
		if (s[1] == '-' || s[1] == '\'') m += 2, s++;
		else if (s[1] == '2') m += 1, s++;
		move(cn, c, m), ct = c, c = cn, cn = ct;
	}
	move(cc, c, 0);
	return (0);
}
		
/* initialize cube c to "start" position */
static void initcube(c)
struct cube *c;
{
	int i;

	for (i = 0; i < 12; i++) c->c_eplace[i] = i, c->c_estate[i] = 0;
	for (i = 0; i <  8; i++) c->c_cplace[i] = i, c->c_cstate[i] = 0;
}

#if 0
/* return in cube y, cube x moved by m */
static void move(y, x, m)
struct cube *y, *x;
int m;
{
	static int cornmove[6][4] = {	/* basic corner move map */
		 0, 5, 2, 7,	 1, 4, 3, 6,
		 0, 7, 3, 4, 	 1, 6, 2, 5,
		 4, 1, 5, 0,	 7, 2, 6, 3 };
	static int edgemove[6][4] = {	/* basic edge move map */
		 4, 3, 5, 0,	 6, 2, 7, 1,
		 8, 0,11, 1,	 9, 2,10, 3,
		 8, 7, 9, 4,	10, 6,11, 5 };
	register int i, *e, *c, f, t;

	for (i = 0; i < 12; i++)
		y->c_eplace[i] = x->c_eplace[i],
		y->c_estate[i] = x->c_estate[i];
	for (i = 0; i <  8; i++)
		y->c_cplace[i] = x->c_cplace[i],
		y->c_cstate[i] = x->c_cstate[i];
	e = edgemove[m >> 2], f = (m & 1) && (m & 16);
	c = cornmove[m >> 2], t = (m & 1) && (m & 24);
	for (i = 0; i < 4; i++, t = 3 - t)
		y->c_eplace[e[i + m & 3]] = x->c_eplace[e[i]],
		y->c_cplace[c[i + m & 3]] = x->c_cplace[c[i]],
		y->c_estate[e[i + m & 3]] = (x->c_estate[e[i]] + f) % 2,
		y->c_cstate[c[i + m & 3]] = (x->c_cstate[c[i]] + t) % 3;
}

/****** stuff for cosets ******/

/* return the coset number of cube c that partitions H0 by H1 */
static long coset0(c)
struct cube *c;
{
	return ((long) nstate(c->c_estate, 12, 2) / 2);
}

/* return the coset number of cube c that partitions H1 by H2 */
static long coset1(c)
struct cube *c;
{
	return ((long) nstate(c->c_cstate, 8, 3) / 3 * 495 +
		       ncombine(c->c_eplace, 12, 4, 495, 8));
}

/* return the coset number of cube c that partitions H2 by H3 */
static long coset2(c)
struct cube *c;
{
	return (((long) ncombine(c->c_eplace, 8, 4, 70, 4) * 70 +
			ncombine(c->c_cplace, 8, 4, 70, 4)) * 6 +
			ncorner(c->c_cplace));
}

/* return the coset number of cube c that partitions H3 by H4 (not at all) */
static long coset3(c)
struct cube *c;
{
	return ((((((long) npermute(c->c_eplace,     4)      * 24 +
			   npermute(c->c_eplace + 4, 4))     * 24 +
			   npermute(c->c_eplace + 8, 4)) / 2 * 24 +
			   npermute(c->c_cplace,     4))     * 24 +
			   npermute(c->c_cplace + 4, 4)) / 6);
}

/* return state number of n ints in list l with i possible states */
static int nstate(l, n, i)
int *l, n, i;
{
	register int t;

	for (t = 0; n--; t *= i, t += *l++);
	return (t);
}

/* return combination number of n ints in l; there are i ints masked with m;
	a total number of combinations c */
static int ncombine(l, n, i, c, m)
register int *l, n, i, c, m;
{
	int t, d;

	t = 0;
	while (i && n) {
		c -= d = c * i / n--;
		if (*l++ & m) i--, t += c, c = d;
	}
	return (t);
}

/* return permutation number of n ints in l, ints mod with n */
static int npermute(l, n)
int *l, n;
{
	register int j, k, m, t, i;

	for (m = -1, t = 0, i = n; i > 0; m &= ~k, i--)
		for (j = *l++ % n, t *= i, k = 1; j-- > 0; k <<= 1)
			if (m & k) t++;
	return (t);
}

/* return corner parity number 0-5 for H3, for corner list l */
static int ncorner(l)
int *l;
{
	static int x[10][4] = { 0,1,4,5, 0,2,4,6, 0,3,4,7, 1,2,5,6,
		1,3,5,7, 2,3,6,7, 4,5,6,7, 4,6,5,7, 4,7,5,6, 4,5,6,7 };
	register int *p, a, i, t[8];

	for (i = 0; i < 8; i++) t[l[i]] = i;
	for (i = 0; i < 10; i++) if (p = x[i], t[p[0]] > t[p[1]])
		a = t[p[0]], t[p[0]] = t[p[1]], t[p[1]] = a,
		a = t[p[2]], t[p[2]] = t[p[3]], t[p[3]] = a;
	return ((t[5] > t[6]) + (t[5] > t[7]) << 1 | (t[6] > t[7]));
}
#else
/* optimized */

#define E(d,s,i) (y->c_eplace[d]=x->c_eplace[s],y->c_estate[d]=x->c_estate[s]^i)
#define E0(d,s)	(E(d,s,0))
#define E1(d,s)	(E(d,s,1))
#define	C(d,s,i) (y->c_cplace[d]=x->c_cplace[s],y->c_cstate[d]=x->c_cstate[s]+i)
#define C0(d,s)	(C(d,s,0))
#define	C1(d,s)	if(C(d,s,1) == 3) y->c_cstate[d] = 0
#define C2(d,s)	if(C(d,s,-1) < 0) y->c_cstate[d] = 2

/* return in cube y, cube x moved by m */
static void move(y, x, m)
register struct cube *y, *x;
int m;
{
	*y = *x;
	switch (m) {
	case  1:
		E0( 3,  4); E0( 5,  3); E0( 0,  5); E0( 4,  0);
		C0( 5,  0); C0( 2,  5); C0( 7,  2); C0( 0,  7); break;
	case  2:
		E0( 5,  4); E0( 0,  3); E0( 4,  5); E0( 3,  0);
		C0( 2,  0); C0( 7,  5); C0( 0,  2); C0( 5,  7); break;
	case  3:
		E0( 0,  4); E0( 4,  3); E0( 3,  5); E0( 5,  0);
		C0( 7,  0); C0( 0,  5); C0( 5,  2); C0( 2,  7); break;
	case  5:
		E0( 2,  6); E0( 7,  2); E0( 1,  7); E0( 6,  1);
		C0( 4,  1); C0( 3,  4); C0( 6,  3); C0( 1,  6); break;
	case  6:
		E0( 7,  6); E0( 1,  2); E0( 6,  7); E0( 2,  1);
		C0( 3,  1); C0( 6,  4); C0( 1,  3); C0( 4,  6); break;
	case  7:
		E0( 1,  6); E0( 6,  2); E0( 2,  7); E0( 7,  1);
		C0( 6,  1); C0( 1,  4); C0( 4,  3); C0( 3,  6); break;
	case  9:
		E0( 0,  8); E0(11,  0); E0( 1, 11); E0( 8,  1);
		C1( 7,  0); C2( 3,  7); C1( 4,  3); C2( 0,  4); break;
	case 10:
		E0(11,  8); E0( 1,  0); E0( 8, 11); E0( 0,  1);
		C0( 3,  0); C0( 4,  7); C0( 0,  3); C0( 7,  4); break;
	case 11:
		E0( 1,  8); E0( 8,  0); E0( 0, 11); E0(11,  1);
		C1( 4,  0); C2( 0,  7); C1( 7,  3); C2( 3,  4); break;
	case 13:
		E0( 2,  9); E0(10,  2); E0( 3, 10); E0( 9,  3);
		C1( 6,  1); C2( 2,  6); C1( 5,  2); C2( 1,  5); break;
	case 14:
		E0(10,  9); E0( 3,  2); E0( 9, 10); E0( 2,  3);
		C0( 2,  1); C0( 5,  6); C0( 1,  2); C0( 6,  5); break;
	case 15:
		E0( 3,  9); E0( 9,  2); E0( 2, 10); E0(10,  3);
		C1( 5,  1); C2( 1,  6); C1( 6,  2); C2( 2,  5); break;
	case 17:
		E1( 7,  8); E1( 9,  7); E1( 4,  9); E1( 8,  4);
		C1( 1,  4); C2( 5,  1); C1( 0,  5); C2( 4,  0); break;
	case 18:
		E0( 9,  8); E0( 4,  7); E0( 8,  9); E0( 7,  4);
		C0( 5,  4); C0( 0,  1); C0( 4,  5); C0( 1,  0); break;
	case 19:
		E1( 4,  8); E1( 8,  7); E1( 7,  9); E1( 9,  4);
		C1( 0,  4); C2( 4,  1); C1( 1,  5); C2( 5,  0); break;
	case 21:
		E1( 6, 10); E1(11,  6); E1( 5, 11); E1(10,  5);
		C1( 2,  7); C2( 6,  2); C1( 3,  6); C2( 7,  3); break;
	case 22:
		E0(11, 10); E0( 5,  6); E0(10, 11); E0( 6,  5);
		C0( 6,  7); C0( 3,  2); C0( 7,  6); C0( 2,  3); break;
	case 23:
		E1( 5, 10); E1(10,  6); E1( 6, 11); E1(11,  5);
		C1( 3,  7); C2( 7,  2); C1( 2,  6); C2( 6,  3); break;
	}
}
/* return the coset number of cube c that partitions H0 by H1 */
static long coset0(c)
struct cube *c;
{
	register int t, i, *p;

	for (t = 0, i = 11, p = c->c_estate; i-- > 0; t <<= 1, t |= *p++);
	return ((long) t);
}

/* return the coset number of cube c that partitions H1 by H2 */
static long coset1(c)
struct cube *c;
{
	static int tab[] = {
	  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  3,  4,
	  5,  5,  5,  5,  6,  6,  7,  8,  9,  9, 10, 11, 12, 13, 14, 14,
	 15, 15, 15, 15, 16, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 24,
	 25, 25, 26, 27, 28, 29, 30, 30, 31, 32, 33, 33, 34, 34, 34, 34,
	 35, 35, 35, 35, 36, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 44,
	 45, 45, 46, 47, 48, 49, 50, 50, 51, 52, 53, 53, 54, 54, 54, 54,
	 55, 55, 56, 57, 58, 59, 60, 60, 61, 62, 63, 63, 64, 64, 64, 64,
	 65, 66, 67, 67, 68, 68, 68, 68, 69, 69, 69, 69, 69, 69, 69, 69,
	 70, 70, 70, 70, 71, 71, 72, 73, 74, 74, 75, 76, 77, 78, 79, 79,
	 80, 80, 81, 82, 83, 84, 85, 85, 86, 87, 88, 88, 89, 89, 89, 89,
	 90, 90, 91, 92, 93, 94, 95, 95, 96, 97, 98, 98, 99, 99, 99, 99,
	100,101,102,102,103,103,103,103,104,104,104,104,104,104,104,104,
	105,105,106,107,108,109,110,110,111,112,113,113,114,114,114,114,
	115,116,117,117,118,118,118,118,119,119,119,119,119,119,119,119,
	120,121,122,122,123,123,123,123,124,124,124,124,124,124,124,124,
	125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,125,
	126,126,126,126,127,127,128,129,130,130,131,132,133,134,135,135,
	136,136,137,138,139,140,141,141,142,143,144,144,145,145,145,145,
	146,146,147,148,149,150,151,151,152,153,154,154,155,155,155,155,
	156,157,158,158,159,159,159,159,160,160,160,160,160,160,160,160,
	161,161,162,163,164,165,166,166,167,168,169,169,170,170,170,170,
	171,172,173,173,174,174,174,174,175,175,175,175,175,175,175,175,
	176,177,178,178,179,179,179,179,180,180,180,180,180,180,180,180,
	181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,
	182,182,183,184,185,186,187,187,188,189,190,190,191,191,191,191,
	192,193,194,194,195,195,195,195,196,196,196,196,196,196,196,196,
	197,198,199,199,200,200,200,200,201,201,201,201,201,201,201,201,
	202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,202,
	203,204,205,205,206,206,206,206,207,207,207,207,207,207,207,207,
	208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,208,
	209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,
	209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,209,
	210,210,210,210,211,211,212,213,214,214,215,216,217,218,219,219,
	220,220,221,222,223,224,225,225,226,227,228,228,229,229,229,229,
	230,230,231,232,233,234,235,235,236,237,238,238,239,239,239,239,
	240,241,242,242,243,243,243,243,244,244,244,244,244,244,244,244,
	245,245,246,247,248,249,250,250,251,252,253,253,254,254,254,254,
	255,256,257,257,258,258,258,258,259,259,259,259,259,259,259,259,
	260,261,262,262,263,263,263,263,264,264,264,264,264,264,264,264,
	265,265,265,265,265,265,265,265,265,265,265,265,265,265,265,265,
	266,266,267,268,269,270,271,271,272,273,274,274,275,275,275,275,
	276,277,278,278,279,279,279,279,280,280,280,280,280,280,280,280,
	281,282,283,283,284,284,284,284,285,285,285,285,285,285,285,285,
	286,286,286,286,286,286,286,286,286,286,286,286,286,286,286,286,
	287,288,289,289,290,290,290,290,291,291,291,291,291,291,291,291,
	292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,
	293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,
	293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,
	294,294,295,296,297,298,299,299,300,301,302,302,303,303,303,303,
	304,305,306,306,307,307,307,307,308,308,308,308,308,308,308,308,
	309,310,311,311,312,312,312,312,313,313,313,313,313,313,313,313,
	314,314,314,314,314,314,314,314,314,314,314,314,314,314,314,314,
	315,316,317,317,318,318,318,318,319,319,319,319,319,319,319,319,
	320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,320,
	321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,
	321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,321,
	322,323,324,324,325,325,325,325,326,326,326,326,326,326,326,326,
	327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,327,
	328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,
	328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,328,
	329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,
	329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,
	329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,
	329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,329,
	330,330,330,330,331,331,332,333,334,334,335,336,337,338,339,339,
	340,340,341,342,343,344,345,345,346,347,348,348,349,349,349,349,
	350,350,351,352,353,354,355,355,356,357,358,358,359,359,359,359,
	360,361,362,362,363,363,363,363,364,364,364,364,364,364,364,364,
	365,365,366,367,368,369,370,370,371,372,373,373,374,374,374,374,
	375,376,377,377,378,378,378,378,379,379,379,379,379,379,379,379,
	380,381,382,382,383,383,383,383,384,384,384,384,384,384,384,384,
	385,385,385,385,385,385,385,385,385,385,385,385,385,385,385,385,
	386,386,387,388,389,390,391,391,392,393,394,394,395,395,395,395,
	396,397,398,398,399,399,399,399,400,400,400,400,400,400,400,400,
	401,402,403,403,404,404,404,404,405,405,405,405,405,405,405,405,
	406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,406,
	407,408,409,409,410,410,410,410,411,411,411,411,411,411,411,411,
	412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,412,
	413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
	413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,
	414,414,415,416,417,418,419,419,420,421,422,422,423,423,423,423,
	424,425,426,426,427,427,427,427,428,428,428,428,428,428,428,428,
	429,430,431,431,432,432,432,432,433,433,433,433,433,433,433,433,
	434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,434,
	435,436,437,437,438,438,438,438,439,439,439,439,439,439,439,439,
	440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,440,
	441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
	441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,441,
	442,443,444,444,445,445,445,445,446,446,446,446,446,446,446,446,
	447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,447,
	448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
	448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,448,
	449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
	449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
	449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
	449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,449,
	450,450,451,452,453,454,455,455,456,457,458,458,459,459,459,459,
	460,461,462,462,463,463,463,463,464,464,464,464,464,464,464,464,
	465,466,467,467,468,468,468,468,469,469,469,469,469,469,469,469,
	470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,470,
	471,472,473,473,474,474,474,474,475,475,475,475,475,475,475,475,
	476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,476,
	477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,
	477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,477,
	478,479,480,480,481,481,481,481,482,482,482,482,482,482,482,482,
	483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,483,
	484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,
	484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,484,
	485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,
	485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,
	485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,
	485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,485,
	486,487,488,488,489,489,489,489,490,490,490,490,490,490,490,490,
	491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,491,
	492,492,492,492,492,492,492,492,492,492,492,492,492,492,492,492,
	492,492,492,492,492,492,492,492,492,492,492,492,492,492,492,492,
	493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,
	493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,
	493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,
	493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,493,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,
	494,494,494,494,494,494,494,494,494,494,494,494,494,494,494,494 };
	register int t, u, i, *p;

	for (t = 0, i =  7, p = c->c_cstate; i-- > 0; t += (t << 1) + *p++);
	for (u = 0, i = 11, p = c->c_eplace; i-- > 0; u <<= 1, u |= *p++ & 8);
	return ((long) t * 495 + tab[u >> 3]);
}

/* return the coset number of cube c that partitions H2 by H3 */
static long coset2(c)
struct cube *c;
{
	static int tab[] = {
	  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  3,  4,
	  5,  5,  5,  5,  6,  6,  7,  8,  9,  9, 10, 11, 12, 13, 14, 14,
	 15, 15, 15, 15, 16, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 24,
	 25, 25, 26, 27, 28, 29, 30, 30, 31, 32, 33, 33, 34, 34, 34, 34,
	 35, 35, 35, 35, 36, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 44,
	 45, 45, 46, 47, 48, 49, 50, 50, 51, 52, 53, 53, 54, 54, 54, 54,
	 55, 55, 56, 57, 58, 59, 60, 60, 61, 62, 63, 63, 64, 64, 64, 64,
	 65, 66, 67, 67, 68, 68, 68, 68, 69, 69, 69, 69, 69, 69, 69, 69 };
	register int t, u, i, *p;

	for (t = 0, i =  7, p = c->c_eplace; i-- > 0; t <<= 1, t |= *p++ & 4);
	for (u = 0, i =  7, p = c->c_cplace; i-- > 0; u <<= 1, u |= *p++ & 4);
	return (((long) tab[t >> 2] * 70 + tab[u >> 2]) * 6 +
			ncorner(c->c_cplace));
}

/* return the coset number of cube c that partitions H3 by H4 (not at all) */
static long coset3(c)
struct cube *c;
{
	return ((((((long) npermute(c->c_eplace,     4)      * 24 +
			   npermute(c->c_eplace + 4, 4))     * 24 +
			   npermute(c->c_eplace + 8, 4)) / 2 * 24 +
			   npermute(c->c_cplace,     4))     * 24 +
			   npermute(c->c_cplace + 4, 4)) / 6);
}

/* return state number of n ints in list l with i possible states */
static int nstate(l, n, i)
int *l, n, i;
{
	register int t;

	for (t = 0; n--; t *= i, t += *l++);
	return (t);
}

/* return combination number of n ints in l; there are i ints masked with m;
	a total number of combinations c */
static int ncombine(l, n, i, c, m)
register int *l, n, i, c, m;
{
	int t, d;

	t = 0;
	while (i && n) {
		c -= d = c * i / n--;
		if (*l++ & m) i--, t += c, c = d;
	}
	return (t);
}

/* return permutation number of n ints in l, ints mod with n */
static int npermute(l, n)
int *l, n;
{
	(void) n;
	switch (*l++ & 3) {
	case 0:	switch (*l++ & 3) {
		case 1:	return ((*l & 3) == 2? 0: 1);
		case 2: return ((*l & 3) == 1? 2: 3);
		case 3:	return ((*l & 3) == 1? 4: 5);
		}
	case 1:	switch (*l++ & 3) {
		case 0: return ((*l & 3) == 2? 6: 7);
		case 2: return ((*l & 3) == 0? 8: 9);
		case 3: return ((*l & 3) == 0?10:11);
		}
	case 2:	switch (*l++ & 3) {
		case 0:	return ((*l & 3) == 1?12:13);
		case 1: return ((*l & 3) == 0?14:15);
		case 3: return ((*l & 3) == 0?16:17);
		}
	case 3: switch (*l++ & 3) {
		case 0: return ((*l & 3) == 1?18:19);
		case 1: return ((*l & 3) == 0?20:21);
		case 2: return ((*l & 3) == 0?22:23);
		}
	}
	return (-1);
}

/* return corner parity number 0-5 for H3, for corner list l */
static int ncorner(l)
int *l;
{
	static int x[10][4] = { 0,1,4,5, 0,2,4,6, 0,3,4,7, 1,2,5,6,
		1,3,5,7, 2,3,6,7, 4,5,6,7, 4,6,5,7, 4,7,5,6, 4,5,6,7 };
	register int *p, a, i, t[8];

	for (i = 0; i < 8; i++) t[l[i]] = i;
	for (i = 0; i < 10; i++) if (p = x[i], t[p[0]] > t[p[1]])
		a = t[p[0]], t[p[0]] = t[p[1]], t[p[1]] = a,
		a = t[p[2]], t[p[2]] = t[p[3]], t[p[3]] = a;
	return ((t[5] > t[6]) + (t[5] > t[7]) << 1 | (t[6] > t[7]));
}
#endif
