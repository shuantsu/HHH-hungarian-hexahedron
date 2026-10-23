/* Copyright 1996-1998 Lawrence Leinweber Cleveland, Ohio All rights reserved */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <commdlg.h>
#include "HHH.rc"

#ifndef GCL_HCURSOR
#define GCL_HCURSOR	(-12)
#endif

#define	BUFLEN		8192	/* length of global buffers */
#define GENMARGIN	2	/* generator window margins around edit */
#define	GENINDENT	60	/* generator window edit window indent */
#define GENHEIGHT	22	/* generator window height */

#define WX(hW)	((struct wx *) GetWindowLong((hW), 0))

#define GREPLACE(s) ((void) SendMessage(gr.gr_wg, EM_REPLACESEL, 0, (LPARAM) s))
#define GSELECT(i,j) ((void) SendMessage(gr.gr_wg, EM_SETSEL, 0, MAKELONG(i,j)))
#define GGETTEXT()	GetWindowText(gr.gr_wg, linebuf, BUFLEN)

struct wl {			/* window file list item info */
	struct wl *wl_prev;		/* last item */
	struct wl *wl_next;		/* next item */
	int wl_face[56];		/* face color list */
	char wl_text[4];		/* generator text */
};

struct wx {			/* window file info (WNDCLASS.cbWndExtra) */
	HWND	wx_prev;		/* next window */
	HWND	wx_next;		/* last window */
	int	wx_save;		/* file not saved flag */
	HWND	wx_list;                /* list box child window */
	struct wl wx_item;		/* item list */
	struct wl wx_undo;		/* undo list */
	struct wl *wx_undo0;		/* undo begin */
	struct wl *wx_undo1;		/* undow end */
	char	wx_name[_MAX_PATH];	/* name for title */
	char	wx_path[_MAX_PATH];	/* full file name, or "" */
};

