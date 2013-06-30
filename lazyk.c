/*
 *  Lazy K interpreter
 *
 *  Copyright 2008 irori <irorin@gmail.com>
 *  This is free software. You may modify and/or distribute it under the
 *  terms of the GNU General Public License, version 2 or any later version.
 *  It comes with no warranty.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#define HEAP_SIZE 3800000
#define RDSTACK_SIZE	50000

extern void output_char(int);
extern void error(const char *);

/**********************************************************************
 *  Storage management
 **********************************************************************/

/* TAG STRUCTURE
 *
 *  -------- -------- -------- ------00   Pair
 *  -------- -------- -------- ------01   Int
 *  -------- -------- -------- ------10   Combinator
 *  -------- -------- -------- -----011   Character
 *  -------- -------- -------- -----111   Miscellaneous
 */

struct tagPair;
typedef struct tagPair *Cell;
#define CELL(x)	((Cell)(x))
#define TAG(c)	((int)(c) & 0x03)

/* pair */
typedef struct tagPair {
    Cell car;
    Cell cdr;
} Pair;
#define ispair(c)	(TAG(c) == 0)
#define car(c)		((c)->car)
#define cdr(c)		((c)->cdr)
#define SET(c,fst,snd)  ((c)->car = (fst), (c)->cdr = (snd))

/* integer */
#define isint(c)	(TAG(c) == 1)
#define mkint(n)	CELL(((n) << 2) + 1)
#define intof(c)	((signed int)(c) >> 2)

/* combinator */
#define iscomb(c)	(TAG(c) == 2)
#define mkcomb(n)	CELL(((n) << 2) + 2)
#define combof(c)	((int)(c) >> 2)
#define COMB_S		mkcomb(0)
#define COMB_K		mkcomb(1)
#define COMB_I		mkcomb(2)
#define COMB_IOTA	mkcomb(3)
#define COMB_KI		mkcomb(4)
#define COMB_READ	mkcomb(5)
#define COMB_WRITE	mkcomb(6)
#define COMB_INC	mkcomb(7)
#define COMB_CONS	mkcomb(8)

/* character */
#define ischar(c)	(((int)(c) & 0x07) == 0x03)
#define mkchar(n)	CELL(((n) << 3) + 0x03)
#define charof(c)	((int)(c) >> 3)

/* immediate objects */
#define isimm(c)	(((int)(c) & 0x07) == 0x07)
#define mkimm(n)	CELL(((n) << 3) + 0x07)
#define NIL		mkimm(0)
#define COPIED		mkimm(1)
#define UNUSED_MARKER	mkimm(2)

Pair *heap_area, *free_area, *free_ptr;

void gc_run(Cell *save1, Cell *save2);
void rs_copy(void);
Cell copy_cell(Cell c);

void errexit(char *fmt, ...)
{
    char buf[200];
    va_list arg;
    va_start(arg, fmt);
    vsprintf(buf, fmt, arg);
    va_end(arg);

    error(buf);

    exit(1);
}

void storage_init()
{
    heap_area = malloc(sizeof(Pair) * HEAP_SIZE * 2);
    if (heap_area == NULL)
	errexit("Cannot allocate heap storage (%d cells)\n", HEAP_SIZE);
    assert(((int)heap_area & 3) == 0 && (sizeof(Pair) & 3) == 0);
    
    free_ptr = heap_area;
    heap_area += HEAP_SIZE;
    free_area = heap_area;
}

Cell pair(Cell fst, Cell snd)
{
    Cell c;
    if (free_ptr >= heap_area)
	gc_run(&fst, &snd);

    assert(free_ptr < heap_area);
    c = free_ptr++;
    car(c) = fst;
    cdr(c) = snd;
    return c;
}

Cell alloc(int n)
{
    Cell p;
    if (free_ptr + n > heap_area)
	gc_run(NULL, NULL);

    assert(free_ptr + n <= heap_area);
    p = free_ptr;
    free_ptr += n;
    return p;
}


void gc_run(Cell *save1, Cell *save2)
{
    int num_alive;
    Pair *scan;

    free_ptr = scan = free_area;
    free_area = heap_area - HEAP_SIZE;
    heap_area = free_ptr + HEAP_SIZE;

    rs_copy();
    if (save1)
	*save1 = copy_cell(*save1);
    if (save2)
	*save2 = copy_cell(*save2);

    while (scan < free_ptr) {
	car(scan) = copy_cell(car(scan));
	cdr(scan) = copy_cell(cdr(scan));
	scan++;
    }

    num_alive = free_ptr - (heap_area - HEAP_SIZE);

    if (num_alive * 9 / 10 > HEAP_SIZE)
	errexit("out of memory");
}

