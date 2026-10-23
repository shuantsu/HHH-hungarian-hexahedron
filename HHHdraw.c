/* Copyright 1991-1998 Lawrence Leinweber Cleveland, Ohio All rights reserved */
#include <windows.h>
#ifdef _Windows
#include <mmsystem.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#define	YELLOW	14
#define	WHITE	15
#define	RED	4
#define	ORANGE	6	/* brown */
#define	BLUE	1
#define	GREEN	2
#define	BLACK	0
#define GRAY	8

/* coordinate systems:
        in struct face, xyz contains 3d coordinates and xy contains 2d.
	the 3d space is projected isometrically onto the 2d space
	such that a 3d coordinate (X3, Y3, Z3) maps to (X2, Y2) as follows:
	X2 = (X3 - Y3) * SQR(3) / 2; Y2 = (X3 + Y3) / 2 - Z3;
	the axes move positive as follows: X2 to the right, Y2 down,
	X3 right and down, Y3 left and down, Z3 up.
	The 3d numbers are 32 bit signed with a 16 bit fraction.
	The cube is centered on the origin of this space and its width is 6.
	The faces lie is these directions: +X is Right (yellow), -X is Left
	(white), +Y is Front (red), -Y is Back (orange), +Z is Up (blue)
	and -Z is Down (green).
	The 2d numbers are 16 bit numbers on a 640 by 350 screen (x,y).
	The R, F and U faces are shown centered at (160, 175)
	and L, B and D faces are centered at (480, 175).  The second image
	appears as if seen in a mirror held behind the cube.
	Units in the 3d system are scaled up by a factor of 18 in the
	vertical direction and a factor 18 * (640/350) * (3/4) in the
	horizontal because the screen's physical aspect ratio is 4 by 3.
	During rotation the cube projects horizontally a maximum of
	3 * (SQRT(3) + SQRT(6)) and vertically 3 * (SQRT(10) + 1).
	The horizontal maximum is slightly larger and, allowing a small
	margin, the range is (0, 640) for both images in 2d units.  If the
	area used on the physical screen is a 2 by 1 rectangle, the
	vertical range of this rectangle is (175 / 3, 875 / 3).
	The function rot() is defined as follows:
	rot(xy, xyz, side, r, rx, ry)
	int *xy, side, r;
	long *xyz, rx, ry;
	The function projects xyz, four sets of 3d coordinates, onto xy,
	four sets of 2d coordinates.  If side is set, the projection occurs
	onto the second image.  (rx, ry) is the rotation unit vector of
	signed 32 numbers with 16 bit fractions.  If r=0 the rotation is
	around the X axis (Y toward Z), r=1, the Y axis (Z toward X) and
	r=2 the Z axis (X toward Y).
	One function actually draws the cube by rendering a parallelogram:
	dopgram(xy, color, page)
	int *xy, color, page;
	The parallelogram vertices are xy (in clockwise order), drawn in
	EGA index color.  The page number argument is defunct.
*/
/*
	new, improved graphics (independent of (640,350) screen):
	windows aspect ratio calls don't seem to work, but newer screen
	dimensions are almost orthogonal.  windows version (upgraded from DOS)
	now much faster, but projecting (640,350) onto other screen shapes
	(i.e. window shapes) causes nasty jaggies.  most of the code
	distills down to getting a bitmap drawing box of aspect ratio
	2 (horizontally) by 1 (vertically).  easy to center this, and get
	coordinate origins of 3d cubes (1/2 from top, 1/4 and 3/4 from left).
	Only (output) magic is scale factors to fit cubes in 3D to boxes in 2D.
	The horizontal scale factor SQRT(3)/2 is coded as 19/22 (-0.27% error).
	From 3D units (where cube width=6), the maximum projection during
	rotation in 2D units is 3*(SQRT(6)+SQRT(3)) by 3*(SQRT(10)+1),
	approximately the same in each direction and approximately 12.5 units.
	To leave a little margin around the edges, 13 is used: so the number
	of available pixels is divided by 13 to get the fundamental scale factor
	(all the code is in cubew).
*/

void rot(int *, long *, int, int, long, long);
void domove(int);
int docube(int, int, long, long, long, long);
void faceread(int *);
void facewrite(int *);
void initgraph(void);
void termgraph(void);
void doface(struct face *);
void dopgram(int *, int, int);
void animate(int, int);
void getface(long *, long *, int, int, int);
void getrot(long *, long *, long, long);
void initrot(void);

static int colors[] = { YELLOW, WHITE, RED, ORANGE, BLUE, GREEN, BLACK, GRAY };