int Mess(HWND, char *, char *, int);
int regwin(UINT, WNDPROC, LPSTR);
HWND fwopen(HWND, int);
int aboutbox(HWND);
LRESULT CALLBACK cbdialogabout(HWND, UINT, WPARAM, LPARAM);
int savefile(HWND, int);
void mkfilewin(int);
LRESULT CALLBACK cbfilewin(HWND, UINT, WPARAM, LPARAM);
void initfilemenu(HWND);
void dialogop(HWND);
void mkiconmap(void);
void drawfile(HWND, LPDRAWITEMSTRUCT);
void itemmake(HWND, int, struct wl *);
void itemins(HWND, int, struct wl *, struct wl *);
void itemempty(struct wl *);
void itemdel(HWND, int, int, struct wl *, struct wl *, struct wl *);
void fileop(HWND, int);
LRESULT CALLBACK cbdialogop(HWND, UINT, WPARAM, LPARAM);
int mkmainwin(void);
void dohelp(HWND, UINT, DWORD);
void rmmainwin(void);
void scalewin(int, int);
LRESULT CALLBACK cbmainwin(HWND, UINT, WPARAM, LPARAM);
void cbmainmenu(int, int);
void initmainmenu(void);
int upcube(int, int);
void facetoeq(int, int);
void timeserv(void);
void CALLBACK cbtimer(HWND, UINT, UINT, DWORD);
LRESULT CALLBACK cbdialogbusy(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK cbeditwin(HWND, UINT, WPARAM, LPARAM);
void editinit(void);
void editterm(void);
void cbcolor(int, int, int);
void drawcolor(LPDRAWITEMSTRUCT);
int cbmainmouse(WPARAM, LPARAM);
void catmove(int);
void rparen(void);
void readsol(void);
int getsol(int *);
int initface(void);
void initrot(void);
void domove(int);
int docube(int, int, long, long, long, long);
void animate(int, int);
int winlayer(int, int);
void winface(long *, long *, int, int, int);
void getrot(long *, long *, long, long);
void dopgram(int *, int, int);
void rot(int *, long *, int, int, long, long);
int eqtomove(char *, char *, int);
void faceread(int *);
void facewrite(int *);
char *ftoc(char *, int *);
char *backsearch(int *, void (*func)(), int);
int backstat(void);
void backgo(int);
int nrnd(int);
void error(char *);
void exit(int);

static HINSTANCE hInstance;	/* application instance */
static HACCEL hA;		/* accelerators */
static int editmode = { 0 };	/* facelet colors being edited? */
static int genmode = { 0 };	/* generator being edited? */
static int hint = { 0 };	/* current hint level */
static int searchtime = { 0 };	/* search secs for solution (-1 until break) */
static FILE *fp = { 0 };	/* current file pointer */
static char *linebuf;		/* buffer for text line */
static char *linetab;		/* main cube generator */
static char *movebuf;		/* buffer for move list */
static char *movetab;		/* list of main cube moves */
static char *iconbuf;		/* buffer for drawing file item */
static char helpfile[_MAX_PATH];
static char hintfile[_MAX_PATH];
static char initfile[_MAX_PATH];

static struct {			/* search timer info */
	int t_limits[5];	/* time limit for All, One, Regen, Scram, Edit*/
	int t_limit;		/* time limit for current search (or -1 none) */
	DWORD t_tick;		/* system ticks at start */
	DWORD t_next;		/* next time break (ticks) */
	HWND t_hW;		/* dialog window */
	int t_move;		/* best move count */
	int t_back;		/* current back search limit */
} timer;

static struct gr {     		/* Windows stuff for cube graphics */
	HWND		gr_w;		/* current window */
	HWND		gr_wm;		/* main window */
	HWND		gr_we;		/* edit window */
	HDC		gr_dm;		/* device context of main window */
	HDC		gr_de;		/* device context of edit window */
	HDC		gr_dd;		/* device context of drawing device */
	int		gr_cl;		/* current color */
	HBRUSH		gr_br[16];	/* brushes (16 EGA colors) */
	HBRUSH		gr_brm;		/* menu background */
	COLORREF	gr_clh;		/* highlight background */
	COLORREF	gr_clht;	/* highlight text */
	COLORREF	gr_clw;		/* window background */
	COLORREF	gr_clwt;	/* window text */
	HBRUSH		gr_obr;		/* original brush */
	HPEN		gr_op;		/* original pen */
	HFONT		gr_of;		/* original font main window */
	HFONT		gr_ofe;		/* original font edit window */
/*	int		gr_ax;		/* x pixel width (rel) */
/*	int		gr_ay;		/* y pixel width (rel) */
	int		gr_wx;		/* window x size */
	int		gr_wy;		/* window y size */
	HBITMAP		gr_bm;		/* drawing bitmap */
	HBITMAP		gr_obm;		/* old drawing bitmap */
	HWND		gr_wg;		/* generator edit window */
	HMENU		gr_mc;		/* menu of colors */
	HICON		gr_i;		/* icon */
	HBITMAP		gr_bc;		/* color bitmap (for editting) */
	int		gr_bcx;		/* color bitmap item width */
	int		gr_bcy;		/* color bitmap item height */
	int		gr_fm[54];	/* facelet colors of main window */
	int		gr_fe[54];	/* facelet colors of edit window */
	RECT		gr_r;		/* rect of "restored" main window */
	TEXTMETRIC	gr_tm;		/* font header info */
	int		gr_cw[256];	/* widths of ASCII chars in font */
	HCURSOR		gr_ca;		/* normal arrow cursor */
	HCURSOR		gr_cs[12];	/* cursors [face + down? * 6] */
	int		gr_bx;		/* drawing bitmap width */
	int		gr_by;		/* drawing bitmap height */
	int		gr_bx0;		/* drawing bitmap x origin */
	int		gr_by0;		/* drawing bitmap y origin */
	int		gr_byc0;	/* drawing bitmap cube y origin */
	int		gr_bxc0l;	/* drawing bitmap cube x origin left */
	int		gr_bxc0r;	/* drawing bitmap cube x oright right */
	long		gr_bx1;		/* drawing bitmap scale factor */
	long		gr_by1;		/* drawing bitmap scale factor */
} gr;

struct iconbi {		/* icon bitmap info for list box cube */
	BITMAPINFOHEADER bi_h;	/* header */
	RGBQUAD bi_q[8];	/* colors */
} iconbi = {{ sizeof(BITMAPINFOHEADER), 64, 64, 1, 8, BI_RGB, 0, 0, 0, 8, 0 }};

static char iconmap[4096];	/* map facelet to pixel, 54=black, 55=gray */

/*
char zbuf[1024] = { 0 };
void z(void)
{
	MessageBox(0, zbuf, "zbuf", 0);
	zbuf[0] = '\0';
}
*/

int PASCAL WinMain(hIcurr, hIprev, cmd, nCmdShow)
HINSTANCE hIcurr, hIprev;
LPSTR cmd;
int nCmdShow;
{
	MSG msg;
	int i;

	(void) hIprev, (void) cmd;
	hInstance = hIcurr;
	if (!(linebuf = malloc(BUFLEN)) || !(linetab = malloc(BUFLEN)) ||
		!(movebuf = malloc(BUFLEN)) || !(movetab = malloc(BUFLEN)) ||
		!(iconbuf = malloc(BUFLEN)))
		error("Out of Memory!");
#ifndef SHAREWARE
	mkiconmap();
#endif
	readsol();
	if (initface()) return (0);
	if (!mkmainwin()) return (0);
	if (access(initfile, 0) && aboutbox(gr.gr_w))
		{ rmmainwin(); return (0); }
#ifdef SHAREWARE
	aboutbox(gr.gr_w);
#endif
	(void) ShowWindow(gr.gr_w, nCmdShow);
	i = GGETTEXT(), GSELECT(i, i);
	hA = LoadAccelerators(hInstance, MAKEINTRESOURCE(ACCELERATOR));
	while (GetMessage(&msg, (HWND) NULL, 0, 0))
		if (!hA || !TranslateAccelerator(GetActiveWindow(), hA, &msg))
			TranslateMessage(&msg), DispatchMessage(&msg);
	rmmainwin();
	return (msg.wParam);
}

/* write message s with title t with parent hW.  nbuttons == 1,
	STOP icon, OK button, 2: "!" OK-CANCEL, 3: "?" YES-NO-CANCEL */
int Mess(hW, s, t, nbuttons)
HWND hW;
char *s, *t;
int nbuttons;
{
	return (MessageBox(hW, s, t, nbuttons == 1? MB_ICONSTOP:
		nbuttons == 2? MB_ICONEXCLAMATION | MB_OKCANCEL:
		MB_ICONQUESTION | MB_YESNOCANCEL));
}

/* general fatal error message s */
void error(s)
char *s;
{
	Mess(NULL, s, "Fatal Error", 1);
	exit(1);
}

/* RegisterClass with generic info.  style is a bunch of flags, lpfnWndProc
	is callback function.  lpszClassName is system global id.  return ok? */
int regwin(style, lpfnWndProc, lpszClassName)
UINT    style;
WNDPROC lpfnWndProc;
LPSTR lpszClassName;
{
	WNDCLASS wc;

	wc.style = style, wc.lpfnWndProc = lpfnWndProc;
	wc.cbClsExtra = 0, wc.cbWndExtra = 4;
	wc.hInstance = hInstance, wc.hIcon = gr.gr_i;
	wc.hCursor = gr.gr_ca;
	wc.hbrBackground = gr.gr_br[8];
	wc.lpszMenuName = NULL, wc.lpszClassName = lpszClassName;
	return (RegisterClass(&wc));
}

static int aboutbox(hW)
HWND hW;
{
	DLGPROC dp;
	int cm;

	dp = (DLGPROC) MakeProcInstance((FARPROC) cbdialogabout, hInstance);
	cm = DialogBox(hInstance, MAKEINTRESOURCE(DIALOG_ABOUT), hW, dp);
/*
	FreeProcInstance((FARPROC) dp);
*/
/*
	if (cm == IDHELP)
		dohelp(hW, HELP_CONTENTS, 0L),
		dohelp(hW, HELP_CONTEXT, 1L);
*/
	return (cm == IDCANCEL);
}

/* callback function for about dialog window */
static LRESULT CALLBACK cbdialogabout(hW, uMsg, wParam, lParam)
HWND hW;
UINT uMsg;
WPARAM wParam;
LPARAM lParam;
{
	(void) lParam;
	if (uMsg == WM_INITDIALOG) MessageBeep(MB_OK);
	else if (uMsg != WM_COMMAND) return (0);
	else if (wParam == IDOK || wParam == IDCANCEL || wParam == IDHELP)
		EndDialog(hW, wParam);
	else return (0);
	return (1);
}

/* open file, assign it to hW. if askflag, ask user for filename.
	return 0 on error, hW on success, or other window if file already open.
	if name assigned already, open for writing, otherwise for reading. */
static HWND fwopen(hW, askflag)
HWND hW;
int askflag;
{
	extern char **_argv;
	OPENFILENAME o;
	char *s, buf[_MAX_PATH], dir[_MAX_PATH];
	char fname[_MAX_PATH], ext[_MAX_PATH];
	static char path[_MAX_PATH];
	int i, wflag;
	HWND w;

	fp = 0;
	wflag = *WX(hW)->wx_path;
	_splitpath(wflag? WX(hW)->wx_path: *path? path: *_argv,
		buf, dir, fname, ext);
	_makepath(path, buf, dir, "SCRATCH", ".CUB");
	if (!wflag && !askflag && hW != gr.gr_wm)
		for (i = 0; !(askflag = !(i < 10)); i++) {
			strcpy(fname, "SCRATCH0"), fname[7] += i;
			_makepath(path, buf, dir, fname, ".CUB");
			w = gr.gr_wm;
			while ((w = WX(w)->wx_next) != gr.gr_wm &&
				strcmp(WX(w)->wx_path, path));
			if (w == gr.gr_wm && access(path, 0)) break;
		}
	if (askflag) {
		o.lStructSize = sizeof(OPENFILENAME);
		o.hwndOwner = wflag? hW: gr.gr_wm;
		o.lpstrFilter = "Cube Files\0*.cub\0";
		o.lpstrCustomFilter = NULL;
		o.nFilterIndex = 0;
		o.lpstrFile = path, strcpy(path, wflag? fname: "");
		o.nMaxFile = sizeof(path);
		o.lpstrFileTitle = fname;
		o.nMaxFileTitle = sizeof(fname);
		o.lpstrInitialDir = buf, strcat(buf, dir);
		o.lpstrTitle = NULL;
		o.Flags = OFN_HIDEREADONLY | OFN_NOREADONLYRETURN |
			OFN_PATHMUSTEXIST | (wflag? 0: OFN_CREATEPROMPT);
		o.lpstrDefExt = "cub";
		if (wflag? !GetSaveFileName(&o): !GetOpenFileName(&o))
			return (0);
		w = gr.gr_wm;
		do if (w != hW && !strcmp(WX(w)->wx_path, path)) return (w);
		while ((w = WX(w)->wx_next) != gr.gr_wm);
	}
	if (!wflag || askflag) strcpy(WX(hW)->wx_path, path);
	if (hW != gr.gr_wm) {
		if (*(s = fname) >= 'a' && *s <= 'z') *s += 'A' - 'a';
		while (*++s && *s != '.')
			if (*s >= 'A' && *s <= 'Z') *s += 'a' - 'A';
		strcpy(s, " - HHH"), SetWindowText(hW, fname);
		*s = '\0', strcpy(WX(hW)->wx_name, fname);
	}
	fp = fopen(!wflag || askflag? path: WX(hW)->wx_path, wflag? "w": "r");
	if (fp || !wflag) return (hW);
	sprintf(buf, "Can't save %s!", WX(hW)->wx_name);
	Mess(wflag? hW: gr.gr_wm, buf, NULL, 1);
	return (0);
}

/* save file hW, if mode = 0, silently, 1: save as, 2: ask before closing,
	3: ask before exitting.  return cancelled? */
static int savefile(hW, mode)
HWND hW;
int mode;
{
	char *s, buf[_MAX_PATH];
	int i, n;
	struct wx *wx;
	struct wl *p;
	HWND w;

	if (!(wx = WX(hW))->wx_save && mode != 1) return (0);
	if (mode > 1) {
		if (n = mode == 3 && IsIconic(hW)) ShowWindow(hW, SW_RESTORE);
		if (mode == 3) (void) SetFocus(hW);
		sprintf(buf, "Save %s%s?", hW == gr.gr_wm? "main Cube":
			wx->wx_name, mode == 3?" before exiting": "");
		i = Mess(hW, buf, mode == 2? "Closing": "Exiting", 3);
		if (n && i != IDCANCEL) ShowWindow(hW, SW_MINIMIZE);
		if (i != IDYES) return (i == IDCANCEL);
	}
	if (!(w = fwopen(hW, mode == 1))) return (mode < 2? 0: Mess(hW,
			mode == 2? "Close anyway?": "Exit anyway?",
			mode == 2? "Closing": "Exiting", 2) == IDCANCEL);
	if (w != hW) {
		sprintf(buf, "Can't save %s as %s because\n%s is open "
			"in this window!", WX(hW)->wx_name,
			WX(w)->wx_name, WX(w)->wx_name);
		if (IsIconic(w)) ShowWindow(w, SW_RESTORE);
		(void) SetFocus(w);
		(void) Mess(w, buf, "Save As", 1);
		(void) SetFocus(hW);
		return (0);
	}
	wx->wx_save = 0;
	fputs("CUBE FILE\n", fp);
	if (hW == gr.gr_wm) {
		strcpy(linebuf, linetab), strcat(linebuf, "\n");
		fputs(linebuf, fp), fclose(fp);
		return (0);
	}
	for (p = &wx->wx_item; (p = p->wl_next) != &wx->wx_item;)
		strcpy(linebuf, p->wl_text),
		strcat(linebuf, "\n"), fputs(linebuf, fp);
	fclose(fp);
	return (0);
}

#ifndef SHAREWARE
/* make new file window, open existing file if f, otherwise new */
static void mkfilewin(f)
int f;
{
	HWND hW, w;
	int i, x, y, cx, cy;
	struct wx *wx;
	char *s, buf[_MAX_PATH];
	RECT r;

	hW = CreateWindow("HHH File", "File - HHH", WS_THICKFRAME |
		WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0,
		LoadMenu(hInstance, MAKEINTRESOURCE(MENU_FILE)),
		hInstance, NULL);
	wx = WX(hW);
	wx->wx_item.wl_prev = wx->wx_item.wl_next = &wx->wx_item;
	wx->wx_undo.wl_prev = wx->wx_undo.wl_next = &wx->wx_undo;
	wx->wx_undo0 = wx->wx_undo1 = &wx->wx_item;
	if ((w = fwopen(hW, f)) != hW) {
		(void) SendMessage(hW, WM_CLOSE, 0, 0);
		if (w && IsIconic(w)) ShowWindow(w, SW_RESTORE);
		if (w) (void) SetFocus(w);
		return;
	}
	w = wx->wx_list;
	if (!fp) wx->wx_save = 1, strcpy(linebuf, linetab),
			itemmake(w, -1, &wx->wx_item);
	else if (!fgets(linebuf, BUFLEN, fp) ||
			strcmp(linebuf, "CUBE FILE\n")) {
		sprintf(buf, "%s is not a Cube file!", wx->wx_name);
		(void) Mess(gr.gr_wm, buf, NULL, 1);
		(void) SendMessage(hW, WM_CLOSE, 0, 0), fclose(fp);
	     	return;
	}
	else {
		if (GetPrivateProfileString("FILES", wx->wx_path, "",
				linebuf, BUFLEN, initfile) &&
			sscanf(linebuf, "%d,%d,%d,%d", &x, &y, &cx, &cy) == 4) {
			GetWindowRect(GetDesktopWindow(), &r);
			if (x >= r.left && x + cx < r.right &&
					y >= r.top && y + cy < r.bottom)
				MoveWindow(hW, x, y, cx, cy, 0);
		}
		while (fgets(linebuf, BUFLEN, fp)) {
			for (s = linebuf; *s && *s != '\n'; s++)
				if ((*s & 0xFF) == 0262) *s = '2';
			if (*s == '\n') *s = '\0';
			itemmake(w, -1, &wx->wx_item);
		}
	}
	if (fp) fclose(fp);
	strcpy(wx->wx_item.wl_text, "");
	(void) SendMessage(w, LB_INSERTSTRING, -1, (LPARAM) &wx->wx_item);
	(void) SendMessage(w, LB_SELITEMRANGE, 1, MAKELPARAM(0, 0));
	(void) SendMessage(w, WM_SETFONT, GetStockObject(ANSI_VAR_FONT), 0);
	ShowWindow(hW, SW_SHOW);
}

/* callback function for file window */
static LRESULT CALLBACK cbfilewin(hW, uMsg, wParam, lParam)
HWND hW;
UINT uMsg;
WPARAM wParam;
LPARAM lParam;
{
	RECT r;
	HWND p, q;
	int i, j;
	char *s, buf[_MAX_PATH];
	LPMEASUREITEMSTRUCT lpmis;
	struct wx *wx;

	wx = WX(hW);
	if (uMsg == WM_COMMAND && LOWORD(wParam) == CM_FFC) uMsg = WM_CLOSE;
	switch (uMsg) {
	case WM_COMMAND:
		if (!LOWORD(wParam) && HIWORD(lParam) != LBN_DBLCLK) break;
		switch (LOWORD(wParam)) {
		case IDOK:
		case 0:		dialogop(hW); break;
		case IDCANCEL:
			p = wx->wx_list;
			(void) SendMessage(p, LB_GETSELITEMS, 1, (LPARAM) &i);
			j = SendMessage(p, LB_GETCARETINDEX, 0, 0);
			if (i == j) i++;
			j = SendMessage(p, LB_GETSELCOUNT, 0, 0);
			if (j > 1) (void) SendMessage(wx->wx_list,
				LB_SELITEMRANGE, 0, MAKELPARAM(i, i + j - 2));
			break;
		case CM_FHC:
				dohelp(hW, HELP_CONTENTS, 0L); break;
		case IDHELP:
		case CM_FHF:	dohelp(hW, HELP_CONTEXT, 10L); break;
		case CM_FHO:	dohelp(hW, HELP_CONTEXT, 11L); break;
		case CM_FHI:	dohelp(hW, HELP_KEY, (LONG) ""); break;
		case CM_FHH:
				dohelp(hW, HELP_CONTENTS, 0L);
				WinHelp(hW, hintfile, HELP_CONTEXT,
						(long) hint);
				WinHelp(hW, hintfile, HELP_SETCONTENTS,
						(long) hint + 13L);
				break;
		case CM_FHA:	if (!aboutbox(hW)) break;
				if (editmode)
					SendMessage(gr.gr_we, WM_CLOSE, 0, 0);
				SendMessage(gr.gr_wm, WM_CLOSE, 0, 0);
				break;
		case CM_ACS:
		case CM_FFS:	(void) savefile(hW, 0); break;
		case CM_FFA:	(void) savefile(hW, 1); break;
		case CM_ACZ:
		case CM_FEU:	fileop(hW, 6); break;
		case CM_ACX:
		case CM_FET:	fileop(hW, 1); break;
		case CM_ACC:
		case CM_FEC:	fileop(hW, 2); break;
		case CM_ACV:
		case CM_FEP:	fileop(hW, 3); break;
		case CM_ADEL:
		case CM_FED:	fileop(hW, 0); break;
		}
		break;
	case WM_INITMENU:	initfilemenu(hW); break;
	case WM_SETFOCUS:	(void) SetFocus(wx->wx_list); break;
	case WM_MEASUREITEM:
		lpmis = (LPMEASUREITEMSTRUCT) lParam;
		lpmis->itemWidth = 70, lpmis->itemHeight = 64;
		break;
	case WM_DRAWITEM:
		drawfile(hW, (LPDRAWITEMSTRUCT) lParam); break;
	case WM_CREATE:
		SetWindowLong(hW, 0, (LONG) (s = malloc(sizeof(struct wx))));
		wx = (struct wx *) s;
		for (i = 0; i < sizeof(struct wx); i++) *s++ = 0;
		p = WX(q = gr.gr_wm)->wx_prev;
		WX(p)->wx_next = hW, wx->wx_prev = p;
		WX(q)->wx_prev = hW, wx->wx_next = q;
		p = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE |
			WS_VSCROLL | LBS_EXTENDEDSEL | LBS_NOTIFY |
			LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hW,
			0, hInstance, NULL);
		wx->wx_list = p;
		break;
	case WM_SIZE:
		p = wx->wx_list;
		GetClientRect(hW, &r);
		MoveWindow(p, r.left, r.top,
			r.right - r.left, r.bottom - r.top, 0);
		InvalidateRect(hW, 0, 1);
		break;
	case WM_CLOSE:
		if (savefile(hW, 2)) return (0);
		if (IsWindowVisible(hW)) {
			ShowWindow(hW, SW_RESTORE);
			GetWindowRect(hW, &r);
			sprintf(linebuf, "%d,%d,%d,%d", r.left, r.top,
				r.right - r.left, r.bottom - r.top);
			WritePrivateProfileString("FILES", wx->wx_path,
				linebuf, initfile);
		}
		itemempty(&wx->wx_item), itemempty(&wx->wx_undo);
		p = wx->wx_prev, q = wx->wx_next;
		WX(p)->wx_next = q, WX(q)->wx_prev = p;
		free((char *) wx);
		(void) DestroyWindow(hW);
		break;
	default:return (DefWindowProc(hW, uMsg, wParam, lParam));
	}
	return (1);
}

/* do the big gray out on the menu in file window hW */
static void initfilemenu(hW)
HWND hW;
{
	static int men[] = { CM_FFS, CM_FEU, CM_FET, CM_FEC, CM_FEP, CM_FED, 0};
	int mask, i, j;
	struct wx *wx;
	HMENU hM;

	mask = 0;
	wx = WX(hW);
	if (wx->wx_save) mask |= 1;
	if (wx->wx_undo0 != wx->wx_undo1 || wx->wx_undo.wl_next != &wx->wx_undo)
		mask |= 2;
	i = SendMessage(wx->wx_list, LB_GETSELCOUNT, 0, 0);
	j = SendMessage(wx->wx_list, LB_GETCOUNT, 0, 0) - 1;
	if (SendMessage(wx->wx_list, LB_GETCARETINDEX, 0, 0) != j) i--;
	if (SendMessage(wx->wx_list, LB_GETSEL, j, 0) > 0) i--;
	if (i) mask |= 0x2C;
	if (IsClipboardFormatAvailable(CF_TEXT)) mask |= 0x10;
	hM = GetMenu(hW);
	sprintf(linebuf, "&Hint %d", hint);
	(void) ModifyMenu(hM, CM_FHH, MF_BYCOMMAND, CM_FHH, linebuf);
	for (i = 0; men[i]; i++)
		EnableMenuItem(hM, men[i], MF_BYCOMMAND |
				(mask >> i & 1? MF_ENABLED: MF_GRAYED));
}

