
# line 19 "parser.y"
#ifndef lint
#define lint
#endif
#define defTycon(n,l,lhs,rhs,w)	 tyconDefn(intOf(l),lhs,rhs,w); sp-=n
#define sigdecl(l,vs,t)		 ap(SIGDECL,triple(l,vs,t))
#define grded(gs)		 ap(GUARDED,gs)
#define letrec(bs,e)		 (nonNull(bs) ? ap(LETREC,pair(bs,e)) : e)
#define yyerror(s)		 /* errors handled elsewhere */
#define YYSTYPE			 Cell

static Cell   local gcShadow     Args((Int,Cell));
static Void   local syntaxError  Args((String));
static String local unexpected   Args((Void));
static Cell   local checkPrec    Args((Cell));
static Void   local fixDefn      Args((Syntax,Cell,Cell,List));
static Void   local setSyntax    Args((Int,Syntax,Cell));
static Cell   local buildTuple   Args((List));
static Cell   local checkClass   Args((Cell));
static List   local checkContext Args((List));
static Pair   local checkDo	 Args((List));
static Cell   local checkTyLhs	 Args((Cell));
static Cell   local tidyInfix    Args((Cell));

/* For the purposes of reasonably portable garbage collection, it is
 * necessary to simulate the YACC stack on the Gofer stack to keep
 * track of all intermediate constructs.  The lexical analyser
 * pushes a token onto the stack for each token that is found, with
 * these elements being removed as reduce actions are performed,
 * taking account of look-ahead tokens as described by gcShadow()
 * below.
 *
 * Of the non-terminals used below, only start, topDecl & begin do not leave
 * any values on the Gofer stack.  The same is true for the terminals
 * EVALEX and SCRIPT.  At the end of a successful parse, there should only
 * be one element left on the stack, containing the result of the parse.
 */

#define gc0(e)			 gcShadow(0,e)
#define gc1(e)			 gcShadow(1,e)
#define gc2(e)			 gcShadow(2,e)
#define gc3(e)			 gcShadow(3,e)
#define gc4(e)			 gcShadow(4,e)
#define gc5(e)			 gcShadow(5,e)
#define gc6(e)			 gcShadow(6,e)
#define gc7(e)			 gcShadow(7,e)

# define EVALEX 257
# define SCRIPT 258
# define COCO 259
# define INFIXL 260
# define INFIXR 261
# define INFIX 262
# define FUNARROW 263
# define UPTO 264
# define CASEXP 265
# define OF 266
# define IF 267
# define THEN 268
# define ELSE 269
# define WHERE 270
# define TYPE 271
# define DATA 272
# define FROM 273
# define LET 274
# define IN 275
# define VAROP 276
# define VARID 277
# define NUMLIT 278
# define CHARLIT 279
# define STRINGLIT 280
# define REPEAT 281
# define CONOP 282
# define CONID 283
# define TCLASS 284
# define IMPLIES 285
# define TINSTANCE 286
# define DO 287
# define TRUNST 288
# define PRIMITIVE 289
# define DEFAULT 290
# define DERIVING 291
# define HIDING 292
# define IMPORT 293
# define INTERFACE 294
# define MODULE 295
# define RENAMING 296
# define TO 297
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 488 "parser.y"


static Cell local gcShadow(n,e)		/* keep parsed fragments on stack  */
Int  n;
Cell e; {
    /* If a look ahead token is held then the required stack transformation
     * is:
     *   pushed: n               1     0          1     0
     *           x1  |  ...  |  xn  |  la   ===>  e  |  la
     *                                top()            top()
     *
     * Othwerwise, the transformation is:
     *   pushed: n-1             0        0
     *           x1  |  ...  |  xn  ===>  e
     *                         top()     top()
     */
    if (yychar>=0) {
	pushed(n-1) = top();
        pushed(n)   = e;
    }
    else
	pushed(n-1) = e;
    sp -= (n-1);
    return e;
}

static Void local syntaxError(s)       /* report on syntax error           */
String s; {
    ERROR(row) "Syntax error in %s (unexpected %s)", s, unexpected()
    EEND;
}

static String local unexpected() {	/* find name for unexpected token  */
    static char buffer[100];
    static char *fmt = "%s \"%s\"";
    static char *kwd = "keyword";
    static char *hkw = "(Haskell) keyword";

    switch (yychar) {
	case 0	       : return "end of input";

#define keyword(kw) sprintf(buffer,fmt,kwd,kw); return buffer;
	case INFIXL    : keyword("infixl");
	case INFIXR    : keyword("infixr");
	case INFIX     : keyword("infix");
	case TINSTANCE : keyword("instance");
	case TCLASS    : keyword("class");
	case PRIMITIVE : keyword("primitive");
	case CASEXP    : keyword("case");
	case OF        : keyword("of");
	case IF        : keyword("if");
	case DO	       : keyword("do");
	case TRUNST    : keyword("runST");
	case THEN      : keyword("then");
	case ELSE      : keyword("else");
	case WHERE     : keyword("where");
	case TYPE      : keyword("type");
	case DATA      : keyword("data");
	case LET       : keyword("let");
	case IN        : keyword("in");
#undef keyword

#define hasword(kw) sprintf(buffer,fmt,hkw,kw); return buffer;
	case DEFAULT   : hasword("default");
	case DERIVING  : hasword("deriving");
	case HIDING    : hasword("hiding");
	case IMPORT    : hasword("import");
	case INTERFACE : hasword("interface");
	case MODULE    : hasword("module");
	case RENAMING  : hasword("renaming");
	case TO	       : hasword("to");
#undef hasword

	case FUNARROW  : return "`->'";
	case '='       : return "`='";
	case COCO      : return "`::'";
	case '-'       : return "`-'";
	case ','       : return "comma";
	case '@'       : return "`@'";
	case '('       : return "`('";
	case ')'       : return "`)'";
	case '|'       : return "`|'";
	case ';'       : return "`;'";
	case UPTO      : return "`..'";
	case '['       : return "`['";
	case ']'       : return "`]'";
	case FROM      : return "`<-'";
	case '\\'      : return "backslash (lambda)";
	case '~'       : return "tilde";
	case '`'       : return "backquote";
	case VAROP     :
	case VARID     :
	case CONOP     :
	case CONID     : sprintf(buffer,"symbol \"%s\"",
				 textToStr(textOf(yylval)));
			 return buffer;
	case NUMLIT    : return "numeric literal";
	case CHARLIT   : return "character literal";
	case STRINGLIT : return "string literal";
	case IMPLIES   : return "`=>";
	default	       : return "token";
    }
}

static Cell local checkPrec(p)         /* Check for valid precedence value */
Cell p; {
    if (!isInt(p) || intOf(p)<MIN_PREC || intOf(p)>MAX_PREC) {
        ERROR(row) "Precedence value must be an integer in the range [%d..%d]",
                   MIN_PREC, MAX_PREC
        EEND;
    }
    return p;
}

static Void local fixDefn(a,line,p,ops)/* Declare syntax of operators      */
Syntax a;
Cell   line;
Cell   p;
List   ops; {
    Int l = intOf(line);
    a     = mkSyntax(a,intOf(p));
    map2Proc(setSyntax,l,a,ops);
}

