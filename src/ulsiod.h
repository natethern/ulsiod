/* Scheme In One Defun, but in C this time.
 
 *                   COPYRIGHT (c) 1988-1994, 2009 BY                             *
 *        PARADIGM ASSOCIATES INCORPORATED, CAMBRIDGE, MASSACHUSETTS.       *
 *        See the source file SLIB.C for more information.                  *

 $Id: siod.h,v 1.8 1998/02/10 12:55:56 gjc Exp $

*/

struct obj
{short siod_gc_mark;
 short type;
 union {struct {struct obj * car;
		struct obj * cdr;} cons;
	struct {double data;} flonum;
	struct {char *pname;
		struct obj * vcell;} symbol;
	struct {char *name;
		struct obj * (*f)(void);} subr0;
  	struct {char *name;
 		struct obj * (*f)(struct obj *);} subr1;
 	struct {char *name;
 		struct obj * (*f)(struct obj *, struct obj *);} subr2;
 	struct {char *name;
 		struct obj * (*f)(struct obj *, struct obj *, struct obj *);
 	      } subr3;
 	struct {char *name;
 		struct obj * (*f)(struct obj *, struct obj *, struct obj *,
				  struct obj *);
 	      } subr4;
 	struct {char *name;
 		struct obj * (*f)(struct obj *, struct obj *, struct obj *,
				  struct obj *,struct obj *);
 	      } subr5;
 	struct {char *name;
 		struct obj * (*f)(struct obj **, struct obj **);} subrm;
	struct {char *name;
		struct obj * (*f)(void *,...);} subr;
	struct {struct obj *env;
		struct obj *code;} closure;
	struct {long dim;
		long *data;} long_array;
	struct {long dim;
		double *data;} double_array;
	struct {long dim;
		char *data;} string;
	struct {long dim;
		unsigned char *data;} u_string;
	struct {long dim;
		signed char *data;} s_string;
	struct {long dim;
		struct obj **data;} lisp_array;
	struct {FILE *f;
		char *name;} c_file;}
 storage_as;};

#define CAR(x) ((*x).storage_as.cons.car)
#define CDR(x) ((*x).storage_as.cons.cdr)
#define PNAME(x) ((*x).storage_as.symbol.pname)
#define VCELL(x) ((*x).storage_as.symbol.vcell)
#define SUBR0(x) (*((*x).storage_as.subr0.f))
#define SUBR1(x) (*((*x).storage_as.subr1.f))
#define SUBR2(x) (*((*x).storage_as.subr2.f))
#define SUBR3(x) (*((*x).storage_as.subr3.f))
#define SUBR4(x) (*((*x).storage_as.subr4.f))
#define SUBR5(x) (*((*x).storage_as.subr5.f))
#define SUBRM(x) (*((*x).storage_as.subrm.f))
#define SUBRF(x) (*((*x).storage_as.subr.f))
#define FLONM(x) ((*x).storage_as.flonum.data)

#define NIL ((struct obj *) 0)
#define EQ(x,y) ((x) == (y))
#define NEQ(x,y) ((x) != (y))
#define NULLP(x) EQ(x,NIL)
#define NNULLP(x) NEQ(x,NIL)

#define TYPE(x) (((x) == NIL) ? 0 : ((*(x)).type))

#define TYPEP(x,y) (TYPE(x) == (y))
#define NTYPEP(x,y) (TYPE(x) != (y))

#define tc_nil    0
#define tc_cons   1
#define tc_flonum 2
#define tc_symbol 3
#define tc_subr_0 4
#define tc_subr_1 5
#define tc_subr_2 6
#define tc_subr_3 7
#define tc_lsubr  8
#define tc_fsubr  9
#define tc_msubr  10
#define tc_closure 11
#define tc_free_cell 12
#define tc_string       13
#define tc_double_array 14
#define tc_long_array   15
#define tc_lisp_array   16
#define tc_c_file       17
#define tc_byte_array   18
#define tc_subr_4 19
#define tc_subr_5 20
#define tc_subr_2n 21
#define FO_comment 35
#define tc_user_min 50
#define tc_user_max 100

#define FO_fetch 127
#define FO_store 126
#define FO_list  125
#define FO_listd 124

#define tc_table_dim 100

typedef struct obj* siod_LISP;
typedef siod_LISP (*SUBR_FUNC)(void); 

#define CONSP(x)   TYPEP(x,tc_cons)
#define FLONUMP(x) TYPEP(x,tc_flonum)
#define SYMBOLP(x) TYPEP(x,tc_symbol)

#define NCONSP(x)   NTYPEP(x,tc_cons)
#define NFLONUMP(x) NTYPEP(x,tc_flonum)
#define NSYMBOLP(x) NTYPEP(x,tc_symbol)