static struct face {	/* face info */
	long f_xyz[4][3];	/* three dimensional coordinates of corners */
	int f_xy[4][2];		/* two dimensional screen coordinates */
	int f_side;		/* if set displayed on mirror image side */
	int f_color;		/* color index */
	int *f_pcolor;		/* pointer to actual color */
	int f_layer[3];		/* mask of layers in which face lies by dim */
	int f_move[4][3];	/* move mapping 90 degree turns by dimension */
} *face;	/* 54 "color" faces; 54 black backgrounds; 72 edges */

struct face *facepile[3][6][40]; /* face stacking[dir][stack order][list] */

static int page = { 0 };	/* current display page */

/*
char zbuf[1024] = { 0 };
void z(void)
{
	MessageBox(0, zbuf, "zbuf", 0);
	zbuf[0] = '\0';
}
*/

void initrot()
{
	int n;
	struct face *f;

	for (n = 0, f = face; n < 108; n++, f++)
		rot(f->f_xy, f->f_xyz, f->f_side = n/9%2, 0, 0x10000L, 0L);
	for (n = 0; n < 72; n++, f++)
		rot(f->f_xy, f->f_xyz, f->f_side = n/12%2, 0, 0x10000L, 0L);
}

/* initialize face structures */
int initface()
{
	long *xyz, l;
	int n, i, *m, a, b, c, na, nb, nc, t, j, k;
	struct face *f, **pf[6];

	if ((face = (struct face *) calloc(180, sizeof(struct face))) == NULL)
		return (1);
	for (n = 0, f = face; n < 54; n++, f++) {
		c = ((b = ((a = n/18) + 1) % 3) + 1) % 3, na = nb = nc = n;
		for (i = 0; i < 4; i++) {
			xyz = f->f_xyz[6>>3-i&3];
			xyz[a] = n/9%2? -0x30000L: 0x30000L;
			xyz[b] = ((long) (n % 3*20 + i%2*16 - 28) << 16) / 10L;
			xyz[c] = ((long) (n/3%3*20 + i/2*16 - 28) << 16) / 10L;
			m = f->f_move[i], m[a] = na, m[b] = nb, m[c] = nc;
			if (i & 1) t = nb, nb = nc, nc = t;
			na = na/9*9 + na%3*3 + (2 - na/3%3);
			nb = (nb/18+2)%3*18 + (1 - nb/9%2)*9 + nb%3*3 + nb/3%3;
			nc = (nc/18+1)%3*18 + nc/9%2*9 + (2 - nc%3)*3 + nc/3%3;
			if (i & 1) t = nb, nb = nc, nc = t;
		}
		f->f_color = n/9;
		f->f_pcolor = &f->f_color;
		f->f_layer[a] = n%18/9? 1: 4;
		f->f_layer[b] = 1 << n%3;
		f->f_layer[c] = 1 << n%9/3;
	}
	for (n = 0; n < 54; n++, f++) {
		c = ((b = ((a = n/18) + 1) % 3) + 1) % 3;
		for (i = 0; i < 4; i++) {
			xyz = f->f_xyz[6>>3-i&3];
			xyz[a] = n/9&1? -0x30000L: 0x30000L;
			xyz[b] = i&1? -0x30000L: 0x30000L;
			xyz[c] = i&2? -0x30000L: 0x30000L;
			xyz[(a+18-n/3)%3] = 0x10000L *
				((n%18/9? 3-n%3*2: n%3*2-1) -
				((n/3%3==0? n%18/9: n/3%3==1? i&2: i&1)? 2: 0));
		}
		f->f_color = 6;
		f->f_pcolor = &f->f_color;
	}
	for (n = 0; n < 72; n++, f++) {
		c = ((b = ((a = n/24) + 1) % 3) + 1) % 3;
		for (i = 0; i < 4; i++) {
			xyz = f->f_xyz[6>>3-i&3];
			xyz[a] = n/12%2? -0x30000L: 0x30000L;
			xyz[b] = ((long) (n % 3*20 + i%2*16 - 28) << 16) / 10L;
			xyz[c] = ((long) (n/3%2*58 + i/2*2  - 30) << 16) / 10L;
			if (n/3%2 == ((i&2) != 0)) xyz[c] = xyz[c] * 49 / 50;
			if (n/6%2) l = xyz[b], xyz[b] = xyz[c], xyz[c] = l;
		}
		if (n/6%2) for (i = 0; i < 3; i++)
			l = f->f_xyz[1][i],
			f->f_xyz[1][i] = f->f_xyz[3][i],
			f->f_xyz[3][i] = l;
		t = n/3;
		f->f_pcolor = &face[n%3*(t&2^3)+(t&1?-9:0)+(t&4?9:t&2?15:11)+
					(t/8+2+(t&2?2:0))%3*18].f_color;
	}
	for (i = 0; i < 3; i++) for (j = 0; j < 6; j++) for (k = 0; k < 3; k++)
		a = j & 1, b = j / 2, t = k + (j < 2? 10: j < 4? 4: 22),
		facepile[i][j][k] = face + 54 + (i*18 + a*9 + k*21 + b) % 54,
		facepile[i][j][t+3] = face + k   + (i+1)%3*18 + 3*(a?  5-b: b),
		facepile[i][j][t+6] = face + k*3 + (i+2)%3*18 +   (a? 11-b: b),
		facepile[i][4+a][k+b*3+9+22] = face + i*18 + a*9 + b*3 + k;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 6; j++) pf[j] = facepile[i][j] + 3;
		for (f = face + 108, k = 0; k < 72; k++, f++) {
			l = f->f_xyz[0][i];
			if (k/12&1) l = -l;
			*pf[(l<-0x10000L?0:l>0x10000L?4:2) + (k/12&1)]++ = f;
		}
	}
	initrot();
	return (0);
}