static Void local setSyntax(line,sy,op)/* set syntax of individ. operator  */
Int    line;
Syntax sy;
Cell   op; {
    addSyntax(line,textOf(op),sy);
    opDefns = cons(op,opDefns);
}

static Cell local buildTuple(tup)      /* build tuple (x1,...,xn) from list*/
List tup; {                            /* [xn,...,x1]                      */
    Int  n = 0;
    Cell t = tup;
    Cell x;

    do {                               /*     .                    .       */
        x      = fst(t);               /*    / \                  / \      */
        fst(t) = snd(t);               /*   xn  .                .   xn    */
        snd(t) = x;                    /*        .    ===>      .          */
        x      = t;                    /*         .            .           */
        t      = fun(x);               /*          .          .            */
        n++;                           /*         / \        / \           */
    } while (nonNull(t));              /*        x1  NIL   (n)  x1         */
    fst(x) = mkTuple(n);
    return tup;
}

/* The yacc parser presented above is not sufficiently powerful to
 * determine whether a tuple at the front of a sigType is part of a
 * context:    e.g. (Eq a, Num a) => a -> a -> a
 * or a type:  e.g.  (Tree a, Tree a) -> Tree a
 *
 * Rather than complicate the grammar, both are parsed as tuples of types,
 * using the following checks afterwards to ensure that the correct syntax
 * is used in the case of a tupled context.
 */

static List local checkContext(con)	/* validate type class context	   */
Type con; {
    if (con==UNIT)			/* allows empty context ()	   */
	return NIL;
    else if (whatIs(getHead(con))==TUPLE) {
	List qs = NIL;

	while (isAp(con)) {		/* undo work of buildTuple  :-(    */
	    Cell temp = fun(con);
	    fun(con)  = arg(con);
	    arg(con)  = qs;
	    qs	      = con;
	    con       = temp;
	    checkClass(hd(qs));
	}
	return qs;
    }
    else				/* single context expression	   */
	return singleton(checkClass(con));
}

static Cell local checkClass(c)		/* check that type expr is a class */
Cell c; {				/* constrnt of the form C t1 .. tn */
    Cell cn = getHead(c);

    if (!isCon(cn))
	syntaxError("class expression");
    else if (argCount<1) {
	ERROR(row) "Class \"%s\" must have at least one argument",
		   textToStr(textOf(cn))
	EEND;
    }
    return c;
}

static Pair local checkDo(dqs)		/* convert reversed list of dquals */
List dqs; {				/* to a (expr,quals) pair	   */
#if DO_COMPS
    if (isNull(dqs) || whatIs(hd(dqs))!=DOQUAL) {
	ERROR(row) "Last generator in do {...} must be an expression"
	EEND;
    }
    fst(dqs) = snd(fst(dqs));		/* put expression in fst of pair   */
    snd(dqs) = rev(snd(dqs));		/* & reversed list of quals in snd */
#endif
    return dqs;
}

static Cell local checkTyLhs(c)		/* check that lhs is of the form   */
Cell c; {				/* T a1 ... a			   */
    Cell tlhs = c;
    while (isAp(tlhs) && whatIs(arg(tlhs))==VARIDCELL)
	tlhs = fun(tlhs);
    if (whatIs(tlhs)!=CONIDCELL) {
	ERROR(row) "Illegal left hand side in datatype definition"
	EEND;
    }
    return c;
}

/* expressions involving a sequence of two or more infix operator symbols
 * are parsed as elements of type:
 *    InfixExpr ::= [Expr]
 *		 |  ap(ap(Operator,InfixExpr),Expr)
 *
 * thus x0 +1 x1 ... +n xn is parsed as: +n (....(+1 [x0] x1)....) xn
 *
 * Once the expression has been completely parsed, this parsed form is
 * `tidied' according to the precedences and associativities declared for
 * each operator symbol.
 *
 * The tidy process uses a `stack' of type:
 *    TidyStack ::= ap(ap(Operator,TidyStack),Expr)
 *		 |  NIL
 * when the ith layer of an InfixExpr has been transferred to the stack, the
 * stack is of the form: +i (....(+n NIL xn)....) xi
 *
 * The tidy function is based on a simple shift-reduce parser:
 *
 *  tidy                :: InfixExpr -> TidyStack -> Expr
 *  tidy [m]   ss        = foldl (\x f-> f x) m ss
 *  tidy (m*n) []        = tidy m [(*n)]
 *  tidy (m*n) ((+o):ss)
 *	       | amb     = error "Ambiguous"
 *	       | shift   = tidy m ((*n):(+o):ss)
 *	       | reduce  = tidy (m*(n+o)) ss
 *			   where sye     = syntaxOf (*)
 *				 (ae,pe) = sye
 *				 sys     = syntaxOf (+)
 *				 (as,ps) = sys
 *				 amb     = pe==ps && (ae/=as || ae==NON_ASS)
 *				 shift   = pe>ps || (ps==pe && ae==LEFT_ASS)
 *				 reduce  = otherwise
 *
 * N.B. the conditions amb, shift, reduce are NOT mutually exclusive and
 * must be tested in that order.
 *
 * As a concession to efficiency, we lower the number of calls to syntaxOf
 * by keeping track of the values of sye, sys throughout the process.  The
 * value APPLIC is used to indicate that the syntax value is unknown.
 */

static Cell local tidyInfix(e)         /* convert InfixExpr to Expr        */
Cell e; {                              /* :: InfixExpr                     */
    Cell   s   = NIL;                  /* :: TidyStack                     */
    Syntax sye = APPLIC;               /* Syntax of op in e (init unknown) */
    Syntax sys = APPLIC;               /* Syntax of op in s (init unknown) */
    Cell   temp;

    while (nonNull(tl(e))) {
        if (isNull(s)) {
            s           = e;
            e           = arg(fun(s));
            arg(fun(s)) = NIL;
            sys         = sye;
            sye         = APPLIC;
        }
        else {
            if (sye==APPLIC) {         /* calculate sye (if unknown)       */
                sye = syntaxOf(textOf(fun(fun(e))));
                if (sye==APPLIC) sye=DEF_OPSYNTAX;
            }
            if (sys==APPLIC) {         /* calculate sys (if unknown)       */
                sys = syntaxOf(textOf(fun(fun(s))));
                if (sys==APPLIC) sys=DEF_OPSYNTAX;
            }

            if (precOf(sye)==precOf(sys) &&                      /* amb    */
                   (assocOf(sye)!=assocOf(sys) || assocOf(sye)==NON_ASS)) {
                ERROR(row) "Ambiguous use of operator \"%s\" with \"%s\"",
                           textToStr(textOf(fun(fun(e)))),
                           textToStr(textOf(fun(fun(s))))
                EEND;
            }
            else if (precOf(sye)>precOf(sys) ||                  /* shift  */
                       (precOf(sye)==precOf(sys) && assocOf(sye)==LEFT_ASS)) {
                temp        = arg(fun(e));
                arg(fun(e)) = s;
                s           = e;
                e           = temp;
                sys         = sye;
                sye         = APPLIC;
            }
            else {                                               /* reduce */
                temp        = arg(fun(s));
                arg(fun(s)) = arg(e);
                arg(e)      = s;
                s           = temp;
                sys         = APPLIC;
                /* sye unchanged */
            }
        }
    }

    e = hd(e);
    while (nonNull(s)) {
        temp        = arg(fun(s));
        arg(fun(s)) = e;
        e           = s;
        s           = temp;
    }

    return e;
}

