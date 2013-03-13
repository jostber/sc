// SC is free software distributed under the MIT license

#include "sc.h"

struct colorpair* cpairs[8];
static struct crange* color_base;

void initcolor (int colornum)
{
    if (!colornum) {
	int i;
	for (i = 0; i < 8; i++)
	    cpairs[i] = (struct colorpair*) scxmalloc (sizeof(struct colorpair));
    }

    // default colors
    if (!colornum || colornum == 1) {
	cpairs[0]->fg = COLOR_WHITE;
	cpairs[0]->bg = COLOR_BLUE;
	cpairs[0]->expr = NULL;
	init_pair(1, cpairs[0]->fg, cpairs[0]->bg);
    }

    // default for negative numbers
    if (!colornum || colornum == 2) {
	cpairs[1]->fg = COLOR_RED;
	cpairs[1]->bg = COLOR_WHITE;
	cpairs[1]->expr = NULL;
	init_pair(2, cpairs[1]->fg, cpairs[1]->bg);
    }

    // default for cells with errors
    if (!colornum || colornum == 3) {
	cpairs[2]->fg = COLOR_WHITE;
	cpairs[2]->bg = COLOR_RED;
	cpairs[2]->expr = NULL;
	init_pair(3, cpairs[2]->fg, cpairs[2]->bg);
    }

    // default for '*' marking cells with attached notes
    if (!colornum || colornum == 4) {
	cpairs[3]->fg = COLOR_BLACK;
	cpairs[3]->bg = COLOR_YELLOW;
	cpairs[3]->expr = NULL;
	init_pair(4, cpairs[3]->fg, cpairs[3]->bg);
    }

    if (!colornum || colornum == 5) {
	cpairs[4]->fg = COLOR_BLACK;
	cpairs[4]->bg = COLOR_CYAN;
	cpairs[4]->expr = NULL;
	init_pair(5, cpairs[4]->fg, cpairs[4]->bg);
    }

    if (!colornum || colornum == 6) {
	cpairs[5]->fg = COLOR_RED;
	cpairs[5]->bg = COLOR_CYAN;
	cpairs[5]->expr = NULL;
	init_pair(6, cpairs[5]->fg, cpairs[5]->bg);
    }

    if (!colornum || colornum == 7) {
	cpairs[6]->fg = COLOR_DEFAULT;
	cpairs[6]->bg = COLOR_DEFAULT;
	cpairs[6]->expr = NULL;
	init_pair(7, cpairs[6]->fg, cpairs[6]->bg);
    }

    if (!colornum || colornum == 8) {
	cpairs[7]->fg = COLOR_RED;
	cpairs[7]->bg = COLOR_DEFAULT;
	cpairs[7]->expr = NULL;
	init_pair(8, cpairs[7]->fg, cpairs[7]->bg);
    }

    if (color && has_colors())
	attron(COLOR_PAIR(1));
}

void change_color (int pair, struct enode *e)
{
    if ((--pair) < 0 || pair > 7) {
	error("Invalid color number");
	return;
    }

    int v = (int) eval(e);

    if (!cpairs[pair])
	cpairs[pair] =
	    (struct colorpair *)scxmalloc((unsigned)sizeof(struct colorpair));
    cpairs[pair]->fg = v & 7;
    cpairs[pair]->bg = (v >> 3) & 7;
    cpairs[pair]->expr = e;
    if (color && has_colors())
	init_pair(pair + 1, cpairs[pair]->fg, cpairs[pair]->bg);

    modflg++;
    FullUpdate++;
}

void add_crange (struct ent *r_left, struct ent *r_right, int pair)
{
    struct crange *r;
    int minr, minc, maxr, maxc;

    minr = r_left->row < r_right->row ? r_left->row : r_right->row;
    minc = r_left->col < r_right->col ? r_left->col : r_right->col;
    maxr = r_left->row > r_right->row ? r_left->row : r_right->row;
    maxc = r_left->col > r_right->col ? r_left->col : r_right->col;

    if (!pair) {
	if (color_base) {
	    for (r = color_base; r; r = r->r_next) {
		if (    (r->r_left->row == r_left->row) &&
			(r->r_left->col == r_left->col) &&
			(r->r_right->row == r_right->row) &&
			(r->r_right->col == r_right->col)) {
		    if (r->r_next)
			r->r_next->r_prev = r->r_prev;
		    if (r->r_prev)
			r->r_prev->r_next = r->r_next;
		    else
			color_base = r->r_next;
		    scxfree((char *)r);
		    modflg++;
		    FullUpdate++;
		    return;
		}
	    }
	}
	error("Color range not defined");
	return;
    }

    r = (struct crange*) scxmalloc (sizeof(struct crange));
    r->r_left = lookat(minr, minc);
    r->r_right = lookat(maxr, maxc);
    r->r_color = pair;

    r->r_next = color_base;
    r->r_prev = (struct crange *)0;
    if (color_base)
	color_base->r_prev = r;
    color_base = r;

    modflg++;
    FullUpdate++;
}