Cell copy_cell(Cell c)
{
    Cell r;

    if (!ispair(c))
	return c;
    if (car(c) == COPIED)
	return cdr(c);

    r = free_ptr++;
    car(r) = car(c);
    if (car(c) == COMB_I) {
	Cell tmp = cdr(c);
	while (ispair(tmp) && car(tmp) == COMB_I)
	    tmp = cdr(tmp);
	cdr(r) = tmp;
    }
    else
	cdr(r) = cdr(c);
    car(c) = COPIED;
    cdr(c) = r;
    return r;
}

/**********************************************************************
 *  Reduction Machine
 **********************************************************************/

typedef struct {
    Cell *sp;
    Cell *stack;
} RdStack;

RdStack rd_stack;

void rs_init(void)
{
    int i;
    rd_stack.stack = (Cell *)malloc(sizeof(Cell) * RDSTACK_SIZE);
    rd_stack.sp = rd_stack.stack + RDSTACK_SIZE;

    for (i = 0; i < RDSTACK_SIZE; i++)
	rd_stack.stack[i] = UNUSED_MARKER;
}

void rs_copy(void)
{
    Cell *c;
    for (c = rd_stack.stack + RDSTACK_SIZE - 1; c >= rd_stack.sp; c--)
	*c = copy_cell(*c);
}

int rs_max_depth(void)
{
    int i;
    for (i = 0; i < RDSTACK_SIZE; i++) {
	if (rd_stack.stack[i] != UNUSED_MARKER)
	    break;
    }
    return RDSTACK_SIZE - i;
}

void rs_push(Cell c)
{
    if (rd_stack.sp <= rd_stack.stack)
	errexit("runtime error: stack overflow\n");
    *--rd_stack.sp = c;
}

#define TOP		(*rd_stack.sp)
#define POP		(*rd_stack.sp++)
#define PUSH(c)		rs_push(c)
#define PUSHED(n)	(*(rd_stack.sp+(n)))
#define DROP(n)		(rd_stack.sp += (n))
#define ARG(n)		cdr(PUSHED(n))
#define APPLICABLE(n)	(bottom - rd_stack.sp > (n))

/**********************************************************************
 *  Loader
 **********************************************************************/

const char *g_input;

Cell read_one(int i_is_iota);
Cell read_many(char closing_char);

Cell load_program(void)
{
    return read_many('\0');
}

int next_char(void)
{
    int c;
    do {
	c = *g_input++;
	if (c == '#') {
	    while ((c = *g_input++) && c != '\n')
		;
	}
    } while (isspace(c));
    return c;
}

Cell read_many(char closing_char)
{
    int c;
    Cell obj;

    c = next_char();
    if (c == closing_char)
	return COMB_I;
    g_input--;

    PUSH(read_one(0));
    while ((c = next_char()) != closing_char) {
	g_input--;
	obj = pair(TOP, read_one(0));
	TOP = obj;
    }
    return POP;
}

Cell read_one(int i_is_iota)
{
    int c;
    Cell obj;

    c = next_char();
    switch (c) {
    case '`': case '*':
	PUSH(read_one(c == '*'));
	obj = pair(TOP, read_one(c == '*'));
	POP;
	return obj;
    case '(':
	obj = read_many(')');
	return obj;
    case 's': case 'S': return COMB_S;
    case 'k': case 'K': return COMB_K;
    case 'i': return i_is_iota ? COMB_IOTA : COMB_I;
    case 'I': return COMB_I;
    case '0': case '1': {
	obj = COMB_I;
	do {
	    if (c == '0')
		obj = pair(pair(obj, COMB_S), COMB_K);
	    else
		obj = pair(COMB_S, pair(COMB_K, obj));
	    c = next_char();
	} while (c == '0' || c == '1');
	g_input--;
	return obj;
    }
    case '\0':
	errexit("parse error: unexpected EOF\n");
    default:
	errexit("parse error: %c\n", c);
    }
}

/**********************************************************************
 *  Reducer
 **********************************************************************/

int reductions;