/*-------------------------------------------------------------------------*/
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 86,
	259, 143,
	44, 143,
	-2, 169,
-1, 95,
	285, 75,
	-2, 74,
-1, 151,
	285, 75,
	-2, 118,
-1, 234,
	264, 19,
	-2, 41,
-1, 270,
	96, 78,
	282, 78,
	-2, 65,
	};
# define YYNPROD 216
# define YYLAST 906
short yyact[]={

  18, 130,  93, 319, 311, 348, 194,  21, 267,   5,
 342, 268, 231, 128, 369, 368, 260,  37,  34,  36,
  52,  53,   6, 110, 298, 202,  95, 218, 312,  62,
  84, 206, 101,  86,  68, 252, 333, 166, 104, 134,
  89, 137, 230, 388, 105, 315, 374, 379, 352, 105,
 364, 213,  86, 141, 316,  85,  72,  36, 148,  42,
  70, 249, 310,  91,  91, 333,  90,  90, 136,  38,
 228, 257, 220, 217,  85, 109, 250, 111, 112, 284,
 254, 147,  66, 102, 299, 177, 365, 263, 214, 371,
 148, 203,  39,  86, 101, 169,  71, 161, 175, 212,
   4,   2,   3, 138, 299, 281, 207,  43, 151, 151,
 153, 277, 275, 226, 181,  85, 180, 258, 184, 101,
 287, 185,  92, 183,  51, 186,  96, 187, 171, 176,
  86, 199, 191, 179, 196, 246, 214, 124,  56, 155,
 387, 125, 149, 280, 205, 102, 193, 101, 101, 232,
 370,  15,  85, 198, 386, 155, 222, 221,  45, 278,
 366,  94,  48, 367, 308, 233, 224,  22, 165, 227,
 102, 159,  10, 272, 255, 223, 162, 148, 120, 235,
 116, 363, 360,  86, 199,  22,  88, 307, 361, 345,
 145, 362, 308, 236, 237, 313, 115, 282, 102, 102,
 283, 114, 238, 233, 101,  85, 248, 190, 253, 148,
  48, 135, 189, 147, 273, 264, 297, 244,  27,  11,
 245, 126,  20, 296, 133, 152, 197, 285, 172, 305,
 266,  46, 269, 279, 162, 261,  27,  22, 139, 164,
  20, 192,  10, 150, 150, 276,  35, 295,  97, 209,
 210, 242, 131,  19, 243, 102, 289, 290, 225,  86,
 328, 300, 233, 302, 303, 291, 294, 286, 237,  99,
 204,  19, 288,  30, 264, 100,  86, 201,  86,  29,
  87,  85,  86, 256, 233, 324, 334, 325,  27,  11,
  29, 351,  20, 331, 199,  30,  22,  61,  85, 320,
  85,  58,  30, 338,  85,  72, 343, 204,  29, 233,
  97, 349, 336, 344, 211, 253, 332, 170, 340, 215,
 337, 346, 339,  19, 350, 354, 327,  30, 335, 314,
 358,  99, 355, 265, 304,  97, 357, 100,  44, 269,
 359, 353, 326, 269,  46,  22, 129,  27,  11, 306,
  10,  20,  45, 322, 154, 182,  99,  86, 199, 375,
 376, 127, 100,  97, 270, 293, 158, 215, 343, 382,
 378, 349, 380, 364, 385, 344, 384, 383, 381,  85,
 320,  22,  19, 132,  99,  99,  10,  77,  78,  79,
 100, 100,  14, 146,  13, 356,  27,  11,  75,  76,
  20,  12, 301, 274,  30,  23,  24,  25,  26, 101,
  29,  81, 131,  82,  30,  16,  80,  83, 271,  67,
  74, 131,  30,  23,  24,  25,  26, 241,  29, 140,
 131,  19,  27,  11,  98, 239,  20, 292, 240, 119,
  22,  99, 120, 251,  49,  10,  30, 100, 117,  91,
   9, 118, 234,  73,  63,  28,   8,  77,  78,  79,
 102, 131,  14, 208,  13,  22, 131,  19,  75,  76,
  10,  12,  40,  47,  30,  23,  24,  25,  26,  64,
  29,  81,  69,  82,  41,  16,  80,  83, 160, 157,
  74,  27,  11, 156,  22,  20, 318, 103, 219,  10,
 216, 144, 174, 131, 106, 173, 373,  65, 142, 143,
 372, 347,   7,  22, 309, 330,  27,  11,  10, 329,
  20,  14, 341,  13, 262, 229,  19, 259, 121, 200,
  12, 168,  59,  30,  23,  24,  25,  26,  60,  29,
 163,  33,  32, 132,  16,  27,  11,  31,   1,  20,
  22,  19, 132,   0,   0,  10,   0,   0,   0,   0,
   0, 377,   0,   0,  27,  11,   0, 178,  20,   0,
  14,   0,  13,   0,   0,   0,  22, 188, 103,  12,
  19,  10,  30,  23,  24,  25,  26,   0,  29,  22,
 114,   0, 132,  16,   0,   0,   0, 132,   0,  19,
   0,  27,  11,  22,   0,  20,  14, 168,  13,   0,
   0,   0,   0,   0,   0,  12,   0,   0,  30,  23,
  24,  25,  26,   0,  29,   0,   0,  27,  11,  16,
   0,  20, 167,   0, 132, 247,  19,   0,   0,   0,
  27,   0,   0,   0,  20,   0,  99,   0,   0,   0,
   0,   0, 100,   0,  27,   0,   7,   0,  20,   0,
  22,   0,  19,   0,   0,  14,   0,  13,   0,   0,
   0,   0,   0,   0,  12,  19,   0,  30,  23,  24,
  25,  26,   0,  29,   0,   0,   0,   0,  16,  19,
  14,   0,  13,   0,   0,   0,   0,   0,   0,  12,
   0,   0,  30,  23,  24,  25,  26,   0,  29,   0,
   7,  27,   0,  16,   0,  20,   0,   0,   0,  14,
   0, 323,   0,   0,   0,   0,   0,   0,  12, 321,
   0,  30,  23,  24,  25,  26, 317,  29,  14,   0,
  13,   0,  16,   0,   0,   0,  19,  12,   0,   0,
  30,  23,  24,  25,  26,   0,  29,   0,   0,   0,
   0,  16,   0,   0,   0,   0,   7,   0,   0,   0,
   0,   0,   0,   0,   0,  14,   0,  13,   0,   0,
   0,   0,   0,   0, 195,   0,   0,  30,  23,  24,
  25,  26,   0,  29,   0,   0,   0,   0,  16,   0,
   0,  14,   0,  13,   0,   0,   0,   0,   0,   0,
  12,   0,   0,  30,  23,  24,  25,  26,   0,  29,
   0,   0,   0,   0,  16,   0,  30,  23,  24,  25,
  26,   0,  29,   0,   0,   0,   0,  16,  17,   0,
  30,  23,  24,  25,  26,   0,  29,   0,   0,   0,
  50,  16,   0,   0,  54,  55,   0,   0,  57,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 107,   0,   0,   0,  54, 108,   0,
   0,   0,   0,   0,   0, 113,   0,  30,  23,  24,
  25,  26,   0,  29, 122, 123 };