#define TKBUFFERN 5120

#ifndef WIN32
#define __stdcall
#endif


struct gen_readio
{int (*getc_fcn)(void *);
 void (*ungetc_fcn)(int,void *);
 void *cb_argument;};

struct gen_printio
{int (*putc_fcn)(int,void *);
 int (*puts_fcn)(char *,void *);
 void *cb_argument;};

#define GETC_FCN(x) (*((*x).getc_fcn))((*x).cb_argument)
#define UNGETC_FCN(c,x) (*((*x).ungetc_fcn))(c,(*x).cb_argument)
#define PUTC_FCN(c,x) (*((*x).putc_fcn))(c,(*x).cb_argument)
#define PUTS_FCN(c,x) (*((*x).puts_fcn))(c,(*x).cb_argument)

struct siod_repl_hooks
{void (*siod_repl_puts)(char *);
 siod_LISP (*siod_repl_read)(void);
 siod_LISP (*siod_repl_eval)(siod_LISP);
 void (*siod_repl_print)(siod_LISP);};

void __stdcall ulsiod_process_cla(int argc,char **argv,int warnflag);
void __stdcall ulsiod_print_welcome(void);
void __stdcall ulsiod_print_hs_1(void);
void __stdcall ulsiod_print_hs_2(void);
long siod_no_interrupt(long n);
siod_LISP siod_get_eof_val(void);
long siod_repl_driver(long want_sigint,long want_init,struct siod_repl_hooks *);
void siod_set_siod_repl_hooks(void (*puts_f)(char *),
		    siod_LISP (*read_f)(void),
		    siod_LISP (*eval_f)(siod_LISP),
		    void (*print_f)(siod_LISP));
long siod_repl(struct siod_repl_hooks *);
siod_LISP err(const char *message, siod_LISP x);
siod_LISP siod_errswitch(void);
char *get_c_string(siod_LISP x);
char *get_c_siod_string_dim(siod_LISP x,long *);
char *try_get_c_string(siod_LISP x);
long siod_get_c_long(siod_LISP x);
double siod_get_c_double(siod_LISP x);
siod_LISP siod_lerr(siod_LISP message, siod_LISP x);

extern siod_LISP sym_t;
siod_LISP siod_newcell(long type);
siod_LISP siod_cons(siod_LISP x,siod_LISP y);
siod_LISP siod_consp(siod_LISP x);
siod_LISP siod_car(siod_LISP x);
siod_LISP siod_cdr(siod_LISP x);
siod_LISP siod_setcar(siod_LISP cell, siod_LISP value);
siod_LISP siod_setcdr(siod_LISP cell, siod_LISP value);
siod_LISP siod_flocons(double x);
siod_LISP siod_numberp(siod_LISP x);
siod_LISP siod_plus(siod_LISP x,siod_LISP y);
siod_LISP siod_ltimes(siod_LISP x,siod_LISP y);
siod_LISP siod_difference(siod_LISP x,siod_LISP y);
siod_LISP siod_Quotient(siod_LISP x,siod_LISP y);
siod_LISP siod_greaterp(siod_LISP x,siod_LISP y);
siod_LISP siod_lessp(siod_LISP x,siod_LISP y);
siod_LISP siod_eq(siod_LISP x,siod_LISP y);
siod_LISP siod_eql(siod_LISP x,siod_LISP y);
siod_LISP siod_symcons(char *pname,siod_LISP vcell);
siod_LISP siod_symbolp(siod_LISP x);
siod_LISP siod_symbol_boundp(siod_LISP x,siod_LISP env);
siod_LISP siod_symbol_value(siod_LISP x,siod_LISP env);
siod_LISP siod_cintern(char *name);
siod_LISP siod_rintern(char *name);
siod_LISP siod_subrcons(long type, char *name, SUBR_FUNC f);
siod_LISP siod_closure(siod_LISP env,siod_LISP code);
void siod_gc_protect(siod_LISP *location);
void siod_gc_protect_n(siod_LISP *location,long n);
void siod_gc_protect_sym(siod_LISP *location,char *st);

void __stdcall siod_init_storage(void);
void __stdcall siod_init_slibu(void);

void siod_init_subr(char *name, long type, SUBR_FUNC fcn);
void siod_init_subr_0(char *name, siod_LISP (*fcn)(void));
void siod_init_subr_1(char *name, siod_LISP (*fcn)(siod_LISP));
void siod_init_subr_2(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP));
void siod_init_subr_2n(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP));
void siod_init_subr_3(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP,siod_LISP));
void siod_init_subr_4(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP,siod_LISP,siod_LISP));
void siod_init_subr_5(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP,siod_LISP,siod_LISP,siod_LISP));
void siod_init_lsubr(char *name, siod_LISP (*fcn)(siod_LISP));
void siod_init_fsubr(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP));
void siod_init_msubr(char *name, siod_LISP (*fcn)(siod_LISP *,siod_LISP *));