/* initiate/execute commands from file-cube operation dialog box under hW */
void dialogop(hW)
HWND hW;
{
	struct wl *wl;
	int cm, i, j;
	DLGPROC dp;
	char *s, *t;

	wl = (struct wl *) SendMessage(WX(hW)->wx_list, LB_GETITEMDATA,
		SendMessage(WX(hW)->wx_list, LB_GETCARETINDEX, 0, 0), 0);
	if (wl == &WX(hW)->wx_item) cm = CM_FOB;
	else {
		dp = (DLGPROC) MakeProcInstance((FARPROC)cbdialogop, hInstance);
		cm = DialogBox(hInstance, MAKEINTRESOURCE(DIALOG_OP), hW, dp);
/*
		FreeProcInstance((FARPROC) dp);
*/
		if (cm == IDCANCEL) return;
	}
	for (s = wl->wl_text; *s && *s != ':'; s++);
	if (*s) s++; else s = wl->wl_text;
	if (cm == CM_FOB || cm == CM_FOC)
		strcpy(linebuf, linetab), fileop(hW, cm == CM_FOB? 5: 4);
	if (cm == CM_FOF || cm == CM_FOO || cm == CM_FOI) {
		if (IsIconic(gr.gr_wm)) ShowWindow(gr.gr_wm, SW_RESTORE);
		(void) SetFocus(gr.gr_wg), UpdateWindow(gr.gr_wm);
	}
 	if (cm == CM_FOF)
		GSELECT(0, -1), GREPLACE(wl->wl_text), (void) upcube(1, 0);
	if ((cm == CM_FOO || cm == CM_FOI) && *s) {
		for (t = s, i = j = 0; *t; t++)
			if (*t == '(') j++;
			else if (*t == ')') if (j) j--; else i++;
		for (t = linebuf; i-- >= 0; *t++ = '(');
		while (*s) *t++ = *s++;
		while (j-- >= 0) *t++ = ')';
		if (cm == CM_FOI) *t++ = '\'';
		*t = '\0';
		GREPLACE(linebuf), (void) upcube(1, 1);
	}
}

/* callback function for file-cube operation dialog window */
static LRESULT CALLBACK cbdialogop(hW, uMsg, wParam, lParam)
HWND hW;
UINT uMsg;
WPARAM wParam;
LPARAM lParam;
{
	static int last = { CM_FOB }, ctrls[] = { CM_FOB, CM_FOC,
			CM_FOF, CM_FOO, CM_FOI, IDOK, IDCANCEL, IDHELP, 0 };
	int i;

	(void) lParam;
	if (uMsg == WM_INITDIALOG) {
		CheckDlgButton(hW, last, 1);
		for (i = 0; ctrls[i]; i++)
			SendDlgItemMessage(hW, ctrls[i], WM_SETFONT,
				GetStockObject(ANSI_VAR_FONT), 0);
		MessageBeep(MB_OK);
	}
	else if (uMsg == WM_CTLCOLOR) {
		SetBkColor(LOWORD(wParam), GetSysColor(COLOR_MENU));
		SetTextColor(LOWORD(wParam), GetSysColor(COLOR_MENUTEXT));
		return (gr.gr_brm);
	}
	else if (uMsg != WM_COMMAND) return (0);
	else switch (LOWORD(wParam)) {
	case IDOK:	EndDialog(hW, last); break;
	case IDCANCEL:	EndDialog(hW, IDCANCEL); break;
	case IDHELP:	dohelp(hW, HELP_CONTEXT, 11L);
			break;
	default:	last = LOWORD(wParam); return (0);
	}
	return (1);
}

/* initialize iconmap to facelet/color indices per bitmap */
void mkiconmap(void)
{
	static int x0[] = { 0x1F, 7, 1, 0, 0, 0, 0, 0x20, 0x38, 0x3E },
		x1[] = { 0, 0, 0x18, 0x1E, 0x1E, 0x1E, 0x1E, 6, 0, 0 },
		y0[] = { 0x3E, 0x38, 0x20, 0, 0, 0, 0, 1, 7, 0x1F },
		y1[] = { 0, 0, 6, 0x1E, 0x1E, 0x1E, 0x1E, 0x18, 0, 0 },
		z0[] = { 0x7DF, 0x707, 0x401, 0, 0x401, 0x707, 0x7DF },
		z1[] = { 0, 0x20, 0xF8, 0x3FE, 0xF8, 0x20, 0 };
	int i, j, p, q, x, y;
	char (*map)[64] = { (char **) iconmap };

	for (i = 0; i < 64; i++) for (j = 0; j < 64; j++) map[i][j] = 54;
	for (i = 2; i < 62; i++) for (j = 0; j < 64; j++) map[i][j] = 55;
	for (p = 0; p < 10; p++) for (q = 0; q < 6; q++) if (~x0[p] >> q & 1)
		for (i = 0; i < 3; i++) for (j = 0; j < 3; j++)
			x = 25 - 5 * j + q, y = 29 - 3 * j + 6 * i - p,
			map[y][x] = x1[p] >> q & 1? i * 3 + j: 54,
			map[y + 9][x + 18] = x1[p] >> q & 1? i * 3 + j + 9: 54;
	for (p = 0; p < 10; p++) for (q = 0; q < 6; q++) if (~y0[p] >> q & 1)
		for (i = 0; i < 3; i++) for (j = 0; j < 3; j++)
			x = 5 * i + q, y = 29 + 6 * j - 3 * i - p,
			map[y][x] = y1[p] >> q & 1? i * 3 + j + 18: 54,
			map[y + 9][x + 48] = y1[p] >> q & 1? i * 3 + j + 27: 54;
	for (p = 0; p < 7; p++) for (q = 0; q < 11; q++) if (~z0[p] >> q & 1)
		for (i = 0; i < 3; i++) for (j = 0; j < 3; j++)
			x = 10 + 5 * j - 5 * i + q, y = 50 - 3 * j - 3 * i - p,
			map[y][x] = z1[p] >> q & 1? i * 3 + j + 36: 54,
			map[y -18][x + 33] = z1[p] >> q & 1? i * 3 + j + 45: 54;
}

/* draw file item (cube and text) for file window hW and info p */
void drawfile(hW, p)
HWND hW;
LPDRAWITEMSTRUCT p;
{
	RECT r;
	char *s, *t, c;
	int foc, sel, i, n, *f;
	COLORREF ob, ot;
	struct wl *wl;

	wl = (struct wl *) p->itemData;
	foc = p->itemState & ODS_FOCUS;
	sel = !foc && p->itemState & ODS_SELECTED && wl != &WX(hW)->wx_item;
	r = p->rcItem;
	if (p->itemAction & ODA_DRAWENTIRE && wl != &WX(hW)->wx_item) {
		f = wl->wl_face;
		for (s = iconbuf, t = iconmap; t < iconmap + 4096;
			*s++ = f[*t++]);
		SetDIBitsToDevice(p->hDC, r.left, r.top, 64,
			64, 0, 0, 0, 64, (void FAR *) iconbuf,
			&iconbi, DIB_RGB_COLORS);
	}
	r.left += 66;
	if (foc || sel)
		ob = SetBkColor(p->hDC, foc? gr.gr_clwt: gr.gr_clh),
		ot = SetTextColor(p->hDC, foc? gr.gr_clw: gr.gr_clht);
	n = (r.bottom - r.top + gr.gr_tm.tmDescent) / gr.gr_tm.tmHeight;
	strcpy(iconbuf, *wl->wl_text? wl->wl_text: "   ");
	for (s = iconbuf; *s && *s != ':'; s++);
	if (!*s) s = iconbuf;
	if (*iconbuf) while (*++s)
		if (*s == '2' && !isdigit(s[-1]) && !isdigit(s[1])) *s = 0262;
	for (s = t = iconbuf; *s && --n >= 0;
				s = t, r.top += gr.gr_tm.tmHeight) {
		r.right -= gr.gr_cw[' '];
		for (i = r.left; i < r.right && *t; i += gr.gr_cw[*t++ & 0xFF]);
		if (i >= r.right) {
			i -= gr.gr_cw[*--t & 0xFF];
			if (!n && t - 3 > iconbuf) strcpy(t - 3, "...");
		}
		r.right += gr.gr_cw[' '], c = *t, *t++ = ' ';
		DrawText(p->hDC, s, t - s, &r, DT_NOPREFIX | DT_LEFT), *--t = c;
		if (s == iconbuf) r.left += gr.gr_cw['M'] * 2;
	}
	if (foc || sel)
		(void) SetBkColor(p->hDC, ob),
		(void) SetTextColor(p->hDC, ot);
}

/* make item from linebuf.  insert item before index i in list window l.
	insert linked list item in item list before q */
void itemmake(l, i, q)
HWND l;
int i;
struct wl *q;
{
	struct wl *n;
	char *s;
	int j, tmp[54];

	if (!(s = malloc(sizeof(struct wl) + strlen(linebuf))))
		error("Out of Memory!");
	n = (struct wl *) s;
	for (j = 0; j < 54; j++) n->wl_face[j] = j / 9;
	n->wl_face[54] = 6, n->wl_face[55] = 7;
	faceread(tmp), facewrite(n->wl_face);
	strcpy(n->wl_text, linebuf);
	if (!eqtomove(movebuf, linebuf, BUFLEN))
		for (s = movebuf; *s; domove(*s++));
	faceread(n->wl_face), facewrite(tmp);
	itemins(l, i, q, n);
}

/* insert item before index i in list window l.
	insert linked list item p in item list before q */
void itemins(l, i, q, p)
HWND l;
int i;
struct wl *q, *p;
{
	(p->wl_prev = q->wl_prev)->wl_next = p;
	p->wl_next = q, q->wl_prev = p;
	(void) SendMessage(l, LB_INSERTSTRING, i, (LPARAM) p);
}

/* destroy file window linked list with header p, leave header */
void itemempty(p)
struct wl *p;
{
	struct wl *q, *r;

	for (q = p->wl_next; q != p; r = q, q = q->wl_next, free((char *) r));
	p->wl_next = p->wl_prev = p;
}

/* delete n items starting at index i from list window l;
	move linked list items s0 through s1 to undo list before u1 */
void itemdel(l, i, n, u1, s0, s1)
HWND l;
int i, n;
struct wl *u1, *s0, *s1;
{
	struct wl *u0, *m0, *m1;

	u0 = u1->wl_prev;
	m0 = s0->wl_next, m1 = s1->wl_prev;
	s0->wl_next = s1, s1->wl_prev = s0;
	u0->wl_next = m0, m0->wl_prev = u0;
	m1->wl_next = u1, u1->wl_prev = m1;
	while (n--) (void) SendMessage(l, LB_DELETESTRING, i, 0);
}

/* file operation: op 0: delete, 1 cut, 2 copy, 3 paste, 4 replace with
	linebuf, 5 insert linebuf, 6 undo, on window hW */