short yypact[]={

-156,-1000, 400,-238,-1000,-201,-167,-1000,  62,  62,
 563, 145,   1, 400, 400, 145, 145,-1000,  74, 145,
-1000,-1000, 256,-1000,-1000,-1000,-1000, 400,-1000,-1000,
-1000,-1000, 197,-276,-1000,-1000,-216,-1000,  -1, 107,
 425,-1000,-1000,-1000,-1000,-239,-1000, 425, 145, 620,
-1000, 425,-191,-188,-1000,-1000, 145,-1000, 549, 155,
 139,-1000, 407, 398,  62, 145, 145,  44,  97, 134,
 287,-1000,-1000,-1000,-217,-215, 107,-225,-225,-225,
 137, 107, 107, 107,  95, 110,  74,-1000, 199,-1000,
-1000,-1000, 425,-1000,-248,-1000, 369,-1000,-1000,-1000,
-1000,  54,  -8,  62,  37,  20,-1000, 400,-1000, 296,
-1000, 400,  -2,-1000,-1000,-1000,-1000,-1000, 400,-1000,
 400, 536, 171, 166,-1000, 400, 510, 400,-1000, 127,
-1000,-1000,-1000, -15,-1000,  30,-1000,-1000,  83,-254,
  62,-1000,  62,  62,  55,-1000,-1000,-229,  43,-197,
-258,-1000,-198,-1000, 107,  18,-1000,-201,-1000, 400,
  52,-1000, 400,-200, 169, 296, 107, 107,-1000,-1000,
 161, 394, 164, 210, 176,-1000,  42,-1000, 425,-1000,
-1000,-1000, 341,-214,-193, 425,-1000,-1000,-1000,-1000,
-1000,-184, 130,-1000,  10,  -6,-1000,-1000,-1000,-1000,
-280, 195,  50, 107,-1000, 108,-215, 129,-1000, 129,
 129, 107,  18,-1000, 160, 155,-1000, -11, 107,-1000,
 -12,-1000,-1000,-1000, 115,-1000, 400,  82, -18, 156,
-1000,-1000,-185,-1000, 187,-1000,-1000,-1000,-1000,-1000,
 107,  79,-1000,-1000,-1000, 107,-1000,-1000,-1000, 400,
 400, 378,-1000, -40, 400, 510, 400, 400, 425,-1000,
 189,  50, 146, 120,-1000, 187,-213, -96,-1000, -51,
-1000,  -7,  62,-1000,-1000, 473,-1000, 425, 454, 115,
 400, 197,-1000, 169,-1000,  -4,-1000,-1000,-1000,-1000,
-1000,-1000, 341,-1000,-201, -20, 400,-1000,-1000, 425,
-1000,-1000,-1000,-1000, 296,  25, 148,-1000,  50,-1000,
  18,-1000, 108,   8, 107,-234, 108,-1000, 336,-1000,
-1000,-1000, 296, 400,-1000,-1000, 287,-1000, 141, 147,
 140,-1000, 111,  91,-1000,-1000,-1000,-1000,-1000,-177,
-214, 119,-1000,-282,-283,-1000,-1000, 106,-1000,-170,
-1000,-1000,-237,-1000, -96,-1000, 305,-1000,-191,-1000,
-1000,-1000,   7,-1000, 139, 400,-1000,  25,  18,   7,
  18, 107, 113,  96,-1000,-1000,-1000,-1000,-1000,-232,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-240,-1000 };
short yypgo[]={

   0, 548,   6,  17, 547, 542,  60,  13, 541,  96,
  23, 246, 149, 540, 529, 527, 525,  42,  12,  87,
 524, 522,  10,   0,   7, 519, 515,  30, 211,  26,
 514,   8,   4, 161, 511,   5,   2,  11,  59, 510,
 506, 126, 434, 505, 502, 429, 106, 463, 484, 501,
 190, 142, 500, 498, 496,  75,   3,  22, 493, 489,
 488,  97, 455, 456, 450, 151, 444, 443, 838, 454,
 419,  35, 365, 266, 247,  24, 241, 146,   1 };
short yyr1[]={

   0,   1,   1,   1,   1,   4,   4,   5,   6,   6,
   6,   6,   6,   8,   8,  11,  11,   9,   9,  12,
  12,  13,  13,  16,  16,  17,  17,  14,  14,  14,
  20,  20,  19,  19,  15,  15,  21,  21,  22,  22,
  18,  18,  18,  18,  18,  25,  25,  26,  26,   9,
   9,   9,  28,  28,  28,  30,  30,  34,  34,  35,
  35,  31,  31,  37,  37,  37,  32,  32,  32,  39,
  39,  40,  40,  36,  36,  33,  29,  29,  29,  41,
  41,  42,  42,  42,  42,  42,  42,  42,  42,  42,
  42,  43,  43,  44,  44,   9,   9,   9,  45,  45,
  46,  46,  47,  47,  47,  48,  48,  38,  38,   9,
  49,  49,  49,  50,   9,   9,   9,  51,  51,  52,
  52,  53,  53,  54,  54,  56,  56,  10,  10,  55,
  55,  58,  58,  58,  59,  59,   3,  60,  60,  61,
  61,  61,  27,  27,  23,  23,  62,  62,  24,  24,
   2,   2,   2,  57,  57,  57,  64,  64,  63,  63,
  63,  63,  63,  63,  66,  66,  65,  65,  65,  68,
  68,  68,  68,  68,  68,  68,  68,  68,  68,  68,
  68,  68,  68,  68,  68,  69,  69,  67,  67,  71,
  72,  72,  73,  73,  73,  74,  74,  75,  70,  70,
  70,  70,  70,  70,  70,  70,  76,  76,  77,  77,
  77,  77,   7,   7,  78,  78 };
short yyr2[]={

   0,   2,   3,   2,   1,   3,   1,   1,   3,   3,
   1,   1,   1,   2,   1,   7,   2,   4,   2,   1,
   1,   0,   3,   3,   1,   1,   2,   0,   4,   3,
   0,   1,   3,   1,   0,   4,   3,   1,   3,   3,
   1,   1,   4,   4,   4,   3,   1,   0,   1,   5,
   5,   7,   2,   1,   1,   2,   0,   3,   1,   3,
   1,   3,   1,   3,   1,   1,   0,   2,   4,   0,
   1,   3,   1,   3,   1,   1,   1,   3,   1,   2,
   1,   1,   1,   2,   3,   3,   4,   3,   3,   3,
   2,   2,   1,   3,   3,   3,   3,   3,   1,   0,
   3,   1,   1,   1,   1,   1,   3,   1,   3,   4,
   3,   1,   1,   2,   3,   3,   2,   3,   1,   4,
   0,   4,   0,   3,   1,   1,   1,   3,   2,   3,
   1,   2,   1,   1,   2,   1,   4,   2,   1,   4,
   5,   4,   3,   1,   1,   3,   1,   3,   1,   3,
   3,   1,   1,   1,   3,   1,   3,   5,   2,   4,
   6,   6,   6,   1,   2,   1,   2,   2,   1,   1,
   3,   2,   1,   1,   2,   1,   1,   1,   1,   3,
   3,   3,   4,   4,   4,   3,   3,   3,   1,   2,
   2,   1,   1,   2,   1,   2,   1,   4,   0,   1,
   1,   3,   3,   4,   2,   5,   3,   1,   3,   3,
   1,   4,   2,   1,   1,   1 };