#ifndef _Windows
#ifndef SHOW
main()
{
	show();
	exit(0);
}
#endif

/* run video cube */
show()
{
	static int scr[] = { 0,0, 640,0, 640,350, 0,350 };
	int c, b, ob, l, x, y, i;
	long ox, oy, nx, ny, rx, ry;
	struct face *f;

	initface();
	gintr(0x10),  dopgram(scr, colors[7], page = 1);
	graphpage(page), page = !page;
	dopgram(scr, colors[7], page);
	b = mouseinit()? -1: 0, x = 160, y = 175;
	ox = rx = 0x10000L, oy = ry = 0L;
	for (ob = 1, l = 0; (c = kbhit()? getch(): -1) != 033;
			ob = b, ox = nx, oy = ny) {
		if (b != -1) b = mouseread(&x, &y);
		if (b != ob) {
			if (ob == 1 || ob == 2)
				docube(ob == 2, l, 0x10000L, 0L, 0x10000L, 0L);
			if ((l = getlayer(x, y)) == -1) b = 0;
			else {
				getface(&rx, &ry, x, y, l);
				if (b == 3) i = ((rx + 0x30000L) / 0x20000L) +
					((ry + 0x30000L) / 0x20000L) * 3 + l*9,
					face[i].f_color++, face[i].f_color %= 6,
					docube(0, 0, 0x10000L, 0L, 0x10000L,0L);
				else getrot(&rx, &ry, nx = 0x10000L, ny = 0L);
			}
		}
		else if (b == 1 || b == 2)
                	getface(&nx, &ny, x, y, l), getrot(&nx, &ny, rx, ry),
			docube(b == 2, l, ox, oy, nx, ny);
                else if (c != -1) docmd(c);
	}
	gintr(3);
	exit(0);
}

/* do keyboard command c */
static docmd(c)
int c;
{
	static int type = { 0 }, layer = { 0 };
	int i, *p, tab[54];

	if (!c) c = -getch();
	if (c >= '1' && c <= '9') c -= '1', i = c%3*3 + c/3,
		p = &face[layer * 9 + (layer & 1? 8 - i: i)].f_color,
		++*p, *p %= 6, docube(0, 0, 0x10000L, 0L, 0x10000L, 0L);
	else switch (c) {
	case 's': case 'S':
		do {
			for (i = 0; i < 54; i++) tab[i] = face[i].f_color;
			if ((i = getsol(tab)) != -1) animate(0, i);
		} while (i != -1 && c == 'S'); break;
	case -77: case -80: case -75:
		animate(type, layer << 2 | 2 + (c == -75) - (c == -77)); break;
	default:	for (i = 0; i < 12 && "rRlLfFbBuUdD"[i] != c; i++);
        		if (i < 12) type = i & 1, layer = i >> 1; break;
        }
}
#endif

/* return facelet colors in t */
void faceread(t)
int *t;
{
	int i;

	for (i = 0; i < 54; *t++ = face[i++].f_color);
}

/* set facelet colors from t */
void facewrite(t)
int *t;
{
	int i;

	for (i = 0; i < 54; face[i++].f_color = *t++);
}