void fileop(hW, op)
HWND hW;
int op;
{
	struct wx *wx;
	HWND l;
	int c, c0, c1, i;
	struct wl *p, *p0, *p1, *q, *h;
	char *s, *t;
	HGLOBAL hG;

	l = (wx = WX(hW))->wx_list, h = &wx->wx_item;
	c = SendMessage(l, LB_GETCARETINDEX, 0, 0);
	(void) SendMessage(l, LB_GETSELITEMS, 1, (LPARAM) &c0);
	c1 = c0 + SendMessage(l, LB_GETSELCOUNT, 0, 0);
	p  = (struct wl *) SendMessage(l, LB_GETITEMDATA, c, 0);
	p0 = (struct wl *) SendMessage(l, LB_GETITEMDATA, c0, 0);
	p1 = ((struct wl *) SendMessage(l, LB_GETITEMDATA, c1 - 1, 0))->wl_next;
	if (p1->wl_prev == h && p != h) c1--, p1 = p1->wl_prev,
		(void) SendMessage(l, LB_SELITEMRANGE, 0, MAKELPARAM(c1, c1));
	if (p == p1->wl_prev) p1 = p, c1--;
	else p0 = p0->wl_next, c0++;
	if (op == 4 && p == h) op = 5;
	if (p0 == p1 && (op == 0 && p == h || op == 1 || op == 2))
		return;
	h = &wx->wx_undo;
	if (op == 6 && wx->wx_undo0 == wx->wx_undo1 && h->wl_next == h) return;
	if (op == 1 || op == 2) {
		for (q = p0, i = 1; q != p1; q = q->wl_next)
			i += strlen(q->wl_text) + 2;
		if (!(hG = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,(DWORD)i)))
			return;
		if (!(s = GlobalLock(hG))) { GlobalFree(hG); return; }
		for (q = p0; q != p1; q = q->wl_next, *s++ = '\r', *s++ = '\n')
			strcpy(s, q->wl_text), s += strlen(q->wl_text);
		*s = '\0', GlobalUnlock(hG);
		if (!OpenClipboard(hW)) { GlobalFree(hG); return; }
		if (!EmptyClipboard() || !SetClipboardData(CF_TEXT, hG))
			{ CloseClipboard(), GlobalFree(hG); return; }
		CloseClipboard();
	}
	if (op == 3 && !OpenClipboard(hW)) return;
	if (op == 3 && !((hG =GetClipboardData(CF_TEXT)) && (s=GlobalLock(hG))))
		{ CloseClipboard(); return; }
	(void) SendMessage(l, LB_SELITEMRANGE, 0,
			MAKELPARAM(c < c0? c: c0, c > c1 - 1? c: c1 - 1));
	if (op == 0 || op == 1 || op == 3 && p0 != p1 || op == 4) {
		if (op == 4 || op == 0 && p0 == p1)
			p1 = (p0 = p)->wl_next, c1 = (c0 = c) + 1;
		if (wx->wx_undo0 != wx->wx_undo1 ||
				wx->wx_undo1 != p0 && wx->wx_undo0 != p1)
			itemempty(h);
		itemdel(l, c0, c1 - c0, wx->wx_undo1 == p0? h: h->wl_next,
			p0->wl_prev, p1);
		if (c > c0) c = c0, p = p1;
		wx->wx_undo0 = wx->wx_undo1 = p0 = p1, c1 = c0;
	}
	if ((op == 3 || op == 4 || op == 5) && wx->wx_undo1 != p0)
		itemempty(h), wx->wx_undo0 = wx->wx_undo1 = p0;
	if (op == 4 || op == 5) {
                q = p0->wl_prev, itemmake(l, c0, p0);
		if (wx->wx_undo0 == p0) wx->wx_undo0 = q->wl_next;
	}
	if (op == 3) {
		q = p0->wl_prev;
		while (*s) {
			for (i =0; s[i] && (s[i] !='\r' || s[i+1] !='\n'); i++)
				linebuf[i] = (s[i] & 0xFF) == 0262? '2': s[i];
			linebuf[i] = '\0';
			itemmake(l, c1++, p0), s += i;
			if (s[0] == '\r' && s[1] == '\n') s += 2;
		}
		c = c0 = c1;
		if (wx->wx_undo0 == p0) wx->wx_undo0 = q->wl_next;
		GlobalUnlock(hG), CloseClipboard();
	}
	if (op == 6) {
		q = wx->wx_undo0->wl_prev;
		c0 = SendMessage(l, LB_FINDSTRING, -1, (LPARAM) wx->wx_undo0);
		for (c1 = c0, p0 = h->wl_next; p0 != h; p0 = p1)
			p1 = p0->wl_next, itemins(l, c1++, wx->wx_undo0, p0);
		h->wl_next = h->wl_prev = h;
		if (wx->wx_undo0 != wx->wx_undo1)
			itemdel(l, c1, SendMessage(l, LB_FINDSTRING, -1,
				(LPARAM) wx->wx_undo1) - c1,
				h, wx->wx_undo0->wl_prev, wx->wx_undo1);
		c = c1, wx->wx_undo0 = q->wl_next;
	}
	(void) SendMessage(l, LB_SELITEMRANGE, 1,
			MAKELPARAM(c < c0? c: c0, c > c1 - 1? c: c1 - 1));
	(void) SendMessage(l, LB_SETCARETINDEX, c, 0);
	if (op != 2) wx->wx_save = 1;
}
#endif

/* set up everything for main window, general initialization, return ok */
static int mkmainwin(void)
{
	extern char **_argv;
	RECT r;
	int i, hi, lo, btab[16], gtab[16], rtab[16], t[5];
	BITMAP bm;
	char drive[_MAX_PATH], dir[_MAX_PATH], fname[_MAX_PATH], ext[_MAX_PATH];
	char *s;
	static int cmid[] = { CM_EPB, CM_EPW, CM_EPO, CM_EPR, CM_EPY, CM_EPG };
	static int col[] = { 14, 15, 4, 6, 1, 2, 0, 8 };
	static int curs[] = { CURSOR_R0, CURSOR_L0, CURSOR_F0, CURSOR_B0,
		CURSOR_U0, CURSOR_D0, CURSOR_R1, CURSOR_L1,
		CURSOR_F1, CURSOR_B1, CURSOR_U1, CURSOR_D1 };
	COLORREF c;

	gr.gr_ca = LoadCursor(NULL, IDC_ARROW);
	for (i = 0; i < 12; i++)
		gr.gr_cs[i] = LoadCursor(hInstance, MAKEINTRESOURCE(curs[i]));
	gr.gr_i = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_HHH));
	for (i = 0; i < 16; i++) {
		lo = i == 8? 0x80: 0, hi = i == 7? 0xC0: i < 8? 0x80: 0xFF;
		c = RGB(rtab[i] = i&4? hi: lo,
				gtab[i] = i&2? hi: lo, btab[i] = i&1? hi: lo);
		if ((gr.gr_br[i] = CreateSolidBrush(c)) == NULL) return (0);
	}
	for (i = 0; i < 8; i++)
		iconbi.bi_q[i].rgbBlue = btab[col[i]],
		iconbi.bi_q[i].rgbGreen= gtab[col[i]],
		iconbi.bi_q[i].rgbRed  = rtab[col[i]],
		iconbi.bi_q[i].rgbReserved = 0;
	gr.gr_brm = CreateSolidBrush(GetSysColor(COLOR_MENU));
	gr.gr_clh  = GetSysColor(COLOR_HIGHLIGHT);
	gr.gr_clht = GetSysColor(COLOR_HIGHLIGHTTEXT);
	gr.gr_clw  = GetSysColor(COLOR_WINDOW);
	gr.gr_clwt = GetSysColor(COLOR_WINDOWTEXT);
	if (!regwin(CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS,
			cbmainwin, "HHH Main"))
		return (0);
	if (!regwin(CS_HREDRAW | CS_VREDRAW | CS_OWNDC, cbeditwin, "HHH Edit"))
		return (0);
#ifndef SHAREWARE
	if (!regwin(CS_DBLCLKS, cbfilewin, "HHH File")) return (0);
#endif
	if (!(gr.gr_wm = CreateWindow("HHH Main", "Hungarian HexaHedron",
		WS_THICKFRAME |
		WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0,
		LoadMenu(hInstance, MAKEINTRESOURCE(MENU_MAIN)),
		hInstance, NULL))) return(0);
	gr.gr_dm = GetDC(gr.gr_w = gr.gr_wm);
	gr.gr_of = SelectObject(gr.gr_dm, GetStockObject(ANSI_VAR_FONT));
	(void) SendMessage(gr.gr_wg,WM_SETFONT,GetStockObject(ANSI_VAR_FONT),0);
	(void) GetTextMetrics(gr.gr_dm, &gr.gr_tm);
	(void) GetCharWidth(gr.gr_dm, 0, 255, gr.gr_cw);
	if (!(gr.gr_dd = CreateCompatibleDC(gr.gr_dm))) return (0);
	(void) SetBkColor(gr.gr_dm, GetSysColor(COLOR_MENU));
	(void) SetTextColor(gr.gr_dm, GetSysColor(COLOR_MENUTEXT));
	gr.gr_obr = SelectObject(gr.gr_dd, gr.gr_br[gr.gr_cl = 0]);
	gr.gr_op = SelectObject(gr.gr_dd, GetStockObject(NULL_PEN));
/*
	gr.gr_ax = GetDeviceCaps(gr.gr_dm, ASPECTX);
	gr.gr_ay = GetDeviceCaps(gr.gr_dm, ASPECTY);
	(void) SetMapMode(gr.gr_dm, MM_ANISOTROPIC);
*/
	gr.gr_bx = 1, gr.gr_by = 1;
	if (!(gr.gr_bm = CreateCompatibleBitmap(gr.gr_dm, gr.gr_bx, gr.gr_by)))
		return (0);
	gr.gr_obm = SelectObject(gr.gr_dd, gr.gr_bm);
/*
	(void) SetWindowOrgEx(gr.gr_dd, 0, 58, NULL);
*/
	gr.gr_mc = CreatePopupMenu();
	for (i = 0; i < 6; i++)
		AppendMenu(gr.gr_mc, MF_OWNERDRAW, cmid[i], (LPCSTR) i);
	gr.gr_bc = LoadBitmap(hInstance, MAKEINTRESOURCE(BITMAP_COLOR));
	(void) GetObject(gr.gr_bc, sizeof(BITMAP), &bm);
	gr.gr_bcx = bm.bmWidth / 2, gr.gr_bcy = bm.bmHeight / 6;
	_splitpath(*_argv, drive, dir, fname, ext);
	_makepath(helpfile, drive, dir, fname, ".HLP");
	_makepath(hintfile, drive, dir, fname, ".HNT");
	_makepath(initfile, drive, dir, fname, ".INI");
	strcpy(WX(gr.gr_wm)->wx_name, "Scratch");
	*movetab = 0, *linetab = 0;
	(void) fwopen(gr.gr_wm, 0);
	if (!fp || !fgets(linebuf, BUFLEN, fp) || strcmp(linebuf, "CUBE FILE\n")
			|| !fgets(linebuf, BUFLEN, fp))
		SetWindowText(gr.gr_wg, "Scratch:"), (void) upcube(1, 0);
	else {
		if ((i = strlen(linebuf)) && linebuf[i-1] == '\n')
			linebuf[i-1] = '\0';
		SetWindowText(gr.gr_wg, linebuf), (void) upcube(1, 0);
		WX(gr.gr_wm)->wx_save = 0;
	}
	if (fp) fclose(fp);
	if (GetPrivateProfileString("FILES", WX(gr.gr_wm)->wx_path, "",
			linebuf, BUFLEN, initfile) &&
			sscanf(linebuf, "%d,%d,%d,%d", t, t+1, t+2, t+3) == 4) {
		GetWindowRect(GetDesktopWindow(), &r);
		if (t[0] >= r.left && t[0] + t[2] < r.right &&
				t[1] >= r.top && t[1] + t[3] < r.bottom)
			MoveWindow(gr.gr_wm, t[0], t[1], t[2], t[3], 0);
	}
	if (GetPrivateProfileString("OPTIONS", "TIMES", "",
			linebuf, BUFLEN, initfile) && sscanf(linebuf,
			"%d,%d,%d,%d,%d", t, t+1, t+2, t+3, t+4) == 5)
		for (i = 0; i < 5; i++) timer.t_limits[i] = t[i];
	hint = GetPrivateProfileInt("OPTIONS", "HINT", 0, initfile);
	return (1);
}

/* vigorously massage WinHelp() call */
static void dohelp(hW, cmd, data)
HWND hW;
UINT cmd;
DWORD data;
{
#ifdef SHAREWARE
	char buf[_MAX_PATH + 100], *s, *t;
	int i;

#else
	if (cmd == HELP_CONTENTS) cmd = HELP_CONTEXT, data = 3L;
#endif
	WinHelp(hW, helpfile, cmd, data);
	if (cmd == HELP_QUIT) return;
#ifdef SHAREWARE
	if (cmd == HELP_CONTEXT && data == 1L) return;
	strcpy(buf, "CreateButton(\"order\", \"&Order\", \"JumpId(`");
	for (s = buf + strlen(buf), t = helpfile; *t; t++)
		if (*t == '\\') for (i = 0; i < 4; i++) *s++ = '\\';
		else *s++ = *t;
	strcpy(s, "', `order')\")");
	WinHelp(gr.gr_wm, helpfile, HELP_COMMAND, (DWORD) (LPSTR) buf);
#else
	WinHelp(hW, helpfile, HELP_SETCONTENTS, 3L);
#endif
}

/* clean up mess generated by mkmainwin() */
static void rmmainwin(void)
{
	int i;

	(void) DestroyMenu(gr.gr_mc);
	DeleteObject(gr.gr_bc);
	(void) SelectObject(gr.gr_dd, gr.gr_obm);
	(void) DeleteObject(gr.gr_bm);
	(void) SelectObject(gr.gr_dd, gr.gr_op);
	(void) SelectObject(gr.gr_dd, gr.gr_obr);
	(void) DeleteDC(gr.gr_dd);
	(void) SelectObject(gr.gr_dm, gr.gr_of);
	(void) DestroyWindow(gr.gr_wm);
	(void) UnregisterClass("HHH Edit", hInstance);
	(void) UnregisterClass("HHH Main", hInstance);
	for (i = 0; i < 16; i++) (void) DeleteObject(gr.gr_br[i]);
	(void) DeleteObject(gr.gr_brm);
	(void) DestroyIcon(gr.gr_i);
	for (i = 0; i < 12; i++) DestroyCursor(gr.gr_cs[i]);
}