siod_LISP siod_assq(siod_LISP x,siod_LISP alist);
siod_LISP siod_delq(siod_LISP elem,siod_LISP l);
void siod_set_gc_hooks(long type,
		  siod_LISP (*rel)(siod_LISP),
		  siod_LISP (*mark)(siod_LISP),
		  void (*scan)(siod_LISP),
		  void (*free)(siod_LISP),
		  long *kind);
siod_LISP siod_gc_relocate(siod_LISP x);
siod_LISP siod_user_gc(siod_LISP args);
extern int siod_gc_count;
siod_LISP siod_gc_status(siod_LISP args);
void siod_set_eval_hooks(long type,siod_LISP (*fcn)(siod_LISP, siod_LISP *, siod_LISP *));
siod_LISP siod_leval(siod_LISP x,siod_LISP env);
siod_LISP siod_symbolconc(siod_LISP args);
void siod_set_print_hooks(long type,void (*fcn)(siod_LISP, struct gen_printio *));
siod_LISP siod_lprin1g(siod_LISP exp,struct gen_printio *f);
siod_LISP siod_lprin1f(siod_LISP exp,FILE *f);
siod_LISP siod_lprint(siod_LISP exp,siod_LISP);
siod_LISP siod_lread(siod_LISP);
siod_LISP siod_lreadtk(char *,long j);
siod_LISP siod_lreadf(FILE *f);
void siod_set_read_hooks(char *all_set,char *end_set,
		    siod_LISP (*fcn1)(int, struct gen_readio *),
		    siod_LISP (*fcn2)(char *,long, int *));
siod_LISP siod_apropos(siod_LISP);
siod_LISP siod_vload(char *fname,long cflag,long rflag);
siod_LISP siod_load(siod_LISP fname,siod_LISP cflag,siod_LISP rflag);
siod_LISP siod_require(siod_LISP fname);
siod_LISP siod_save_forms(siod_LISP fname,siod_LISP forms,siod_LISP how);
siod_LISP siod_quit(void);
siod_LISP siod_nullp(siod_LISP x);
siod_LISP siod_strcons(long length,const char *data);
siod_LISP siod_read_from_string(siod_LISP x);
siod_LISP siod_aref1(siod_LISP a,siod_LISP i);
siod_LISP siod_aset1(siod_LISP a,siod_LISP i,siod_LISP v);
siod_LISP siod_cons_array(siod_LISP dim,siod_LISP kind);
siod_LISP siod_ar_cons(long typecode,long n,long initp);
siod_LISP siod_string_append(siod_LISP args);
siod_LISP siod_string_length(siod_LISP string);
siod_LISP siod_string_search(siod_LISP,siod_LISP);
siod_LISP siod_substring(siod_LISP,siod_LISP,siod_LISP);
siod_LISP siod_string_trim(siod_LISP);
siod_LISP siod_string_trim_left(siod_LISP);
siod_LISP siod_string_trim_right(siod_LISP);
siod_LISP siod_string_upcase(siod_LISP);
siod_LISP siod_string_downcase(siod_LISP);
void __stdcall siod_init_subrs(void);
siod_LISP siod_copy_list(siod_LISP);
long siod_c_siod_sxhash(siod_LISP,long);
siod_LISP siod_sxhash(siod_LISP,siod_LISP);
siod_LISP href(siod_LISP,siod_LISP);
siod_LISP hset(siod_LISP,siod_LISP,siod_LISP);
siod_LISP siod_fast_print(siod_LISP,siod_LISP);
siod_LISP siod_fast_read(siod_LISP);
siod_LISP siod_equal(siod_LISP,siod_LISP);
siod_LISP siod_assoc(siod_LISP x,siod_LISP alist);
siod_LISP siod_make_list(siod_LISP x,siod_LISP v);
void siod_set_fatal_exit_hook(void (*fcn)(void));
siod_LISP siod_parse_number(siod_LISP x);
siod_LISP siod_intern(siod_LISP x);
void __stdcall siod_init_trace(void);
long __stdcall siod_repl_c_string(char *,long want_sigint,long want_init,long want_print);
char * __stdcall siod_version(void);
siod_LISP siod_nreverse(siod_LISP);
siod_LISP siod_number2string(siod_LISP,siod_LISP,siod_LISP,siod_LISP);
siod_LISP siod_string2number(siod_LISP,siod_LISP);
siod_LISP siod_verbose(siod_LISP);
int __stdcall siod_verbose_check(int);
siod_LISP siod_setvar(siod_LISP,siod_LISP,siod_LISP);
long siod_allocate_user_tc(void);
siod_LISP siod_cadr(siod_LISP);
siod_LISP siod_caar(siod_LISP);
siod_LISP siod_cddr(siod_LISP);
siod_LISP siod_caaar(siod_LISP);
siod_LISP siod_caadr(siod_LISP);
siod_LISP siod_cadar(siod_LISP);
siod_LISP siod_caddr(siod_LISP);
siod_LISP siod_cdaar(siod_LISP);
siod_LISP siod_cdadr(siod_LISP);
siod_LISP siod_cddar(siod_LISP);
siod_LISP siod_cdddr(siod_LISP);
void siod_chk_string(siod_LISP,char **,long *);
siod_LISP siod_a_true_value(void);
siod_LISP siod_lapply(siod_LISP fcn,siod_LISP args);
siod_LISP siod_mallocl(void *lplace,long size);
void siod_gput_st(struct gen_printio *,char *);
void siod_put_st(char *st);
siod_LISP siod_listn(long n, ...);
char *must_malloc(unsigned long size);
siod_LISP siod_lstrbreakup(siod_LISP str,siod_LISP lmarker);
siod_LISP siod_lstrunbreakup(siod_LISP elems,siod_LISP lmarker);
siod_LISP siod_nconc(siod_LISP,siod_LISP);
siod_LISP siod_poparg(siod_LISP *,siod_LISP);
FILE *siod_get_c_file(siod_LISP p,FILE *deflt);
char *last_c_errmsg(int);
siod_LISP siod_llast_c_errmsg(int);