/* perform in video animation move m on cube, type used by docube() */
void animate(type, m)
int type, m;
{
	static long rottab[] =
		{ 0L, 0x4242L, 0x8000L, 0xb505L, 0xddb4L, 0xF747L, 0x10000L };
	int i, j, dir;

	dir = (m & 4? 5 - m : m + 1) & 2? 1 : -1;
	for (i = ~m & 1; i >= 0; i--) for (j = 1; j <= 6; j++)
		docube(type, m >> 2, rottab[7-j], dir * rottab[j-1],
				     rottab[6-j], dir * rottab[j]);
}

/* return layer of screen coordinates (x,y); or -1 */
int getlayer(x, y)
int x, y;
{
	int f1, f2, f3;

	if (f1 = x > 320) x = 640 - x, y = 350 - y;
	x -= 160, y -= 175;
	if (f2 = x < 0) x = -x;
	if (f3 = y < 0) y = -y;
	x *= 3, y *= 4;
	if (x > 374 || y >= 432 || 19 * y + 11 * x > 8218) return (-1);
	return ((f3 && 11 * x <= 19 * y? 4: f2? 2: 0) | f1);
}

/* return coordinates (*px,*py) on layer l with screen coordinates (x,y) */
void getface(px, py, x, y, l)
long *px, *py;
int x, y, l;
{
	long a, b;

	if (l & 1) x = 640 - x, y = 350 - y;
        x -= 160, y -= 175;
	if (!(l & 6)) x = -x;
	x *= 3, y *= 4;
	a = ((long) x << 16) * 22L / 19L / 72L;
	b = ((long) y << 16) / 72L - a / 2L;
	*px = (l & 4? b + a: l & 2? -b:  a) + 0x30000L;
	*py = (l & 4? b    : l & 2?  a: -b) + 0x30000L;
	if (l & 1) *px = -*px, *py = -*py;
}

/* return rotation unit vector (*px,*py), relative to rotation vector (rx,ry) */
void getrot(px, py, rx, ry)
long *px, *py, rx, ry;
{
	long a, b, t, lx, ly;
	static long K = { 0x00009b75L };
	int i;

	a = (lx = *px) < 0? -lx: lx, b = (ly = *py) < 0? -ly: ly;
	if (!a && !b) a = lx = 0x10000;
	while (!((a | b) & ~0xffffL)) lx <<= 1, ly <<= 1, a <<= 1, b <<= 1;
	a = ((unsigned long) (a & 0xffffL) * K >> 16) + (a >> 16) * K;
	b = ((unsigned long) (b & 0xffffL) * K >> 16) + (b >> 16) * K;
	for (i = 0; i < 17; i++)
		if (b < 0L)	t = a, a -= b >> i, b += t >> i;
		else		t = a, a += b >> i, b -= t >> i;
	if (a >>= 8) lx <<= 8, lx /= a, ly <<= 8, ly /= a;
	lx >>= 4, ly >>= 4, rx >>= 4, ry >>= 4;
	*px = rx * lx + ry * ly >> 8;
	*py = rx * ly - ry * lx >> 8;
}

/* carry out move m on cube face color list; m = 24 * t + 4 * f + q.
	t = 0 layer, 1 cube, 2 slice, 3 middle, 4 anti-slice.
	f = RLFBUD, q = quarter turn */
void domove(i)
int i;
{
	struct face *f;
	int *p, tab[54];
	int r, m, q;

	r = i / 8 % 3;
	m = i / 24 == 1? 7: i / 24 == 2? 5: i / 24 == 3? 2: i & 4? 1: 4;
	q = (i & 4? -i: i) & 3;
	for (f = face; f < face + 54; f++)
		tab[f->f_move[f->f_layer[r] & m? q: 0][r]] = f->f_color;
	if (i / 24 == 4) for (f = face; f < face + 54; f++)
		if (f->f_layer[r] & (m ^ 5))
			tab[f->f_move[4 - q & 3][r]] = f->f_color;
	for (f = face, p = tab; f < face + 54; f++->f_color = *p++);
}

/* run complete display generation.  return quadrant crossed by angle */
	/* types: 0 near layer, 1 cube, 2 slice, 3 middle, 4 anti-slice,
	5 far layer */
	/* l is the face of rotation; rotate by angle from (ox,oy) to (nx,ny) */