/* callback function for main window */
static LRESULT CALLBACK cbmainwin(hW, uMsg, wParam, lParam)
HWND hW;
UINT uMsg;
WPARAM wParam;
LPARAM lParam;
{
	int i, x, y, cx, cy;
	char *s;
	RECT r;
	HWND p;
	static int takefocus = { 0 };

	if (uMsg == WM_COMMAND && LOWORD(wParam) == CM_MFX) uMsg = WM_CLOSE;
	switch (uMsg) {
	case WM_COMMAND:	cbmainmenu(LOWORD(wParam),HIWORD(lParam));break;
	case WM_INITMENU:	initmainmenu(); break;
	case WM_KEYUP:
			if (wParam != VK_CONTROL) break;
			wParam = 0, lParam = 0; /* look out below! */
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
			takefocus = wParam & MK_CONTROL;
			cbmainmouse(wParam, lParam); break;
	case WM_SETFOCUS:
		if (!editmode && !takefocus) (void) SetFocus(gr.gr_wg);
		break;
	case WM_CREATE:
		SetWindowLong(hW, 0, (LONG) (s = malloc(sizeof(struct wx))));
		for (i = 0; i < sizeof(struct wx); i++) *s++ = 0;
		WX(hW)->wx_prev = WX(hW)->wx_next = hW;
		gr.gr_wg = CreateWindow("EDIT", "", ES_AUTOHSCROLL |
			ES_NOHIDESEL | WS_CHILD | WS_VISIBLE | WS_BORDER,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hW, 0,
			hInstance, NULL);
		break;
	case WM_PAINT:
		if (editmode) gr.gr_w = gr.gr_wm,
			faceread(gr.gr_fe), facewrite(gr.gr_fm);
		(void) docube(0, 0, 0x10000L, 0L, 0x10000L, 0L);
		if (editmode) gr.gr_w = gr.gr_we, facewrite(gr.gr_fe);
		break;
	case WM_SIZE:
		if (!IsZoomed(gr.gr_wm)) GetWindowRect(gr.gr_wm, &gr.gr_r);
		gr.gr_wx = LOWORD(lParam);
		if ((gr.gr_wy = HIWORD(lParam) - GENHEIGHT) <= 0) gr.gr_wy = 1;
		cx = LOWORD(lParam) - GENMARGIN - GENINDENT, x = GENMARGIN;
		cy = GENHEIGHT - GENMARGIN - GENMARGIN, y = GENMARGIN;
		if (GENHEIGHT >= HIWORD(lParam))
			cy = HIWORD(lParam) - 1, y = 0, cx = 0;
		if (cy < 1) cy = 1;
		if (cx < GENINDENT) cx = LOWORD(lParam), x = 0;
		MoveWindow(gr.gr_wg, LOWORD(lParam) - cx - x,
			HIWORD(lParam) - cy - y, cx, cy, 1);
		SendMessage(gr.gr_wg, EM_SETSEL, 0,
			SendMessage(gr.gr_wg, EM_GETSEL, 0, 0));
		InvalidateRect(gr.gr_wm, 0, 1);
		break;
	case WM_GETMINMAXINFO:
		GetWindowRect(gr.gr_w, &r), i = r.bottom - r.top;
		GetClientRect(gr.gr_w, &r), i -= r.bottom - r.top;
		((MINMAXINFO FAR *) lParam)->ptMinTrackSize.y =  i+2*GENHEIGHT;
		return (0);
	case WM_CLOSE:
		if (upcube(2, 0)) return (0);
		if (savefile(p = hW, 3)) return (0);
		while ((p = WX(p)->wx_next) != hW) if (savefile(p, 3)) break;
		if (p != hW) return (0);
		while ((p = WX(hW)->wx_next) != hW)
			WX(p)->wx_save = 0,
			(void) SendMessage(p, WM_CLOSE, 0, 0);
		if (IsWindowVisible(hW)) {
			ShowWindow(hW, SW_RESTORE);
			GetWindowRect(hW, &r);
			sprintf(linebuf, "%d,%d,%d,%d", r.left, r.top,
				r.right - r.left, r.bottom - r.top);
			WritePrivateProfileString("FILES", WX(hW)->wx_path,
				linebuf, initfile);
		}
		sprintf(linebuf, "%d,%d,%d,%d,%d", timer.t_limits[0],
			timer.t_limits[1], timer.t_limits[2],
			timer.t_limits[3], timer.t_limits[4]);
		WritePrivateProfileString("OPTIONS", "TIMES",linebuf, initfile);
		sprintf(linebuf, "%d", hint);
		WritePrivateProfileString("OPTIONS", "HINT", linebuf, initfile);
		free((char *) WX(hW));
		dohelp(hW, HELP_QUIT, 0L);
		PostQuitMessage(0);
		break;
	default:return (DefWindowProc(hW, uMsg, wParam, lParam));
	}
	return (1);
}

static int hintcm[13] = { CM_MHH0, CM_MHH1, CM_MHH2, CM_MHH3, CM_MHH4, CM_MHH5,
	CM_MHH6, CM_MHH7, CM_MHH8, CM_MHH9, CM_MHH10, CM_MHH11, CM_MHH12 };
static int timecm[5][11] = {
	{ CM_MSTA0, CM_MSTA1, CM_MSTA2, CM_MSTA5, CM_MSTA10, CM_MSTA20,
	CM_MSTA50, CM_MSTA100, CM_MSTA200, CM_MSTA500, CM_MSTAU },
	{ CM_MSTO0, CM_MSTO1, CM_MSTO2, CM_MSTO5, CM_MSTO10, CM_MSTO20,
	CM_MSTO50, CM_MSTO100, CM_MSTO200, CM_MSTO500, CM_MSTOU },
	{ CM_MSTR0, CM_MSTR1, CM_MSTR2, CM_MSTR5, CM_MSTR10, CM_MSTR20,
	CM_MSTR50, CM_MSTR100, CM_MSTR200, CM_MSTR500, CM_MSTRU },
	{ CM_MSTS0, CM_MSTS1, CM_MSTS2, CM_MSTS5, CM_MSTS10, CM_MSTS20,
	CM_MSTS50, CM_MSTS100, CM_MSTS200, CM_MSTS500, CM_MSTSU },
	{ CM_MSTE0, CM_MSTE1, CM_MSTE2, CM_MSTE5, CM_MSTE10, CM_MSTE20,
	CM_MSTE50, CM_MSTE100, CM_MSTE200, CM_MSTE500, CM_MSTEU } };
static int timeno[] = { 0,1,2,5,10,20,50,100,200,500,-1 };

/* callback sub-function for (menu) command cm and notify ncm for main window */
static void cbmainmenu(cm, ncm)
int cm, ncm;
{
	static int lock = { 0 };
	int flag, i, j, f, q;
	HWND p;
	char *s, *t;
	DWORD d;
	HGLOBAL hG;

	if (!cm && ncm == EN_CHANGE) upcube(0, 0);
	if (!cm && ncm == EN_UPDATE) {
		if (lock) return; else lock = 1;
		(void) GGETTEXT();
		for (t = linebuf; *t && *t != ':'; t++);
		if (!*t) t = linebuf;
		for (s = linebuf; *s; s++)
			if ((*s & 0xFF) == (s == linebuf || s < t ||
				isdigit(s[-1]) || isdigit(s[1])? 0262: '2')) {
				d = SendMessage(gr.gr_wg, EM_GETSEL, 0, 0);
				GSELECT(s - linebuf, s + 1 - linebuf);
				GREPLACE((char *) (*s == '2'? "\262": "2"));
				(void) SendMessage(gr.gr_wg, EM_SETSEL, 0, d);
		}
		lock = 0;
	}
	flag = 1;
	switch (cm) {
	case 0:		return;
	case IDOK:	(void) upcube(1, 1); break;
	case CM_TAB:	(void) upcube(1, 0); break;
	case IDCANCEL:	(void) upcube(3, 1); break;
	case IDHELP:
	case CM_MHC:
			dohelp(gr.gr_wm, HELP_CONTENTS, 0L); break;
	case CM_MHT:	dohelp(gr.gr_wm, HELP_CONTEXT, 5L); break;
	case CM_MHM:	dohelp(gr.gr_wm, HELP_CONTEXT, 6L); break;
	case CM_MHG:	dohelp(gr.gr_wm, HELP_CONTEXT, 8L); break;
	case CM_MHS:	dohelp(gr.gr_wm, HELP_CONTEXT, 9L); break;
	case CM_MHI:	dohelp(gr.gr_wm, HELP_KEY, (long) ""); break;
	case CM_MHH:
#ifdef SHAREWARE
			aboutbox(gr.gr_wm);
#else
			dohelp(gr.gr_wm, HELP_CONTENTS, 0L);
			WinHelp(gr.gr_wm, hintfile, HELP_CONTEXT, (long) hint);
			WinHelp(gr.gr_wm, hintfile, HELP_SETCONTENTS,
					(long) hint + 13L);
#endif
			break;
	case CM_MHA:	if (!aboutbox(gr.gr_wm)) break;
			if (editmode) SendMessage(gr.gr_we, WM_CLOSE, 0, 0);
			SendMessage(gr.gr_wm, WM_CLOSE, 0, 0);
			break;
	case CM_ACZ:
	case CM_MEU:	(void) SendMessage(gr.gr_wg, EM_UNDO,  0, 0); break;
	case CM_ACX:
	case CM_MET:
	case CM_ACC:
	case CM_MEC:
		d = SendMessage(gr.gr_wg, EM_GETSEL, 0, 0);
		if (!(i = HIWORD(d) - LOWORD(d))) break;
		if (!(hG = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,(DWORD)i)))
			break;
		if (!(s = GlobalLock(hG))) { GlobalFree(hG); break; }
		(void) GGETTEXT(), t = linebuf + LOWORD(d);
		while (i--) *s++ = (*t & 0xFF) == 0262? '2': *t, t++;
		*s = '\0', GlobalUnlock(hG);
		if (!OpenClipboard(gr.gr_wg)) { GlobalFree(hG); break; }
		if (!EmptyClipboard() || !SetClipboardData(CF_TEXT, hG))
			{ CloseClipboard(), GlobalFree(hG); break; }
		CloseClipboard();
		if (cm == CM_ACX || cm == CM_MET) GREPLACE("");
		break;
	case CM_ACV:
	case CM_MEP:	(void) SendMessage(gr.gr_wg, WM_PASTE, 0, 0); break;
	case CM_ADEL:   d = SendMessage(gr.gr_wg, EM_GETSEL, 0, 0);
			if (LOWORD(d) == HIWORD(d)) (void) SendMessage(
					gr.gr_wg, EM_SETSEL, 0, d + 0x10000L);
	case CM_MED:	(void) SendMessage(gr.gr_wg, WM_CLEAR, 0, 0); break;
	default:	if (!upcube(2, 1)) flag = 0;
	}
	if (flag) return;
	switch (cm) {
#ifdef SHAREWARE
	case CM_MFN:
	case CM_MFO:	aboutbox(gr.gr_wm); break;
#else
	case CM_MFN:	mkfilewin(0); break;
	case CM_MFO:	mkfilewin(1); break;
#endif
	case CM_ACS:
	case CM_MFS:	(void) savefile(gr.gr_wm, 0); break;
	case CM_MFL:
		p = gr.gr_wm;
		do (void) savefile(p, 0);
		while ((p = WX(p)->wx_next) != gr.gr_wm);
		break;
	case CM_MEE:	editinit(); break;
	case CM_MSA:	facetoeq(0, timer.t_limits[0]); break;
	case CM_AF10:
	case CM_MSO:	facetoeq(1, timer.t_limits[1]); break;
	case CM_MSR:    if (Mess(gr.gr_wm,
				"Sure you want the current generator replaced?",
				"Regenerate", 2) == IDCANCEL) break;
			facetoeq(-1, timer.t_limits[2]);
			break;
	case CM_MSS:    if (Mess(gr.gr_wm,
				"Sure you want to scramble the cube?",
				"Scramble", 2) == IDCANCEL) break;
			for (i = 0; i < 500; i++) domove(nrnd(24));
			facetoeq(-1, timer.t_limits[3]);
			break;
	default:i = cm / 100 % 10, f = cm / 10 % 10, q = cm % 10;
		if (i == 5) i = 0, f ^= 1, q = 4 - q;
		if (cm / 1000 == 1 && i < 5 && f < 6 && q && q < 4)
			catmove(i * 24 + 4 * f + q), animate(i, f * 4 + q);
		for (i = 0; i < 5; i++) for (j = 0; j < 11; j++)
			if (timecm[i][j] == cm)
#ifdef SHAREWARE
				aboutbox(gr.gr_wm);
#else
				timer.t_limits[i] = timeno[j];
#endif
		for (i = 0; i < 13; i++) if (hintcm[i] == cm) {
#ifdef SHAREWARE
			aboutbox(gr.gr_wm);
#else
			sprintf(linebuf,
			"Sure you want to change from hint %d %s to hint %d?",
			hint, hint < i? "up": "down", i);
			if (hint != i && Mess(gr.gr_wm, linebuf,"Change Hint",2)
					== IDCANCEL) break;
			hint = i;
			dohelp(gr.gr_wm, HELP_CONTENTS, 0L);
			WinHelp(gr.gr_wm, hintfile, HELP_CONTEXT, (long) hint);
			WinHelp(gr.gr_wm, hintfile, HELP_SETCONTENTS,
					(long) hint + 13L);
#endif
		}
		break;
	}
}