void clean_crange (void)
{
    struct crange* cr = color_base;
    color_base = NULL;

    while (cr) {
	struct crange* nextcr = cr->r_next;
	scxfree((char *)cr);
	cr = nextcr;
    }
}

struct crange* find_crange (int row, int col)
{
    struct crange *r;
    if (color_base)
	for (r = color_base; r; r = r->r_next)
	    if (r->r_left->row <= row && r->r_left->col <= col && r->r_right->row >= row && r->r_right->col >= col)
		return r;
    return 0;
}

void sync_cranges (void)
{
    for (struct crange* cr = color_base; cr; cr = cr->r_next) {
	cr->r_left = lookat(cr->r_left->row, cr->r_left->col);
	cr->r_right = lookat(cr->r_right->row, cr->r_right->col);
    }
}

void write_cranges (FILE *f)
{
    struct crange* r = color_base;
    for (struct crange* nextr = r; nextr; r = nextr, nextr = r->r_next) {}
    for (; r; r = r->r_prev) {
	fprintf(f, "color %s", v_name(r->r_left->row, r->r_left->col));
	fprintf(f, ":%s", v_name(r->r_right->row, r->r_right->col));
	fprintf(f, " %d\n", r->r_color);
    }
}

void write_colors (FILE *f, int indent)
{
    int i, c = 0;
    for (i = 0; i < 8; i++) {
	if (cpairs[i] && cpairs[i]->expr) {
	    sprintf(line, "color %d = ", i + 1);
	    linelim = strlen(line);
	    decompile(cpairs[i]->expr, 0);
	    line[linelim] = '\0';
	    fprintf(f, "%*s%s\n", indent, "", line);
	    if (brokenpipe) return;
	    ++c;
	}
    }
    if (indent && c)
	fprintf(f, "\n");
}

void list_colors (FILE *f)
{
    struct crange *r;
    struct crange *nextr;

    write_colors(f, 2);
    linelim = -1;
    if (brokenpipe) return;

    if (!are_colors()) {
	fprintf(f, "  No color ranges");
	return;
    }

    fprintf(f, "  %-30s %s\n","Range", "Color");
    if (!brokenpipe) fprintf(f, "  %-30s %s\n","-----", "-----");

    for (r = nextr = color_base; nextr; r = nextr, nextr = r->r_next) // */ ;
    while (r) {
	fprintf(f, "  %-32s %d\n", r_name(r->r_left->row, r->r_left->col,
		r->r_right->row, r->r_right->col), r->r_color);
	if (brokenpipe) return;
	r = r->r_prev;
    }
}

int are_colors (void)
{
    return (color_base != 0);
}

void fix_colors (int row1, int col1, int row2, int col2, int delta1, int delta2)
{
    if (!color_base)
	return;
    struct frange* fr = find_frange (currow, curcol);
    for (struct crange* cr = color_base; cr; cr = cr->r_next) {
	int r1 = cr->r_left->row, c1 = cr->r_left->col;
	int r2 = cr->r_right->row, c2 = cr->r_right->col;
	if (!(fr && (c1 < fr->or_left->col || c1 > fr->or_right->col))) {
	    if (r1 != r2 && r1 >= row1 && r1 <= row2) r1 = row2 - delta1;
	    if (c1 != c2 && c1 >= col1 && c1 <= col2) c1 = col2 - delta1;
	}
	if (!(fr && (c2 < fr->or_left->col || c2 > fr->or_right->col))) {
	    if (r1 != r2 && r2 >= row1 && r2 <= row2) r2 = row1 + delta2;
	    if (c1 != c2 && c2 >= col1 && c2 <= col2) c2 = col1 + delta2;
	}
	if (r1 > r2 || c1 > c2)	// the 0 means delete color range
	    add_crange(cr->r_left, cr->r_right, 0);
	else {
	    cr->r_left = lookat(r1, c1);
	    cr->r_right = lookat(r2, c2);
	}
    }
}