short yychk[]={

-1000,  -1, 257, 258, 256,  -2, -57, 256, -63, -64,
  45,  92, 274, 267, 265, -65, 288, -68, -23, 126,
  95, -24,  40, 278, 279, 280, 281,  91, -62, 283,
 277,  -4,  -5,  -8, 256, -11, 295,  -3, 270, 259,
 -47, -48, -38,  45, 276,  96, 282, -47, -65, -66,
 -68, 123,  -2,  -2, -68, -68,  64, -68,  45, 276,
 282,  41,  -2, -69, -63, -48, -38, -70,  -2, -69,
  -6,  -9, -10, 256, 293, 271, 272, 260, 261, 262,
 289, 284, 286, 290, -27, -57, -23, -11, -12, 256,
 283, 280, 123, -36, -33, -29, -41, 256, -42, 277,
 283,  40,  91, -63, 277, 283, -63, 263, -68, -55,
 -10, 268, 266, -68,  41,  41,  41,  41,  44,  41,
  44, -47, -68, -68,  93,  44, 124, 264,  -7,  59,
 -78, 125, 256, -12, 256, -28, 283, 256, -29, -33,
 -45, 278, -45, -45, -49, -50, 256, -23,  40, -51,
 -33, -29, -51, -29, 259,  44, -58, -59, 256,  61,
 -60, -61, 124, -13,  40, -55, 285, 263, -42,  41,
 263, -29, -41, -43, -44,  44, -29,  93, -47,  96,
  96,  -2,  59,  -7,  -2, 123,  -2,  -2,  41,  41,
  41,  -2, -76, -77,  -2, 274,  -2,  -9, -10, -78,
 -14, 292,  40,  61, 277,  61, 285, -46, -47, -46,
 -46, 259,  44, 280,  45, 276, -52, 270, 285, -53,
 270, -36, -23,  -3,  -2, -61,  61,  -2, 270, -16,
 -17, -18, -12, -23, 283,  -7, -29, -29,  41,  41,
  44, 263,  41,  44,  41,  44,  93, -63, -10, 275,
 269, -67, -71, -57, 264,  44, 273,  61, 123, -15,
 296,  40, -20, -19, -18, 283, -29, -31, -37, -29,
 256, -28,  44, -36, -50, 123, -29, 123,  44,  -2,
  61, 123,  41,  44, 264,  40, -29,  41, -29,  -2,
  -2,  -7,  59, -72, -73, -74, 263, 256, -75, 124,
  -2, -77,  -2,  -2, -55,  40, -19,  41,  44, -30,
 275, -32, 124, 291, -38,  96,  61, -47, -54, -56,
 -10, 256, -55, 267,  -2,  -2,  -6, -17, 264, -25,
 -26, -24, -27,  40, -23, -71,  -3, -75,  -2, -57,
  -7, -21, -22, -23, -24,  41, -18, -34, -35, -23,
 -37, 283,  40, -29, -31,  -7,  59,  -7,  -2,  -7,
  41,  41,  44,  41, 282, 263,  41,  44, 297, 297,
  44, 259, -39, -40, 283, -32, -56, 256, -24,  40,
  -2, -22, -23, -24, -35, -36,  41,  44, 283 };
short yydef[]={

   0,  -2,   0,   0,   4,   1, 151, 152, 153, 155,
   0,   0,   0,   0,   0, 163,   0, 168, 169,   0,
 172, 173,   0, 175, 176, 177, 178, 198, 144, 148,
 146,   3,   0,   6,   7,  14,   0,   2,   0,   0,
   0, 102, 103, 104, 105,   0, 107,   0, 158,   0,
 165,   0,   0,   0, 166, 167,   0, 171,   0, 105,
 107, 174,   0,   0, 153,   0,   0,   0, 199, 200,
   0,  10,  11,  12,   0,   0,   0,  99,  99,  99,
   0,   0,   0,   0,   0,   0,  -2,  13,  21,  16,
  19,  20,   0, 150,   0,  -2,  76,  78,  80,  81,
  82,   0,   0, 154,   0,   0, 156,   0, 164,   0,
 130,   0,   0, 170, 145, 147, 149, 179,   0, 180,
   0,   0,   0,   0, 181,   0,   0, 204,   5,   0,
 213, 214, 215,  27,  18,   0,  53,  54,  75,   0,
   0,  98,   0,   0,   0, 111, 112,   0,   0, 120,
   0,  -2, 122, 116,   0,   0, 128, 132, 133,   0,
 135, 138,   0,   0,   0,   0,   0,   0,  79,  83,
   0,   0,  76,   0,   0,  92,   0,  90,   0, 106,
 108, 159,   0,   0,   0,   0, 186, 185, 182, 183,
 184, 186, 201, 207, 210,   0, 202,   8,   9, 212,
  34,   0,  30,   0,  52,   0,   0,  95, 101,  96,
  97,   0,   0, 113,   0,   0, 114,   0,   0, 115,
   0, 127, 142, 131, 134, 137,   0,   0,   0,   0,
  24,  25,   0,  40,  -2, 136,  73,  77,  84,  85,
   0,   0,  87,  91,  88,   0,  89, 157, 129,   0,
   0,   0, 188,   0, 203,   0,   0,   0,   0,  17,
   0,   0,   0,  31,  33,  41,  56,  66,  62,  64,
  -2,   0,   0, 109, 110,   0, 117,   0,   0,   0,
   0,   0,  22,   0,  26,  47,  94,  86,  93, 160,
 161, 162,   0, 189, 191, 192,   0, 194, 196,   0,
 205, 206, 208, 209,   0,   0,   0,  29,   0,  49,
   0,  50,   0,   0,   0,   0,   0, 100,   0, 124,
 125, 126,   0,   0, 141, 139,   0,  23,   0,   0,
   0,  46,  48,   0, 143, 187, 190, 195, 193,   0,
 211,   0,  37,   0,   0,  28,  32,  55,  58,  60,
  61,  67,  69,  63,  66, 119,   0, 121, 140,  15,
  42,  43,   0,  44,   0,   0,  35,   0,   0,   0,
   0,   0,   0,  70,  72,  51, 123, 126,  45,   0,
 197,  36,  38,  39,  57,  59,  68,   0,  71 };
# line 1 "/usr/lib/yaccpar"
#ifndef lint
static char yaccpar_sccsid[] = "@(#)yaccpar	4.1	(Berkeley)	2/11/83";
#endif not lint

# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps>= &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
#endif
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		yyval = yypv[1];
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 1:
# line 86 "parser.y"
{inputExpr = yypvt[-0];	    sp-=1;} break;
case 2:
# line 87 "parser.y"
{inputExpr = letrec(yypvt[-0],yypvt[-1]); sp-=2;} break;
case 3:
# line 88 "parser.y"
{valDefns  = yypvt[-0];	    sp-=1;} break;
case 4:
# line 89 "parser.y"
{syntaxError("input");} break;
case 5:
# line 102 "parser.y"
{yyval = gc2(yypvt[-1]);} break;
case 6:
# line 103 "parser.y"
{yyval = yypvt[-0];} break;
case 7:
# line 105 "parser.y"
{yyerrok; goOffside(startColumn);} break;
case 8:
# line 107 "parser.y"
{yyval = gc2(yypvt[-2]);} break;
case 9:
# line 108 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 10:
# line 109 "parser.y"
{yyval = gc0(NIL);} break;
case 11:
# line 110 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 12:
# line 111 "parser.y"
{syntaxError("definition");} break;
case 13:
# line 113 "parser.y"
{yyval = gc2(appendOnto(yypvt[-0],yypvt[-1]));} break;
case 14:
# line 114 "parser.y"
{yyval = yypvt[-0];} break;
case 15:
# line 117 "parser.y"
{yyval = gc7(yypvt[-1]);} break;
case 16:
# line 118 "parser.y"
{syntaxError("module definition");} break;
case 17:
# line 120 "parser.y"
{sp-=4;} break;
case 18:
# line 121 "parser.y"
{syntaxError("import declaration");} break;
case 19:
# line 123 "parser.y"
{yyval = yypvt[-0];} break;
case 20:
# line 124 "parser.y"
{yyval = yypvt[-0];} break;
case 21:
# line 126 "parser.y"
{yyval = gc0(NIL);} break;
case 22:
# line 127 "parser.y"
{yyval = gc3(NIL);} break;
case 23:
# line 129 "parser.y"
{yyval = gc3(NIL);} break;
case 24:
# line 130 "parser.y"
{yyval = yypvt[-0];} break;
case 25:
# line 132 "parser.y"
{yyval = yypvt[-0];} break;
case 26:
# line 133 "parser.y"
{yyval = gc2(NIL);} break;
case 27:
# line 135 "parser.y"
{yyval = gc0(NIL);} break;
case 28:
# line 136 "parser.y"
{yyval = gc4(NIL);} break;
case 29:
# line 137 "parser.y"
{yyval = gc3(NIL);} break;
case 30:
# line 139 "parser.y"
{yyval = gc0(NIL);} break;
case 31:
# line 140 "parser.y"
{yyval = yypvt[-0];} break;
case 32:
# line 142 "parser.y"
{yyval = gc3(NIL);} break;
case 33:
# line 143 "parser.y"
{yyval = yypvt[-0];} break;
case 34:
# line 145 "parser.y"
{yyval = gc0(NIL);} break;
case 35:
# line 146 "parser.y"
{yyval = gc4(NIL);} break;
case 36:
# line 148 "parser.y"
{yyval = gc3(NIL);} break;
case 37:
# line 149 "parser.y"
{yyval = yypvt[-0];} break;
case 38:
# line 151 "parser.y"
{yyval = gc3(NIL);} break;
case 39:
# line 152 "parser.y"
{yyval = gc3(NIL);} break;
case 40:
# line 154 "parser.y"
{yyval = yypvt[-0];} break;
case 41:
# line 155 "parser.y"
{yyval = yypvt[-0];} break;
case 42:
# line 156 "parser.y"
{yyval = gc4(NIL);} break;
case 43:
# line 157 "parser.y"
{yyval = gc4(NIL);} break;
case 44:
# line 158 "parser.y"
{yyval = gc4(NIL);} break;
case 45:
# line 160 "parser.y"
{yyval = gc3(NIL);} break;
case 46:
# line 161 "parser.y"
{yyval = yypvt[-0];} break;
case 47:
# line 163 "parser.y"
{yyval = gc0(NIL);} break;
case 48:
# line 164 "parser.y"
{yyval = yypvt[-0];} break;
case 49:
# line 169 "parser.y"
{defTycon(5,yypvt[-2],yypvt[-3],yypvt[-1],yypvt[-0]);} break;
case 50:
# line 171 "parser.y"
{defTycon(5,yypvt[-2],checkTyLhs(yypvt[-3]),
							rev(yypvt[-1]),DATATYPE);} break;
case 51:
# line 174 "parser.y"
{defTycon(7,yypvt[-2],yypvt[-3],
						  ap(QUAL,pair(yypvt[-5],rev(yypvt[-1]))),
						  DATATYPE);} break;
case 52:
# line 178 "parser.y"
{yyval = gc2(ap(yypvt[-1],yypvt[-0]));} break;
case 53:
# line 179 "parser.y"
{yyval = yypvt[-0];} break;
case 54:
# line 180 "parser.y"
{syntaxError("type defn lhs");} break;
case 55:
# line 182 "parser.y"
{yyval = gc2(yypvt[-0]);} break;
case 56:
# line 183 "parser.y"
{yyval = gc0(SYNONYM);} break;
case 57:
# line 185 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 58:
# line 186 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 59:
# line 188 "parser.y"
{yyval = gc3(sigdecl(yypvt[-1],singleton(yypvt[-2]),
							     yypvt[-0]));} break;
case 60:
# line 190 "parser.y"
{yyval = yypvt[-0];} break;
case 61:
# line 192 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 62:
# line 193 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 63:
# line 195 "parser.y"
{yyval = gc3(ap(ap(yypvt[-1],yypvt[-2]),yypvt[-0]));} break;
case 64:
# line 196 "parser.y"
{if (!isCon(getHead(yypvt[-0])))
					     syntaxError("data constructor");
					 yyval = yypvt[-0];} break;