/* do the big gray out on the menu in the main window */
static void initmainmenu(void)
{
	static int men[] = { CM_MFS, CM_MFL, CM_MEU, CM_MET, CM_MEC,
		CM_MEP, CM_MED, CM_MSA, CM_MSO, CM_MSR,
		IDOK, IDCANCEL, CM_TAB, 0 };
	int mask, i, j;
	char *s;
	HWND p;
	HMENU hM;

	mask = 0;
	if (genmode) mask |= 0x1F83;
	if (WX(gr.gr_wm)->wx_save) mask |= 3;
	p = WX(gr.gr_wm)->wx_next;
	while ((p = WX(p)->wx_next) != gr.gr_wm && !WX(p)->wx_save);
	if (p != gr.gr_wm) mask |= 2;
	if (SendMessage(gr.gr_wg, EM_CANUNDO, 0, 0)) mask |= 4;
	if (SendMessage(gr.gr_wg, EM_GETSEL, 0, 0) % 0x10001L) mask |= 0x58;
	if (IsClipboardFormatAvailable(CF_TEXT)) mask |= 0x20;
	faceread(gr.gr_fm);
	for (i = 0; i < 54 && gr.gr_fm[i] == i / 9; i++);
	if (i < 54) mask |= 0x180;
	for (s = linetab; *s && *s != ':'; s++);
	if (*s? s[1]: *linetab) mask |= 0x200;
	hM = GetMenu(gr.gr_wm);
	for (i = 0; i < 5; i++) for (j = 0; j < 11; j++)
		CheckMenuItem(hM, timecm[i][j], MF_BYCOMMAND |
				(timeno[j] == timer.t_limits[i]?
				MF_CHECKED: MF_UNCHECKED));
	for (i = 0; i < 13; i++)
		CheckMenuItem(hM, hintcm[i], MF_BYCOMMAND |
				(i == hint? MF_CHECKED: MF_UNCHECKED));
	sprintf(linebuf, "&Hint %d", hint);
	(void) ModifyMenu(hM, CM_MHH, MF_BYCOMMAND, CM_MHH, linebuf);
	for (i = 0; men[i]; i++)
		EnableMenuItem(hM, men[i], MF_BYCOMMAND |
				(mask >> i & 1? MF_ENABLED: MF_GRAYED));
}

/* if flag == 0, initiate generator mode if cube out of date.
	if flag != 0, terminate generator mode.
	if flag = 2, ask first Yes, No or Cancel, return cancelled?
	if flag = 3, ask first OK, Cancel, OK means no, return cancelled?
	if anflag animate change */
static int upcube(flag, anflag)
int flag, anflag;
{
	static UINT msgs[] = { WM_CLOSE, WM_KEYDOWN, WM_SYSKEYDOWN,
		WM_LBUTTONDOWN, WM_MBUTTONDOWN, WM_RBUTTONDOWN,
		WM_NCLBUTTONDOWN, WM_NCMBUTTONDOWN, WM_NCRBUTTONDOWN, 0 };
	char *s, *t;
	int i, oldmode;
	HWND p;
	MSG msg;

	if (!genmode && flag) return (0);
	(void) GGETTEXT();
	for (s = linebuf; *s; s++) if ((*s & 0xFF) == 0262) *s = '2';
	for (s = movetab; *s; s++);
	for (t = movebuf; s > movetab; --s, *t++ = *s ^ (*s << 1 & 2));
	if (eqtomove(t, linebuf, movebuf + BUFLEN - t)) return (0);
	for (s = t = movebuf; *s; s++)
		if (t == movebuf || (t[-1] & ~3 ^ *s & ~3)) *t++ = *s;
		else if (!(t[-1] = t[-1] + *s & 3)) --t;
		else t[-1] |= *s & ~3;
	i = IDYES;
	if (flag == 3 && t != movebuf)
		if (Mess(gr.gr_w, "Abandoning edited Generator.",
			"Generator Edited", 2) == IDCANCEL) return (1);
		else i = IDNO;
	if (flag == 2 && t != movebuf && (i = Mess(gr.gr_w,
			"Update main Cube with edited Generator?",
			"Generator Edited", 3)) == IDCANCEL) return (1);
	if (genmode != (!flag && t != movebuf))
		for (p = gr.gr_wm; (p = WX(p)->wx_next) != gr.gr_wm;)
			EnableWindow(p, genmode);
	genmode = !flag && t != movebuf;
	if (i == IDNO) { GSELECT(0, -1), GREPLACE(linetab); return (0); }
	if (!genmode && strcmp(linetab, linebuf))
		WX(gr.gr_wm)->wx_save = 1, strcpy(linetab, linebuf);
#if 1
	if (genmode) return (0);
#else
	if (!flag) return (0);
#endif
	for (s = movebuf; s < t; s++) {
		anflag = anflag && GetActiveWindow() == gr.gr_wm;
		while (anflag && PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			for (p = gr.gr_wm; (p = WX(p)->wx_next) != gr.gr_wm;)
				if (p == msg.hwnd || IsChild(p, msg.hwnd))break;
			for (i = 0; msgs[i]; i++)
				if (msg.message == msgs[i]) break;
			if (anflag = (p == msg.hwnd || IsChild(p, msg.hwnd)) &&
					!msgs[i])
				(void) PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		}
		if (anflag) animate(*s / 24, *s % 24);
		else domove(*s);
	}
	if (!anflag && t != movebuf)
		(void) docube(0, 0, 0x10000L, 0L, 0x10000L, 0L);
	(void) eqtomove(movetab, linebuf, BUFLEN);
	return (0);
}

/* generate solution equation from face colors.  if mode < 0, replace with
	inverse.  if mode == 1, one step only. l is time limit seconds */
void facetoeq(mode, l)
int mode;
{
	static int c1[6][6] = {{0,0,0,2,1,3},{0,0,10,18,10,18},
		{19,17,0,0,9,11},{19,17,0,0,11,9},{19,17,11,9,0,0},
		{19,17,9,11,0,0}};
	static int c2[6][6] = {{0,0,0,0,0,0},{0,0,0,0,1,1},{2,0,0,0,1,3},
		{0,2,0,0,1,3},{3,1,0,2,0,0},{1,3,0,2,0,0}};
	int i;
	char *s, *t;
/*
	DLGPROC dp;
*/
	HWND p;
	MSG msg;

	faceread(gr.gr_fm);
	faceread(gr.gr_fe);
	for (i = 0; i < 54 && gr.gr_fe[i] == i / 9; i++);
	if (mode >= 0 && i == 54) return;
	backgo(0);
#ifdef SHAREWARE
	(void) l;
	s = backsearch(gr.gr_fm, 0, 0);
#else
	if (!(timer.t_limit = l)) s = backsearch(gr.gr_fm, 0, 0);
	else {
/*
		dp = (DLGPROC)MakeProcInstance((FARPROC)cbdialogbusy,hInstance);
*/
		timer.t_hW = CreateDialog(hInstance,
				MAKEINTRESOURCE(DIALOG_BUSY), gr.gr_wm,
				(DLGPROC) cbdialogbusy);
		p = gr.gr_wm;
		do EnableWindow(p, 0); while ((p = WX(p)->wx_next) != gr.gr_wm);
		timer.t_next = (timer.t_tick = GetTickCount()) + 1000L;
		timer.t_move = backstat() & 0xFF;
		backgo(1), timer.t_back = 0;
		s = backsearch(gr.gr_fm, timeserv, 32767);
		do EnableWindow(p, 1); while ((p = WX(p)->wx_next) != gr.gr_wm);
		(void) DestroyWindow(timer.t_hW);
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) /* flush! */
			if (!hA || !TranslateAccelerator(
					GetActiveWindow(), hA, &msg))
				TranslateMessage(&msg), DispatchMessage(&msg);
		(void) SetFocus(gr.gr_wm);
/*
		FreeProcInstance((FARPROC) dp);
*/
	}
#endif
	for (t = movebuf; *s; faceread(gr.gr_fe)) {
		if (t == movebuf || (t[-1] & ~3 ^ *s & ~3)) *t++ = *s;
		else if (!(t[-1] = t[-1] + *s & 3)) --t;
		else t[-1] |= *s & ~3;
		domove(*s++);
	}
	if (i = c1[gr.gr_fe[0]][gr.gr_fe[18]]) *t++ = i + 24;
	if (i = c2[gr.gr_fe[0]][gr.gr_fe[18]]) *t++ = i + 24;
	if (mode >= 0) rparen();
	else {
		for (s = linetab; *s && *s != ':'; s++);
		if (!(*s? s[1]: *linebuf) && t == movetab) return;
		GSELECT(*s? s + 1 - linetab: 0, strlen(linetab));
		for (i = 0; i < 54; i++) gr.gr_fm[i] = i / 9;
		*movetab = 0;
	}
	facewrite(gr.gr_fm);
	s = linebuf, *t = '\0';
	if (!mode || mode < 0 && t != movebuf) *s++ = '(';
	if (mode >= 0) t = movebuf;
	while (mode < 0? t > movebuf: *t) {
		if (mode < 0) --t;
		*s++ = "RLFBUD"[*t%24/4];
		if (*t/24) *s++ = " csma"[*t/24];
		if (mode < 0) *t ^= *t << 1 & 2;
		if (*t & 2) *s++ = "2'"[*t & 1];
		if (!mode) t++;
		if (mode > 0) break;
	}
	if (!mode || mode < 0 && s != linebuf) *s++ = ')';
	*s = '\0', GREPLACE(linebuf), (void) upcube(1, mode >= 0);
}

static void timeserv(void)
{
	MSG msg;
	int i;
	long t;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		if (!hA || !TranslateAccelerator(GetActiveWindow(), hA, &msg))
			TranslateMessage(&msg), DispatchMessage(&msg);
	if (GetTickCount() < timer.t_next) return;
	if (timer.t_next == timer.t_tick + 1000L)
		MessageBeep(MB_OK), ShowWindow(timer.t_hW, SW_SHOW);
	timer.t_next += ((GetTickCount() - timer.t_next) / 1000L + 1L) * 1000L;
	t = (GetTickCount() - timer.t_tick - 500L) / 1000L;
	if (t >= timer.t_limit && timer.t_limit > 0) { backgo(0); return; }
	i = backstat();
	sprintf(linebuf, "Elapsed Time: %ld second%s", t, "s" + (t == 1L));
	SetDlgItemText(timer.t_hW, CM_BT, linebuf);
	if (timer.t_back != i >> 8)
		sprintf(linebuf, "Current Back Search Limit: %d move%s",
			timer.t_back = i >> 8, "s" + (i >> 8 == 1)),
		SetDlgItemText(timer.t_hW, CM_BB, linebuf);
	if (timer.t_move > (i & 0xFF)) {
		timer.t_move = i & 0xFF;
		sprintf(linebuf, "Last, Best Solution: %d move%s", timer.t_move,
			"s" + (timer.t_move == 1));
		SetDlgItemText(timer.t_hW, CM_BM, linebuf);
		sprintf(linebuf, "Found at: %ld second%s", t, "s" + (t == 1L));
		SetDlgItemText(timer.t_hW, CM_BF, linebuf);
	}
}

/* callback function for busy dialog window */
static LRESULT CALLBACK cbdialogbusy(hW, uMsg, wParam, lParam)
HWND hW;
UINT uMsg;
WPARAM wParam;
LPARAM lParam;
{
	static int ctrls[] = { CM_BT, CM_BL, CM_BB, CM_BM, CM_BF, IDCANCEL, 0 };
	int i;

	(void) lParam;
	if (uMsg == WM_INITDIALOG) {
		sprintf(linebuf, timer.t_limit < 0? "Time Limit: None":
			"Time Limit: %d second%s", timer.t_limit,
			"s" + (timer.t_limit == 1));
		SetDlgItemText(hW, CM_BL, linebuf);
		for (i = 0; ctrls[i]; i++)
			SendDlgItemMessage(hW, ctrls[i], WM_SETFONT,
				GetStockObject(ANSI_VAR_FONT), 0);
	}
	else if (uMsg == WM_CTLCOLOR) {
		SetBkColor(LOWORD(wParam), GetSysColor(COLOR_MENU));
		SetTextColor(LOWORD(wParam), GetSysColor(COLOR_MENUTEXT));
		return (gr.gr_brm);
	}
	else if (uMsg == WM_COMMAND &&
		(LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDOK))
		backgo(0);
	else return (0);
	return (1);
}