#define SAFE_STRCPY(_to,_from) safe_strcpy((_to),sizeof(_to),(_from))
#define SAFE_STRCAT(_to,_from) safe_strcat((_to),sizeof(_to),(_from))
#define SAFE_STRLEN(_buff) siod_safe_strlen((_buff),sizeof(_buff))

char *safe_strcpy(char *s1,size_t size1,const char *s2);
char *safe_strcat(char *s1,size_t size1,const char *s2);

size_t siod_safe_strlen(const char *s,size_t size);
siod_LISP siod_memq(siod_LISP x,siod_LISP il);
siod_LISP siod_lstrbreakup(siod_LISP,siod_LISP);
siod_LISP siod_lstrbreakup(siod_LISP,siod_LISP);
siod_LISP siod_nth(siod_LISP,siod_LISP);
siod_LISP siod_butlast(siod_LISP);
siod_LISP siod_last(siod_LISP);
siod_LISP siod_readtl(struct gen_readio *f);
siod_LISP siod_funcall1(siod_LISP,siod_LISP);
siod_LISP siod_funcall2(siod_LISP,siod_LISP,siod_LISP);
siod_LISP siod_apply1(siod_LISP,siod_LISP,siod_LISP);
siod_LISP siod_lgetc(siod_LISP p);
siod_LISP siod_lungetc(siod_LISP i,siod_LISP p);
siod_LISP siod_lputc(siod_LISP c,siod_LISP p);
siod_LISP siod_lputs(siod_LISP str,siod_LISP p);

int siod_assemble_options(siod_LISP, ...);
siod_LISP siod_ccall_catch(siod_LISP tag,siod_LISP (*fcn)(void *),void *);
siod_LISP siod_lref_default(siod_LISP li,siod_LISP x,siod_LISP fcn);


siod_LISP siod_symalist(char *item,...);

siod_LISP siod_encode_st_mode(siod_LISP l);
siod_LISP siod_encode_open_flags(siod_LISP l);
long siod_nlength(siod_LISP obj);
int __stdcall ulsiod_main(int argc,char **argv, char **env);
void __stdcall ulsiod_shuffle_args(int *pargc,char ***pargv);
void __stdcall ulsiod_init(int argc,char **argv);

/* Some transferred from siodp.h 20090820 JCGS, in hope that we can
   avoid using a repl-type function for our ordinary evaluator */

siod_LISP siod_leval_catch_1(siod_LISP forms,siod_LISP env);
siod_LISP siod_leval_catch(siod_LISP args,siod_LISP env);
siod_LISP siod_lthrow(siod_LISP tag,siod_LISP value);

int siod_rfs_getc(unsigned char **p);
void siod_rfs_ungetc(unsigned char c,unsigned char **p);

void siod_gc_stop_and_copy(void);

/* Made extern 20090904 JCGS, in hope that we can do better error
   reporting from callers. */
extern siod_LISP sym_errobj;

/* A debugging function, written when something was corrupting the obarray */
extern void siod_check_obarray(const char *label);

/* ulsiod.h ends here */