int docube(type, l, ox, oy, nx, ny)
int type, l;
long ox, oy, nx, ny;
{
	static int mask[] = {	0x92, 0x3F, 0x33, 0xCC, 0x33 };
	int q, r, m, i, j, n;
	struct face *f;
	long t;

	initgraph();
	if (type == 5) type = 0, l ^= 1;
	r = l >> 1, q = 0, m = !type && (l & 1)? 0x61: mask[type];
#ifdef _Windows
	if (type != 1 && ((ox == 0L || oy == 0L) && (nx != 0L && ny != 0L) ||
			(ox < 0L) != (nx < 0L) || (oy < 0L) != (ny < 0L)))
		sndPlaySound("hhh.wav",SND_ASYNC|SND_NODEFAULT);
#endif
	while (-ox >= oy || ox < oy) t = oy, oy = -ox, ox = t, q--;
	while (-nx >= ny || nx < ny) t = ny, ny = -nx, nx = t, q++;
/*
		mciSendString("open hhh.wav", NULL, 0, 0L),
		mciSendString("play", NULL, 0, 0L),
		mciSendString("close", NULL, 0, 0L);
*/
	if (q &= 3) domove(type * 24 + l * 4 + (l & 1? -q & 3: q));
	if (type != 4) for (i = 0; i < 6; i++) if (m >> i & 1) {
		n = i < 2? 19: i < 4? 13: 40;
		for (j = 0; j < n; j++)
			f = facepile[r][i][j],
			rot(f->f_xy, f->f_xyz, f->f_side, r, nx, ny);
	}
	if (type == 4) for (i = 0; i < 6; i++) if (m >> i & 1) {
		n = i < 2? 19: i < 4? 13: 40;
		for (j = 0; j < n; j++)
			f = facepile[r][i][j],
			rot(f->f_xy, f->f_xyz, f->f_side, r, nx,
					(0x61 >> i ^ l) & 1? -ny: ny);
	}
	for (i = 0; i < 6; i++) {
		n = i < 2? 19: i < 4? 13: 40;
		for (j = (m ^ m >> 2) >> i & 1? 0: 1; j < n; j++)
			doface(facepile[r][i][j]);
	}
	termgraph();
	return (l & 1? -q & 3: q);
}

/* draw face f on current page */
static void doface(f)
struct face *f;
{
#ifndef _Windows
	mouserun(0);
#endif
	dopgram(f->f_xy, colors[*(f->f_pcolor)], page);
#ifndef _Windows
	mouserun(1);
#endif
}

#ifndef _Windows
/* initialize graphics page */
static void initgraph()
{
	static int scr[] = { 0,60, 640,60, 640,290, 0,290 };

	mouserun(0), dopgram(scr, colors[7], page), mouserun(1);
}

/* terminate graphics page */
static void termgraph()
{
	graphpage(page), mousepage(page), page = !page;
}

/* initialize mouse, return non-zero on error */
static mouseinit()
{
	struct REGPACK r;

	r.r_ax = 0, intr(0x33, &r);
	if (r.r_ax != -1) return (-1);
	r.r_ax = 1, intr(0x33, &r);
	return (0);
}

/* if i is zero, suspend mouse driver; otherwise let run */
static mouserun(i)
int i;
{
	struct REGPACK r;

	r.r_ax = i? 0x20: 0x1f, intr(0x33, &r);
}

/* display mouse cursor on CRT on page p */
static mousepage(p)
int p;
{
	struct REGPACK r;

	r.r_ax = 2, intr(0x33, &r);
	r.r_ax = 0x1d, r.r_bx = p, intr(0x33, &r);
	r.r_ax = 1, intr(0x33, &r);
}

/* read mouse inputs into (*px,*py) and return button bits: left=1, right=2 */
static int mouseread(px, py)
int *px, *py;
{
	struct REGPACK r;

	r.r_ax = 3, intr(0x33, &r);
	*px = r.r_cx, *py = r.r_dx;
	return (r.r_bx & 3);
}

/* show on CRT graphics page p */
static graphpage(p)
int p;
{
	struct REGPACK r;
	int x, y;

/*
	while (inportb(0x3DA) & 8);
	while (~inportb(0x3DA) & 8);
*/
	r.r_ax = 2, intr(0x33, &r);
	outportb(0x3CE, 5), outportb(0x3CF, 1);
	pagecopy(0xA000, page? 0x8000: 0, 0xA000, page? 0: 0x8000, 28000);
	outportb(0x3CE, 5), outportb(0x3CF, 0);
	r.r_ax = 1, intr(0x33, &r);
	page = !page;
/*
	outportb(0x3D4, 0x0C), outportb(0x3D5, p? 0x80: 0);
	r.r_ax = 0x500 | p, intr(0x10, &r);
*/
}

/* break to graphics interrupt 0x10, with ax register = a */
static gintr(a)
int a;
{
	struct REGPACK r;

	r.r_ax = a, intr(0x10, &r);
}
#endif