void eval(Cell root)
{
    Cell *bottom = rd_stack.sp;
    PUSH(root);

    for (;;) {
	while (ispair(TOP))
	    PUSH(car(TOP));

	if (TOP == COMB_I && APPLICABLE(1))
	{ /* I x -> x */
	    POP;
	    TOP = cdr(TOP);
	}
	else if (TOP == COMB_S && APPLICABLE(3))
	{ /* S f g x -> f x (g x) */
	    Cell a = alloc(2);
	    SET(a+0, ARG(1), ARG(3));	/* f x */
	    SET(a+1, ARG(2), ARG(3));	/* g x */
	    DROP(3);
	    SET(TOP, a+0, a+1);	/* f x (g x) */
	}
	else if (TOP == COMB_K && APPLICABLE(2))
	{ /* K x y -> I x */
	    Cell x = ARG(1);
	    DROP(2);
	    SET(TOP, COMB_I, x);
	    TOP = cdr(TOP);	/* shortcut reduction of I */
	}
	else if (TOP == COMB_IOTA && APPLICABLE(1))
	{ /* IOTA x -> x S K */
	    Cell xs = pair(ARG(1), COMB_S);
	    POP;
	    SET(TOP, xs, COMB_K);
	}
	else if (TOP == COMB_KI && APPLICABLE(2))
	{ /* KI x y -> I y */
	    DROP(2);
	    car(TOP) = COMB_I;
	}
	else if (TOP == COMB_CONS && APPLICABLE(3))
	{ /* CONS x y f -> f x y */
	    Cell fx, y;
	    fx = pair(ARG(3), ARG(1));
	    y = ARG(2);
	    DROP(3);
	    SET(TOP, fx, y);
	}
	else if (TOP == COMB_READ && APPLICABLE(2))
	{ /* READ NIL f -> CONS CHAR(c) (READ NIL) f */
	    int c = *g_input++;
	    Cell a = alloc(2);
	    SET(a+0, COMB_CONS, mkchar(c == '\0' ? 256 : c));
	    SET(a+1, COMB_READ, NIL);
	    POP;
	    SET(TOP, a+0, a+1);
	}
	else if (TOP == COMB_WRITE && APPLICABLE(1))
	{ /* WRITE x -> putc(eval((car x) INC NUM(0))); WRITE (cdr x) */
	    Cell a = alloc(3);
	    SET(a+0, ARG(1), COMB_K);	/* (car x) */
	    SET(a+1, a+0, COMB_INC);	/* (car x) INC */
	    SET(a+2, a+1, mkint(0));	/* (car x) INC NUM(0) */
	    POP;
	    eval(a+2);

	    if (!isint(TOP))
		errexit("invalid output format (result was not a number)\n");
	    if (intof(TOP) >= 256)
		return;

	    output_char(intof(TOP));
	    POP;
	    a = pair(cdr(TOP), COMB_KI);
	    cdr(TOP) = a;
	}
	else if (TOP == COMB_INC && APPLICABLE(1))
	{ /* INC x -> eval(x)+1 */
	    Cell c = ARG(1);
	    POP;
	    eval(c);

	    c = POP;
	    if (!isint(c))
		errexit("invalid output format (attempted to apply inc to a non-number)\n");
	    SET(TOP, COMB_I, mkint(intof(c) + 1));
	}
	else if (ischar(TOP) && APPLICABLE(2)) {
	    int c = charof(TOP);
	    if (c <= 0) {  /* CHAR(0) f z -> z */
		Cell z = ARG(2);
		DROP(2);
		SET(TOP, COMB_I, z);
	    }
	    else {       /* CHAR(n+1) f z -> f (CHAR(n) f z) */
		Cell a = alloc(2);
		Cell f = ARG(1);
		SET(a+0, mkchar(c-1), f);	/* CHAR(n) f */
		SET(a+1, a+0, ARG(2));		/* CHAR(n) f z */
		DROP(2);
		SET(TOP, f, a+1);		/* f (CHAR(n) f z) */
	    }
	}
	else if (isint(TOP) && APPLICABLE(1))
	    errexit("invalid output format (attempted to apply a number)\n");
	else
	    return;
	reductions++;
    }
}

void eval_print(Cell root)
{
    eval(pair(COMB_WRITE,
	      pair(root,
		   pair(COMB_READ, NIL))));
}

/**********************************************************************
 *  Main
 **********************************************************************/

void eval_program(const char *program, const char *input)
{
    Cell root;
    char *prog_file = NULL;
    
    storage_init();
    rs_init();

    g_input = program;
    root = load_program();

    g_input = input;
    eval_print(root);

    // free
}