case 65:
# line 199 "parser.y"
{syntaxError("data type definition");} break;
case 66:
# line 201 "parser.y"
{yyval = gc0(NIL);} break;
case 67:
# line 202 "parser.y"
{yyval = gc2(singleton(yypvt[-0]));} break;
case 68:
# line 203 "parser.y"
{yyval = gc4(yypvt[-1]);} break;
case 69:
# line 205 "parser.y"
{yyval = gc0(NIL);} break;
case 70:
# line 206 "parser.y"
{yyval = yypvt[-0];} break;
case 71:
# line 208 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 72:
# line 209 "parser.y"
{yyval = gc1(singleton(yypvt[-0]));} break;
case 73:
# line 220 "parser.y"
{yyval = gc3(ap(QUAL,pair(yypvt[-2],yypvt[-0])));} break;
case 74:
# line 221 "parser.y"
{yyval = yypvt[-0];} break;
case 75:
# line 223 "parser.y"
{yyval = gc1(checkContext(yypvt[-0]));} break;
case 76:
# line 225 "parser.y"
{yyval = yypvt[-0];} break;
case 77:
# line 226 "parser.y"
{yyval = gc3(ap(ap(ARROW,yypvt[-2]),yypvt[-0]));} break;
case 78:
# line 227 "parser.y"
{syntaxError("type expression");} break;
case 79:
# line 229 "parser.y"
{yyval = gc2(ap(yypvt[-1],yypvt[-0]));} break;
case 80:
# line 230 "parser.y"
{yyval = yypvt[-0];} break;
case 81:
# line 232 "parser.y"
{yyval = yypvt[-0];} break;
case 82:
# line 233 "parser.y"
{yyval = yypvt[-0];} break;
case 83:
# line 234 "parser.y"
{yyval = gc2(UNIT);} break;
case 84:
# line 235 "parser.y"
{yyval = gc3(ARROW);} break;
case 85:
# line 236 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 86:
# line 237 "parser.y"
{yyval = gc4(ap(ARROW,yypvt[-2]));} break;
case 87:
# line 238 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 88:
# line 239 "parser.y"
{yyval = gc3(buildTuple(yypvt[-1]));} break;
case 89:
# line 240 "parser.y"
{yyval = gc3(ap(LIST,yypvt[-1]));} break;
case 90:
# line 241 "parser.y"
{yyval = gc2(LIST);} break;
case 91:
# line 243 "parser.y"
{yyval = gc3(mkTuple(tupleOf(yypvt[-1])+1));} break;
case 92:
# line 244 "parser.y"
{yyval = gc1(mkTuple(2));} break;
case 93:
# line 246 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 94:
# line 247 "parser.y"
{yyval = gc3(cons(yypvt[-0],cons(yypvt[-2],NIL)));} break;
case 95:
# line 252 "parser.y"
{fixDefn(LEFT_ASS,yypvt[-2],yypvt[-1],yypvt[-0]); sp-=3;} break;
case 96:
# line 253 "parser.y"
{fixDefn(RIGHT_ASS,yypvt[-2],yypvt[-1],yypvt[-0]);sp-=3;} break;
case 97:
# line 254 "parser.y"
{fixDefn(NON_ASS,yypvt[-2],yypvt[-1],yypvt[-0]);  sp-=3;} break;
case 98:
# line 256 "parser.y"
{yyval = gc1(checkPrec(yypvt[-0]));} break;
case 99:
# line 257 "parser.y"
{yyval = gc0(mkInt(DEF_PREC));} break;
case 100:
# line 259 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 101:
# line 260 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 102:
# line 262 "parser.y"
{yyval = yypvt[-0];} break;
case 103:
# line 263 "parser.y"
{yyval = yypvt[-0];} break;
case 104:
# line 264 "parser.y"
{yyval = gc1(varMinus);} break;
case 105:
# line 266 "parser.y"
{yyval = yypvt[-0];} break;
case 106:
# line 267 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 107:
# line 269 "parser.y"
{yyval = yypvt[-0];} break;
case 108:
# line 270 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 109:
# line 275 "parser.y"
{primDefn(yypvt[-3],yypvt[-2],yypvt[-0]); sp-=4;} break;
case 110:
# line 277 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 111:
# line 278 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 112:
# line 279 "parser.y"
{syntaxError("primitive defn");} break;
case 113:
# line 281 "parser.y"
{yyval = gc2(pair(yypvt[-1],yypvt[-0]));} break;
case 114:
# line 286 "parser.y"
{classDefn(intOf(yypvt[-2]),yypvt[-1],yypvt[-0]); sp-=3;} break;
case 115:
# line 287 "parser.y"
{instDefn(intOf(yypvt[-2]),yypvt[-1],yypvt[-0]);  sp-=3;} break;
case 116:
# line 288 "parser.y"
{sp-=2;} break;
case 117:
# line 290 "parser.y"
{yyval = gc3(pair(yypvt[-2],checkClass(yypvt[-0])));} break;
case 118:
# line 291 "parser.y"
{yyval = gc1(pair(NIL,checkClass(yypvt[-0])));} break;
case 119:
# line 293 "parser.y"
{yyval = gc4(yypvt[-1]);} break;
case 120:
# line 294 "parser.y"
{yyval = gc0(NIL);} break;
case 121:
# line 296 "parser.y"
{yyval = gc4(yypvt[-1]);} break;
case 122:
# line 297 "parser.y"
{yyval = gc0(NIL);} break;
case 123:
# line 299 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 124:
# line 300 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 125:
# line 302 "parser.y"
{yyval = gc1(yypvt[-0]);} break;
case 126:
# line 303 "parser.y"
{syntaxError("class body");} break;
case 127:
# line 308 "parser.y"
{yyval = gc3(sigdecl(yypvt[-1],yypvt[-2],yypvt[-0]));} break;
case 128:
# line 309 "parser.y"
{yyval = gc2(pair(yypvt[-1],yypvt[-0]));} break;
case 129:
# line 311 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 130:
# line 312 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 131:
# line 314 "parser.y"
{yyval = gc2(letrec(yypvt[-0],yypvt[-1]));} break;
case 132:
# line 315 "parser.y"
{yyval = yypvt[-0];} break;
case 133:
# line 316 "parser.y"
{syntaxError("declaration");} break;
case 134:
# line 318 "parser.y"
{yyval = gc2(pair(yypvt[-1],yypvt[-0]));} break;
case 135:
# line 319 "parser.y"
{yyval = gc1(grded(rev(yypvt[-0])));} break;
case 136:
# line 321 "parser.y"
{yyval = gc4(yypvt[-1]);} break;
case 137:
# line 323 "parser.y"
{yyval = gc2(cons(yypvt[-0],yypvt[-1]));} break;
case 138:
# line 324 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 139:
# line 326 "parser.y"
{yyval = gc4(pair(yypvt[-1],pair(yypvt[-2],yypvt[-0])));} break;
case 140:
# line 333 "parser.y"
{yyval = gc5(pair(yypvt[-4],pair(yypvt[-0],yypvt[-3])));} break;
case 141:
# line 334 "parser.y"
{yyval = gc4(pair(yypvt[-3],pair(yypvt[-0],yypvt[-2])));} break;
case 142:
# line 336 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 143:
# line 337 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 144:
# line 339 "parser.y"
{yyval = yypvt[-0];} break;
case 145:
# line 340 "parser.y"
{yyval = gc3(varMinus);} break;
case 146:
# line 342 "parser.y"
{yyval = yypvt[-0];} break;
case 147:
# line 343 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 148:
# line 345 "parser.y"
{yyval = yypvt[-0];} break;
case 149:
# line 346 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 150:
# line 351 "parser.y"
{yyval = gc3(ap(ESIGN,pair(yypvt[-2],yypvt[-0])));} break;
case 151:
# line 352 "parser.y"
{yyval = yypvt[-0];} break;
case 152:
# line 353 "parser.y"
{syntaxError("expression");} break;
case 153:
# line 355 "parser.y"
{yyval = yypvt[-0];} break;
case 154:
# line 356 "parser.y"
{yyval = gc3(ap(ap(yypvt[-1],yypvt[-2]),yypvt[-0]));} break;
case 155:
# line 357 "parser.y"
{yyval = gc1(tidyInfix(yypvt[-0]));} break;
case 156:
# line 359 "parser.y"
{yyval = gc3(ap(ap(yypvt[-1],yypvt[-2]),yypvt[-0]));} break;
case 157:
# line 360 "parser.y"
{yyval = gc5(ap(ap(yypvt[-1],
							ap(ap(yypvt[-3],singleton(yypvt[-4])),
                                                           yypvt[-2])),yypvt[-0]));} break;