/* callback function for edit cube window */
static LRESULT CALLBACK cbeditwin(hW, uMsg, wParam, lParam)
HWND hW;
UINT uMsg;
WPARAM wParam;
LPARAM lParam;
{
	LPMEASUREITEMSTRUCT lpmis;
	int i, l, c, cm;
	static int cmap[] = { 4, 1, 3, 2, 0, 5 };
	static int lmap[] = { 2, 3, 0, 5, 1, 4 };
	RECT r;
	HMENU hMl, hMi;

	if (uMsg == WM_COMMAND && LOWORD(wParam) == CM_EC) uMsg = WM_CLOSE;
	switch (uMsg) {
	case WM_INITMENU:
		for (l = 0; l < 6; l++) {
			hMl = GetSubMenu((HMENU) wParam, lmap[l] + 1);
			for (i = 0; i < 9; i++) {
				hMi = GetSubMenu(hMl, i%3*3 + i/3);
				for (c = 0; c < 6; c++) EnableMenuItem(hMi, c,
					MF_BYPOSITION |
					(gr.gr_fe[l * 9 + (l & 1? 8 - i: i)] ==
					cmap[c]? MF_GRAYED: MF_ENABLED));
			}
		}
		sprintf(linebuf, "&Hint %d", hint);
		(void) ModifyMenu((HMENU) wParam, CM_EHH, MF_BYCOMMAND, CM_EHH,
				linebuf);
		break;
	case WM_COMMAND:
		switch (cm = (int) LOWORD(wParam)) {
		case CM_EHC:
				dohelp(hW, HELP_CONTENTS, 0L); break;
		case IDHELP:
		case CM_EHE:	dohelp(hW, HELP_CONTEXT, 7L); break;
		case CM_EHI:	dohelp(hW, HELP_KEY, (LONG) ""); break;
		case CM_EHH:
#ifdef SHAREWARE
				aboutbox(hW);
#else
				dohelp(hW, HELP_CONTENTS, 0L);
				WinHelp(hW, hintfile, HELP_CONTEXT,
						(long) hint);
				WinHelp(hW, hintfile, HELP_SETCONTENTS,
						(long) hint + 13L);
#endif
				break;
		case CM_EHA:	if (!aboutbox(hW)) break;
				if (editmode)
					SendMessage(gr.gr_we, WM_CLOSE, 0, 0);
				SendMessage(gr.gr_wm, WM_CLOSE, 0, 0);
				break;
		case CM_EPY:	cbcolor(0, 0, 0); break;
		case CM_EPW:	cbcolor(1, 0, 0); break;
		case CM_EPR:	cbcolor(2, 0, 0); break;
		case CM_EPO:	cbcolor(3, 0, 0); break;
		case CM_EPB:	cbcolor(4, 0, 0); break;
		case CM_EPG:	cbcolor(5, 0, 0); break;
		default:
			if (cm < 1000 || cm >= 2000) break;
			l = cm / 100 % 10, c = cm % 10;
			i = cm / 10 % 10 - 1, i = i%3*3 + i/3;
			gr.gr_fe[l*9 + (l & 1? 8 - i: i)] = cmap[c];
			facewrite(gr.gr_fe);
			InvalidateRect(gr.gr_w, 0, 0); break;
		}
		break;
	case WM_MEASUREITEM:
		lpmis = (LPMEASUREITEMSTRUCT) lParam;
		lpmis->itemWidth = gr.gr_bcx, lpmis->itemHeight = gr.gr_bcy;
		break;
	case WM_DRAWITEM:
		drawcolor((LPDRAWITEMSTRUCT) lParam); break;
	case WM_LBUTTONDOWN:
		cbcolor(-1, (short) LOWORD(lParam), (short) HIWORD(lParam));
		break;
	case WM_RBUTTONDOWN:
		cbcolor(-2, (short) LOWORD(lParam), (short) HIWORD(lParam));
		break;
	case WM_PAINT:
		(void) docube(0, 0, 0x10000L, 0L, 0x10000L, 0L); break;
	case WM_GETMINMAXINFO:
		GetWindowRect(gr.gr_w, &r), i = r.bottom - r.top;
		GetClientRect(gr.gr_w, &r), i -= r.bottom - r.top;
		((MINMAXINFO FAR *) lParam)->ptMinTrackSize.y =  i+2*GENHEIGHT;
		return (0);
	case WM_CLOSE:
		editterm();
		return (0);
	default:return (DefWindowProc(hW, uMsg, wParam, lParam));
	}
	return (1);
}

/* initiate edit cube mode window */
static void editinit(void)
{
	HWND p;

	if (editmode) return;
	faceread(gr.gr_fm), faceread(gr.gr_fe);
	gr.gr_we = CreateWindow("HHH Edit", "Edit - Hungarian HexaHedron",
		WS_THICKFRAME |
		WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0,
		LoadMenu(hInstance, MAKEINTRESOURCE(MENU_EDIT)),
		hInstance, NULL);
	gr.gr_de = GetDC(gr.gr_w = gr.gr_we);
	gr.gr_ofe = SelectObject(gr.gr_de, GetStockObject(ANSI_VAR_FONT));
	(void) SetBkColor(gr.gr_de, GetSysColor(COLOR_MENU));
	(void) SetTextColor(gr.gr_de, GetSysColor(COLOR_MENUTEXT));
	(void) SetMapMode(gr.gr_de, MM_ANISOTROPIC);
	MoveWindow(gr.gr_w, gr.gr_r.left + GENHEIGHT, gr.gr_r.top + GENHEIGHT,
		gr.gr_r.right - gr.gr_r.left, gr.gr_r.bottom - gr.gr_r.top, 0);
	ShowWindow(gr.gr_w, IsZoomed(gr.gr_wm)? SW_SHOWMAXIMIZED: SW_SHOW);
	p = gr.gr_wm;
	do EnableWindow(p, 0);
	while ((p = WX(p)->wx_next) != gr.gr_wm);
	(void) InvalidateRect(gr.gr_w, 0, 0);
	editmode = 1;
}

/* terminate edit cube mode window */
static void editterm(void)
{
	int i, j, c[6];
	char *s, buf[256];
	HWND p;

	if (!editmode) return;
	for (i = 0; i < 54 && gr.gr_fm[i] == gr.gr_fe[i]; i++);
	for (s = linetab; *s && *s != ':'; s++);
	if (i == 54 && !(*s? s[1]: *linetab)) i = IDYES;
	else {
		for (i = 0; i < 6; i++) c[i] = gr.gr_fe[4+i*9];
		s = 0;
                for (j = 0, i = 0; i < 6 && !(j & 1 << c[i]); j |= 1 << c[i++]);
		if (i < 6) s = "Duplicate center facelet color";
		else if ((c[0] ^ c[1]) != 1 || (c[2] ^ c[3]) != 1 ||
			(c[4] ^ c[5]) != 1 || ((c[0] ^ c[2] ^ c[4] ^
			(c[0] < c[2]) ^ (c[2] < c[4]) ^ (c[4] < c[0])) & 1))
			s = "Non-standard center facelet color pattern.\n"
		"Use opposite faces: yellow/white, red/orange blue/green.\n"
		"And yellow-red-blue clockwise around some corner";
		if (s || (s = ftoc(NULL, gr.gr_fe)))
			sprintf(buf, "Abandoning edited Cube.\n\n"
				"The edited Cube contains errors:\n\n%s.", s),
			i = Mess(gr.gr_w, buf, "Editing Error", 2);
		else i = Mess(gr.gr_w,
			"Update main Cube with edited Cube and regenerate?",
			"Done Editing", 3);
	}
	if (i == IDCANCEL) return;
	if (i != IDYES) facewrite(gr.gr_fm);
	gr.gr_w = gr.gr_wm;
	p = gr.gr_wm;
	do EnableWindow(p, 1);
	while ((p = WX(p)->wx_next) != gr.gr_wm);
	(void) SetFocus(gr.gr_wg);
	(void) SelectObject(gr.gr_de, gr.gr_ofe);
	(void) DestroyWindow(gr.gr_we);
	(void) InvalidateRect(gr.gr_w, 0, 0);
	editmode = 0;
	if (i == IDYES) facetoeq(-1, timer.t_limits[4]);
}

/* if c == -1, draw color menu at (x,y), otherwise set face to color c */
/* if c == -2, next color (in menu order, but no menu) */
static void cbcolor(c, x, y)
int c, x, y;
{
	POINT p;
	long rx, ry;
	int l, ix, iy;
	static int f = { 0 };
	static int nextcolor[] = { 5, 3, 0, 2, 1, 4 };

	if (c < 0) {
		p.x = x, p.y = y/*, (void) DPtoLP(gr.gr_de, &p, 1)*/;
		if ((l = winlayer(p.x, p.y)) == -1) return;
		winface(&rx, &ry, p.x, p.y, l);
		ix = (rx + 0x30000L) * 10L / 0x20000L;
		iy = (ry + 0x30000L) * 10L / 0x20000L;
		if (ix % 10 < 1 || ix % 10 > 8 || iy % 10 < 1 || iy % 10 > 8)
			return;
		f = ix / 10 + iy / 10 * 3 + l * 9;
	}
	if (c == -2) c = nextcolor[gr.gr_fe[f]];
	if (c != -1) {
		gr.gr_fe[f] = c, facewrite(gr.gr_fe);
		InvalidateRect(gr.gr_w, 0, 0);
		return;
	}
	p.x = x, p.y = y, ClientToScreen(gr.gr_w, &p);
	TrackPopupMenu(gr.gr_mc, TPM_RIGHTALIGN | TPM_LEFTBUTTON,
			p.x + 10, p.y, 0, gr.gr_w, NULL);
}

/* draw color item in floating pop-up menu with info p */
void drawcolor(p)
LPDRAWITEMSTRUCT p;
{
	HDC hD;
	HBITMAP hB;
	RECT r;

	hD = CreateCompatibleDC(p->hDC);
	hB = SelectObject(hD, gr.gr_bc);
	r = p->rcItem;
	BitBlt(p->hDC, r.left, r.top, r.right - r.left, r.bottom - r.top, hD,
		p->itemState & ODS_SELECTED? gr.gr_bcx: 0,
		p->itemData * gr.gr_bcy, SRCCOPY);
	SelectObject(hD, hB);
	DeleteDC(hD);
}

/* do graphics drag/drop on cube.  lParam is packed mouse coordinates.
	b is button state: -1 no button change, 0 buttons up, 1 left down,
	2 left double click down, 3 right down, 4 right double click down */
/* new: keys (from wParam)	bits	docube() types
	left			1	1		near layer
	right			2       6		far layer
	left&right		3	2		cube
	middle			4	4		middle layer
	middle&left		5	3		slice
	middle&right		6	5		anti-slice
	all			7	2		cube
	(middle button on mouse == CTRL key) */
/* return: got middle key? */
static int cbmainmouse(wParam, lParam)
WPARAM wParam;
LPARAM lParam;
{
	static int bb = { 0 }, ll = { -1 }, xx = { 0 }, yy = { 0 };
	static long nx = { 0x10000L }, ny = { 0L };
	static long rx = { 0x10000L }, ry = { 0L };
	long ox, oy;
	int l, x, y, q, b;
	POINT p;
	HCURSOR hC;
	static int remap[] = { 0, 1, 6, 2, 4, 3, 5, 2 };

	b = remap[(wParam & MK_LBUTTON? 1: 0) | (wParam & MK_RBUTTON? 2: 0) |
		(wParam & (MK_MBUTTON | MK_CONTROL)? 4: 0)];
	b = b == bb? -1: bb? 0: b;
	if (b == -1) b = bb;
	if (b && genmode) { (void) upcube(2, 1); return (bb >= 4); }
/*
	if (b && bb && ((b ^ bb) & 1 || bb == 5)) b = 5;
*/
	x = (short) LOWORD(lParam), y = (short) HIWORD(lParam);
	if (b == bb && x == xx && y == yy) return (bb >= 4);
	if (x < 0 || x >= gr.gr_wx || y < 0 || y >= gr.gr_wy) b = x = y = 0;
	else {
		p.x = x, p.y = y, ClientToScreen(gr.gr_wm, &p);
/*
		if (WindowFromPoint(p) == gr.gr_wm)
			p.x = x, p.y = y, (void) DPtoLP(gr.gr_dm, &p, 1),
			x = p.x, y = p.y;
		else b = x = y = 0;
*/
		if (WindowFromPoint(p) != gr.gr_wm) b = x = y = 0;
	}
	if (b != bb) {
		if (bb) (void) docube(bb - 1, ll, 0x10000L, 0L, 0x10000L, 0L);
		if ((l = winlayer(x, y)) == -1) b = 0;
		else winface(&rx, &ry, x, y, l),
			getrot(&rx, &ry, nx = 0x10000L, ny = 0L);
	}
	else if (b) {
		ox = nx, oy = ny, winface(&nx, &ny, x, y, l = ll);
		getrot(&nx, &ny, rx, ry);
		if (q = docube(b - 1, l, ox, oy, nx, ny))
			rparen(), catmove(b == 6? (l^1) * 4 + q:
					(b - 1) * 24 + l * 4 + q);
	}
	else l = winlayer(x, y);
	if (!b && bb) ReleaseCapture(), (void) SetFocus(GetActiveWindow());
	if (ll != l || b != bb)
		hC = l != -1? gr.gr_cs[l + 6*!!b]: gr.gr_ca,
		SetClassLong(gr.gr_w, GCL_HCURSOR, hC), (void) SetCursor(hC);
	if (b && !bb)
		(void) SetFocus(gr.gr_w), (void) SetCapture(gr.gr_w);
	bb = b, ll = l, xx = x, yy = y;
	return (bb >= 4);
}

/* return layer of screen coordinates (x,y); or -1 */
static int winlayer(x, y)
int x, y;
{
	int f1, f2, f3;
	int sx, sy;
	RECT r;

	GetClientRect(gr.gr_w, &r);
	sx = r.right, sy = r.bottom - GENHEIGHT;
	if (sx <= sy + sy) y -= (sy - (sx >> 1)) >> 1, sy = sx >> 1;
	else x -= (sx - (sy << 1)) >> 1, sx = sy << 1;
	if (f1 = x > (sx >> 1)) x = sx - x, y = sy - y;
	x -= sx >> 2, y -= sy >> 1;
	if (f2 = x < 0) x = -x;
	if (f3 = y < 0) y = -y;
	sy /= 13;
	if (11 * x > 57 * sy || y > 6 * sy || 19 * y + 11 * x > 114 * sy)
		return (-1);
/*
	if (x > 374 || y >= 432 || 19 * y + 11 * x > 8218) return (-1);
*/
	return ((f3 && 11 * x <= 19 * y? 4: f2? 2: 0) | f1);
}

/* return coordinates (*px,*py) on layer l with screen coordinates (x,y) */
static void winface(px, py, x, y, l)
long *px, *py;
int x, y, l;
{
	long a, b;
	int sx, sy;
	RECT r;

	GetClientRect(gr.gr_w, &r);
	sx = r.right, sy = r.bottom - GENHEIGHT;
	if (sx <= sy + sy) y -= (sy - (sx >> 1)) >> 1, sy = sx >> 1;
	else x -= (sx - (sy << 1)) >> 1, sx = sy << 1;
	if (l & 1) x = sx - x, y = sy - y;
	x -= sx >> 2, y -= sy >> 1;
	sy /= 13;
	if (!(l & 6)) x = -x;
	a = ((long) x << 16) * 22L / 19L / sy;
	b = ((long) y << 16) / sy - a / 2L;
	*px = (l & 4? b + a: l & 2? -b:  a) + 0x30000L;
	*py = (l & 4? b    : l & 2?  a: -b) + 0x30000L;
	if (l & 1) *px = -*px, *py = -*py;
}

/* concatenate move m to generator */
void catmove(m)
int m;
{
	char *s, *t, c, d;
	int k, l, q, i;

	k = m / 24, l = m/4%6, q = m%4;
	s = linetab + strlen(linetab);
	if (s > linetab && (s[-1] == '-' || s[-1] == '\'')) i = 3, s--;
	else if (s > linetab && (s[-1] == '2')) i = 2, s--;
	else i = 1;
	c = "RLFBUD"[l], d = " csma"[k];
	if (k)
		if (s > linetab && tolower(s[-1]) == d) d = *--s;
		else i = 0;
	if (s > linetab && (toupper(s[-1]) == c ||
			l == 4 && toupper(s[-1]) == 'T'))
		c = *--s;
	else i = 0;
	for (t = movetab; *t; t++);
	if (!i) *t++ = m, *t = '\0';
	else t[-1] = i + q & 3? m & ~3 | i + q & 3: 0;
	if (!i) s = linetab + strlen(linetab);
	t = s;
	GSELECT(s - linetab, strlen(linetab));
	if (i + q & 3) {
		*t++ = c;
		if (k) *t++ = d;
		if (i+q & 2) *t++ = "2'"[i + q & 1];
	}
	*t = '\0';
	GREPLACE(s);
	WX(gr.gr_wm)->wx_save = 1;
}

/* append rparen's to terminate any + or * operators */
void rparen(void)
{
	char *s, *t;
	int i;

	for (s = linetab; *s && *s != ':'; s++);
	if (*s) s++; else s = linetab;
	for (i = 0; *s; s++)
		if (i? *s == '(' || *s == ')': *s == '+' || *s == '*')
			if (*s == ')') i--; else i++;
	GSELECT(s - linetab, s - linetab);
	if (!i) return;
	for (s = linetab; i--; *s++ = ')');
	*s = '\0';
	GREPLACE(linetab);
}

static void scalewin(wx, wy)
int wx, wy;
{
	int x, y;

	y = wx <= wy + wy? wx >> 1: wy;
	x = y << 1;
	gr.gr_bx0 = wx - x >> 1, gr.gr_by0 = wy - y >> 1;
	gr.gr_byc0 = y >> 1;
	gr.gr_bxc0r = (x >> 1) + (gr.gr_bxc0l = x >> 2);
	if (x != gr.gr_bx || y != gr.gr_by) {
		DeleteObject(SelectObject(gr.gr_dd, gr.gr_obm));
		gr.gr_bx = x, gr.gr_by = y;
		gr.gr_bm = CreateCompatibleBitmap(gr.gr_dm, x, y);
		gr.gr_obm = SelectObject(gr.gr_dd, gr.gr_bm);
		gr.gr_bx1 = ((long) x << 3) * 19 / 22 / 13;
		gr.gr_by1 = ((long) y << 4) / 13;
		initrot();
	}
/*
	long x, y, sx, sy;
	HDC hD;

	sx = (long) gr.gr_ax * wx;
	sy = (long) gr.gr_ay * wy;
	if (sx <= sy + sy)
		sy = sy * 468L / sx, sx = 640L,
		y = 175L - sy / 2L, x = 0L;
	else	sx = sx * 320L / sy, sy = 700 / 3L,
		x = 320L - sx / 2L, y = 175L / 3L;
	hD = GetDC(gr.gr_w);
	(void) SetWindowOrgEx(hD, (int)  x, (int)  y, NULL);
	(void) SetWindowExtEx(hD, (int) sx, (int) sy, NULL);
	(void) SetViewportOrgEx(hD, 0, 0, NULL);
	(void) SetViewportExtEx(hD, wx, wy, NULL);
*/
}

/* initiate parallelogram graphics */
void initgraph()
{
	RECT r;

	(void) GetClientRect(gr.gr_w, &r);
	r.bottom -= GENHEIGHT;
	scalewin(r.right, r.bottom);
	InvalidateRect(gr.gr_w, &r, 0);
/*
	r.left = 0, r.top = 58, r.bottom = 292, r.right = 640;
*/
	r.left = 0, r.right = gr.gr_bx;
	r.top = 0, r.bottom = gr.gr_by;
	(void) FillRect(gr.gr_dd, &r, gr.gr_br[8]);
}

/* draw parallelogram with vertices xy[4][2], in EGA color (page obsolete) */
void dopgram(xy, color, page)
int *xy, color, page;
{
	POINT p[4];

	(void) page;
	p[0].x = xy[0], p[0].y = xy[1];
	p[1].x = xy[2], p[1].y = xy[3];
	p[2].x = xy[4], p[2].y = xy[5];
	p[3].x = xy[6], p[3].y = xy[7];
	if (gr.gr_cl != color)
		(void) SelectObject(gr.gr_dd, gr.gr_br[gr.gr_cl = color]);
	(void) Polygon(gr.gr_dd, p, 4);
}

/* terminate parallelogram graphics */
void termgraph()
{
	PAINTSTRUCT p;
	RECT r;
	HDC hD;

	hD = GetDC(gr.gr_w);
	(void) BeginPaint(gr.gr_w, &p);
/*
	(void) BitBlt(hD, 0, 58, 640, 234, gr.gr_dd, 0, 58, SRCCOPY);
*/
	(void) BitBlt(hD, gr.gr_bx0, gr.gr_by0, gr.gr_bx, gr.gr_by,
		gr.gr_dd, 0, 0, SRCCOPY);
	(void) GetClientRect(gr.gr_w, &r);
	r.top = r.bottom - GENHEIGHT;
/*
	(void) DPtoLP(hD, &r, 2);
*/
	(void) FillRect(hD, &r, gr.gr_brm);
	(void) DrawText(hD, gr.gr_w == gr.gr_wm? "  Generator": "  Edit Cube",
		-1, &r, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	EndPaint(gr.gr_w, &p);
}

/* perform rotation by unit vector (rx, ry) (signed 32 bit w/16 bit fraction)
	about axis X if r=0, Y if r=1, Z if r=2, of four triples in xyz
	(signed 32 bit w/16 bit fraction range +- 3) onto four screen
	pairs xy, (640, 350) screen w/aspect ratio (4,3).  if side center
	at (480,175) otherwise (160,175). */
void rot(xy, xyz, side, r, rx, ry)
int *xy, side, r;
long *xyz, rx, ry;
{
	long *p, *q, t, tab[12];

	for (p = tab; p < tab + 12; *p++ = *xyz++ >> 4);
	rx >>= 2, ry >>= 2;
	p = tab + (r + 1 < 3? r + 1: r - 2);
	q = tab + (r + 2 < 3? r + 2: r - 1);
	for (; p < tab + 12; p += 3, q += 3)
		t  = (*p * rx - *q * ry) >> 14,
		*q = (*p * ry + *q * rx) >> 14, *p = t;
	side = side? gr.gr_bxc0r: gr.gr_bxc0l;
	for (p = tab; p < tab + 12; p += 3)
		*xy++ = (int) ((p[0] - p[1]) * gr.gr_bx1 >> 16) + side,
		*xy++ = (int) (((p[1] + p[0] >> 1) - p[2]) * gr.gr_by1 >> 16)
				+ gr.gr_byc0;
/*
	long *p, *q, t, tab[12];

	for (p = tab; p < tab + 12; *p++ = *xyz++ >> 4);
	rx >>= 2, ry >>= 2;
	p = tab + (r + 1 < 3? r + 1: r - 2);
	q = tab + (r + 2 < 3? r + 2: r - 1);
	for (; p < tab + 12; p += 3, q += 3)
		t  = (*p * rx - *q * ry) >> 14,
		*q = (*p * ry + *q * rx) >> 14, *p = t;
	side = side? 480: 160;
	for (p = tab; p < tab + 12; p += 3)
		*xy++ = (int) ((p[0] - p[1]) * 342L >> 16) + side,
		*xy++ = (int) (((p[1] + p[0] >> 1) - p[2]) * 288L >> 16) + 175;
*/
}

/* parse equation with label s into move list out, buffer length len */
int eqtomove(out, s, len)
char *out, *s;
int len;
{
	struct {
		char *t_paren, *t_plus, *t_star, *t_seq;
	} stack[100], *sp = { stack };
	char *t, *u, *v, c;
	int i, j, k;

	for (t = s; *t && *t != ':'; t++);
	if (*t) s = t + 1;
	sp->t_paren = t = out, sp->t_plus = sp->t_star = sp->t_seq = 0;
	while (1) {
		while (*s == ' ') s++;
		for (u = "RLFBUDT", i = 0; u[i] && toupper(*s) != u[i]; i++);
		if (!*s) k = 0;
		else if (*s == ')') k = 1, s++;
		else if (*s == '+') k = 2, s++;
		else if (*s == '*') k = 3, s++;
		else if (u[i]) {
			for (u="CcSsMmAa", j = 0, s++; u[j] && *s != u[j]; j++);
			if (u[j]) s++, j = j / 2 + 1;
			i = (i == 6? 4: i) * 4 + (u[j]? j * 24: 0) + 1;
			while (*s == ' ') s++;
			while (*s >= '0' && *s <= '9' || *s== '\'' || *s== '-'){
				if (*s == '\'' || *s == '-') j = -1, s++;
				else for (j = 0; *s >= '0' && *s <= '9';)
					j *= 10, j += *s++ - '0';
				i = i & ~3 | i*j & 3;
				while (*s == ' ') s++;
			}
			k = 4;
		}
		else if (*s == '(') k = 5, s++;
		else if (*s >= '0' && *s <= '9')
			for (k = 6, i = 0; *s >= '0' && *s <= '9';
				i *= 10, i += *s++ -'0');
		else if (*s == '\'' || *s == '-') k = 7, s++;
		else { s++; continue; }
		if (k == 7 && sp->t_seq) {
			for (u = sp->t_seq, v = t; u < --v; u++)
				c = *u, *u = *v, *v = c;
			for (u = sp->t_seq; u < t; u++) *u = *u ^ *u << 1 & 2;
		}
		if (k == 7) continue;
		if (k == 6 && sp->t_seq) for (u = t, t = sp->t_seq; i--;)
			for (v = sp->t_seq; v < u; *t++ = *v++)
				if (t == out + len) { *out = 0; return (1); }
		if (k == 6) continue;
		sp->t_seq = t;
		if (k == 5) {
			if (++sp == stack + 100) { *out = 0; return (1); }
			sp->t_paren = t, sp->t_plus = sp->t_star = sp->t_seq =0;
			continue;
		}
		if (k == 4)
			if (!(i&3)) continue;
			else if (t == out + len) { *out = 0; return (1); }
			else { *t++ = i; continue; }
		if (u = sp->t_star)
			for (v = sp->t_plus? sp->t_plus: sp->t_paren;
					u > v; --u, *t++ = *u ^ *u<<1 & 2)
				if (t == out + len) { *out = 0; return (1); }
		sp->t_star = t, sp->t_seq = 0;
		if (k == 3) continue;
		if (u = sp->t_plus) {
			for (v = t; u > sp->t_paren; --u,*t++ = *u ^ *u<<1 & 2)
				if (t == out + len) { *out = 0; return (1); }
			for (; v > sp->t_plus; --v, *t++ = *v ^ *v << 1 & 2)
				if (t == out + len) { *out = 0; return (1); }
		}
		sp->t_plus = t, sp->t_star = 0;
		if (k == 2) continue;
		sp->t_plus = 0;
		if (sp > stack) sp--;
		if (k == 1) continue;
		if (sp == stack) break;
	}
	if (t == out + len) { *out = 0; return (1); }
	*t = '\0';
	return (0);
}