case 158:
# line 364 "parser.y"
{if (isInt(yypvt[-0]))
					     yyval = gc2(mkInt(-intOf(yypvt[-0])));
					 else
					     yyval = gc2(ap(varNegate,yypvt[-0]));
					} break;
case 159:
# line 369 "parser.y"
{yyval = gc4(ap(LAMBDA,
						     pair(rev(yypvt[-2]),
						          pair(yypvt[-1],yypvt[-0]))));} break;
case 160:
# line 372 "parser.y"
{yyval = gc6(letrec(yypvt[-3],yypvt[-0]));} break;
case 161:
# line 373 "parser.y"
{yyval = gc6(ap(COND,triple(yypvt[-4],yypvt[-2],yypvt[-0])));} break;
case 162:
# line 374 "parser.y"
{yyval = gc6(ap(CASE,pair(yypvt[-4],rev(yypvt[-1]))));} break;
case 163:
# line 375 "parser.y"
{yyval = yypvt[-0];} break;
case 164:
# line 377 "parser.y"
{yyval = gc2(cons(yypvt[-0],yypvt[-1]));} break;
case 165:
# line 378 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 166:
# line 380 "parser.y"
{yyval = gc2(ap(yypvt[-1],yypvt[-0]));} break;
case 167:
# line 381 "parser.y"
{yyval = gc2(ap(RUNST,yypvt[-0]));} break;
case 168:
# line 382 "parser.y"
{yyval = yypvt[-0];} break;
case 169:
# line 384 "parser.y"
{yyval = yypvt[-0];} break;
case 170:
# line 385 "parser.y"
{yyval = gc3(ap(ASPAT,pair(yypvt[-2],yypvt[-0])));} break;
case 171:
# line 386 "parser.y"
{yyval = gc2(ap(LAZYPAT,yypvt[-0]));} break;
case 172:
# line 387 "parser.y"
{yyval = gc1(WILDCARD);} break;
case 173:
# line 388 "parser.y"
{yyval = yypvt[-0];} break;
case 174:
# line 389 "parser.y"
{yyval = gc2(UNIT);} break;
case 175:
# line 390 "parser.y"
{yyval = yypvt[-0];} break;
case 176:
# line 391 "parser.y"
{yyval = yypvt[-0];} break;
case 177:
# line 392 "parser.y"
{yyval = yypvt[-0];} break;
case 178:
# line 393 "parser.y"
{yyval = yypvt[-0];} break;
case 179:
# line 394 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 180:
# line 395 "parser.y"
{yyval = gc3(buildTuple(yypvt[-1]));} break;
case 181:
# line 396 "parser.y"
{yyval = gc3(yypvt[-1]);} break;
case 182:
# line 397 "parser.y"
{yyval = gc4(ap(yypvt[-1],yypvt[-2]));} break;
case 183:
# line 398 "parser.y"
{yyval = gc4(ap(ap(varFlip,yypvt[-2]),yypvt[-1]));} break;
case 184:
# line 399 "parser.y"
{yyval = gc4(ap(ap(varFlip,yypvt[-2]),yypvt[-1]));} break;
case 185:
# line 401 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 186:
# line 402 "parser.y"
{yyval = gc3(cons(yypvt[-0],cons(yypvt[-2],NIL)));} break;
case 187:
# line 404 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 188:
# line 405 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 189:
# line 407 "parser.y"
{yyval = gc2(pair(yypvt[-1],yypvt[-0]));} break;
case 190:
# line 409 "parser.y"
{yyval = gc2(letrec(yypvt[-0],yypvt[-1]));} break;
case 191:
# line 410 "parser.y"
{yyval = yypvt[-0];} break;
case 192:
# line 412 "parser.y"
{yyval = gc1(grded(rev(yypvt[-0])));} break;
case 193:
# line 413 "parser.y"
{yyval = gc2(pair(yypvt[-1],yypvt[-0]));} break;
case 194:
# line 414 "parser.y"
{syntaxError("case expression");} break;
case 195:
# line 416 "parser.y"
{yyval = gc2(cons(yypvt[-0],yypvt[-1]));} break;
case 196:
# line 417 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 197:
# line 419 "parser.y"
{yyval = gc4(pair(yypvt[-1],pair(yypvt[-2],yypvt[-0])));} break;
case 198:
# line 424 "parser.y"
{yyval = gc0(nameNil);} break;
case 199:
# line 425 "parser.y"
{yyval = gc1(ap(FINLIST,cons(yypvt[-0],NIL)));} break;
case 200:
# line 426 "parser.y"
{yyval = gc1(ap(FINLIST,rev(yypvt[-0])));} break;
case 201:
# line 427 "parser.y"
{yyval = gc3(ap(COMP,pair(yypvt[-2],rev(yypvt[-0]))));} break;
case 202:
# line 428 "parser.y"
{yyval = gc3(ap(ap(varFromTo,yypvt[-2]),yypvt[-0]));} break;
case 203:
# line 429 "parser.y"
{yyval = gc4(ap(ap(varFromThen,yypvt[-3]),yypvt[-1]));} break;
case 204:
# line 430 "parser.y"
{yyval = gc2(ap(varFrom,yypvt[-1]));} break;
case 205:
# line 431 "parser.y"
{yyval = gc5(ap(ap(ap(varFromThenTo,
                                                               yypvt[-4]),yypvt[-2]),yypvt[-0]));} break;
case 206:
# line 434 "parser.y"
{yyval = gc3(cons(yypvt[-0],yypvt[-2]));} break;
case 207:
# line 435 "parser.y"
{yyval = gc1(cons(yypvt[-0],NIL));} break;
case 208:
# line 437 "parser.y"
{yyval = gc3(ap(FROMQUAL,pair(yypvt[-2],yypvt[-0])));} break;
case 209:
# line 438 "parser.y"
{yyval = gc3(ap(QWHERE,
						     singleton(
							pair(yypvt[-2],pair(yypvt[-1],
								     yypvt[-0])))));} break;
case 210:
# line 442 "parser.y"
{yyval = gc1(ap(BOOLQUAL,yypvt[-0]));} break;
case 211:
# line 443 "parser.y"
{yyval = gc4(ap(QWHERE,yypvt[-1]));} break;
case 212:
# line 469 "parser.y"
{yyval = gc2(yypvt[-0]);} break;
case 213:
# line 470 "parser.y"
{yyval = yypvt[-0];} break;
case 214:
# line 472 "parser.y"
{yyval = yypvt[-0];} break;
case 215:
# line 473 "parser.y"
{yyerrok;
                                         if (canUnOffside()) {
                                             unOffside();
					     /* insert extra token on stack*/
					     push(NIL);
					     pushed(0) = pushed(1);
					     pushed(1) = mkInt(column);
					 }
                                         else
                                             syntaxError("definition");
                                        } break;
# line 148 "/usr/lib/yaccpar"

		}
		goto yystack;  /* stack new state and value */

	}
