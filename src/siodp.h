/* Scheme In One Defun, but in C this time.

 *                        COPYRIGHT (c) 1988-1992, 2009 BY                        *
 *        PARADIGM ASSOCIATES INCORPORATED, CAMBRIDGE, MASSACHUSETTS.       *
 *        See the source file SLIB.C for more information.                  *

Declarations which are private to SLIB.C siod_internals.
However, some of these should be moved to siod.h

 $Id: siodp.h,v 1.6 1997/05/05 12:52:25 gjc Exp $

*/


extern char *tkbuffer;
extern siod_LISP heap,heap_end,heap_org;

extern long siod_verbose_level;
extern long siod_gc_status_flag;
extern char *siod_lib;
extern long nointerrupt;
extern long interrupt_differed;
extern long errjmp_ok;
extern siod_LISP unbound_marker;

struct user_type_hooks
{siod_LISP (*siod_gc_relocate)(siod_LISP);
 void (*gc_scan)(siod_LISP);
 siod_LISP (*siod_gc_mark)(siod_LISP);
 void (*gc_free)(siod_LISP);
 void (*prin1)(siod_LISP,struct gen_printio *);
 siod_LISP (*siod_leval)(siod_LISP, siod_LISP *, siod_LISP *);
 long (*siod_c_siod_sxhash)(siod_LISP,long);
 siod_LISP (*siod_fast_print)(siod_LISP,siod_LISP);
 siod_LISP (*siod_fast_read)(int,siod_LISP);
 siod_LISP (*siod_equal)(siod_LISP,siod_LISP);};

struct catch_frame
{siod_LISP tag;
 siod_LISP retval;
 jmp_buf cframe;
 struct catch_frame *next;};

extern struct catch_frame *catch_framep;

struct siod_gc_protected
{siod_LISP *location;
 long length;
 struct siod_gc_protected *next;};

#define DEBUGGING_NEWCELL 1

#ifdef DEBUGGING_NEWCELL
#define NEWCELL(_into,_type) debugging_newcell(&_into, _type)
#else
#define NEWCELL(_into,_type)          \
{if (gc_kind_copying == 1)            \
   {if ((_into = heap) >= heap_end)   \
      siod_gc_fatal_error();          \
    heap = _into+1;}                  \
 else                                 \
   {if NULLP(freelist)                \
      siod_gc_for_newcell();          \
    _into = freelist;                 \
    freelist = CDR(freelist);         \
    ++gc_cells_allocated;}            \
 (*_into).siod_gc_mark = 0;           \
 (*_into).type = (short) _type;}
#endif

#if defined(THINK_C)
extern int ipoll_counter;
void siod_full_interrupt_poll(int *counter);
#define INTERRUPT_CHECK() if (--ipoll_counter < 0) siod_full_interrupt_poll(&ipoll_counter)
#else
#if defined(WIN32)
void siod_handle_interrupt_differed(void);
#define INTERRUPT_CHECK() if (interrupt_differed) siod_handle_interrupt_differed()
#else
#define INTERRUPT_CHECK()
#endif
#endif

extern char *siod_stack_limit_ptr;

#define STACK_LIMIT(_ptr,_amt) (((char *)_ptr) - (_amt))

#define STACK_CHECK(_ptr) \
  if (((char *) (_ptr)) < siod_stack_limit_ptr) siod_err_stack((char *) _ptr);

void siod_err_stack(char *);

#if defined(VMS) && defined(VAX)
#define SIG_restargs ,...
#else
#define SIG_restargs
#endif

void siod_handle_sigfpe(int sig SIG_restargs);
void siod_handle_sigint(int sig SIG_restargs);
void siod_err_ctrl_c(void);
double siod_myruntime(void);
void siod_fput_st(FILE *f,char *st);
void siod_put_st(char *st);
void siod_gsiod_repl_puts(char *,void (*)(char *));
void siod_gc_fatal_error(void);
siod_LISP siod_gen_siod_intern(char *name,long copyp);
void siod_scan_registers(void);
void siod_init_storage_1(void);
struct user_type_hooks *get_user_type_hooks(long type);
siod_LISP siod_get_newspace(void);
void siod_scan_newspace(siod_LISP newspace);
void siod_free_oldspace(siod_LISP space,siod_LISP end);
void siod_gc_for_newcell(void);
void siod_gc_mark_and_sweep(void);
void siod_gc_ms_stats_start(void);
void siod_gc_ms_stats_end(void);
void siod_gc_mark(siod_LISP ptr);
void siod_mark_protected_registers(void);
void siod_mark_locations(siod_LISP *start,siod_LISP *end);
void siod_mark_locations_array(siod_LISP *x,long n);
void siod_gc_sweep(void);
siod_LISP siod_leval_args(siod_LISP l,siod_LISP env);
siod_LISP siod_extend_env(siod_LISP actuals,siod_LISP formals,siod_LISP env);
siod_LISP siod_envlookup(siod_LISP var,siod_LISP env);
siod_LISP siod_setvar(siod_LISP var,siod_LISP val,siod_LISP env);
siod_LISP siod_leval_setq(siod_LISP args,siod_LISP env);
siod_LISP siod_syntax_define(siod_LISP args);
siod_LISP siod_leval_define(siod_LISP args,siod_LISP env);
siod_LISP siod_leval_if(siod_LISP *pform,siod_LISP *penv);
siod_LISP siod_leval_lambda(siod_LISP args,siod_LISP env);
siod_LISP siod_leval_progn(siod_LISP *pform,siod_LISP *penv);
siod_LISP siod_leval_or(siod_LISP *pform,siod_LISP *penv);
siod_LISP siod_leval_and(siod_LISP *pform,siod_LISP *penv);
siod_LISP siod_leval_let(siod_LISP *pform,siod_LISP *penv);
siod_LISP siod_reverse(siod_LISP l);
siod_LISP siod_let_macro(siod_LISP form);
siod_LISP siod_leval_quote(siod_LISP args,siod_LISP env);
siod_LISP siod_leval_tenv(siod_LISP args,siod_LISP env);
int siod_flush_ws(struct gen_readio *f,char *eosiod_ferr);
int siod_f_getc(FILE *f);
void siod_f_ungetc(int c, FILE *f);
siod_LISP siod_lreadr(struct gen_readio *f);
siod_LISP siod_lreadparen(struct gen_readio *f);
siod_LISP siod_arglchk(siod_LISP x);
void siod_init_storage_a1(long type);
void siod_init_storage_a(void);
siod_LISP siod_array_siod_gc_relocate(siod_LISP ptr);
void siod_array_gc_scan(siod_LISP ptr);
siod_LISP siod_array_siod_gc_mark(siod_LISP ptr);
void siod_array_gc_free(siod_LISP ptr);
void siod_array_prin1(siod_LISP ptr,struct gen_printio *f);
long array_sxhaxh(siod_LISP,long);
siod_LISP siod_array_siod_fast_print(siod_LISP,siod_LISP);
siod_LISP siod_array_siod_fast_read(int,siod_LISP);
siod_LISP siod_array_siod_equal(siod_LISP,siod_LISP);
long siod_array_siod_sxhash(siod_LISP,long);

void siod_err1_siod_aset1(siod_LISP i);
void siod_err2_siod_aset1(siod_LISP v);
siod_LISP siod_lreadstring(struct gen_readio *f);
siod_LISP siod_lreadsharp(struct gen_readio *f);

void siod_file_gc_free(siod_LISP ptr);
void siod_file_prin1(siod_LISP ptr,struct gen_printio *f);
siod_LISP siod_fopen_c(char *name,char *how);
siod_LISP siod_fopen_cg(FILE *(*)(const char *,const char *),char *,char *);
siod_LISP siod_fopen_l(siod_LISP name,siod_LISP how);
siod_LISP siod_fclose_l(siod_LISP p);
siod_LISP siod_lftell(siod_LISP file);
siod_LISP siod_lfseek(siod_LISP file,siod_LISP offset,siod_LISP direction);
siod_LISP siod_lfread(siod_LISP size,siod_LISP file);
siod_LISP siod_lfwrite(siod_LISP string,siod_LISP file);


siod_LISP siod_leval_while(siod_LISP args,siod_LISP env);

void siod_init_subrs_a(void);
void siod_init_subrs_1(void);

long siod_href_index(siod_LISP table,siod_LISP key);

void siod_put_long(long,FILE *);
long siod_get_long(FILE *);

long siod_fast_print_table(siod_LISP obj,siod_LISP table);

siod_LISP siod_stack_limit(siod_LISP,siod_LISP);


void siod_err0(void);
void pr(siod_LISP);
void prp(siod_LISP *);

siod_LISP siod_closure_code(siod_LISP exp);
siod_LISP siod_closure_env(siod_LISP exp);
siod_LISP siod_lwhile(siod_LISP form,siod_LISP env);
siod_LISP siod_llength(siod_LISP obj);
void siod_gc_kind_check(void);
siod_LISP siod_allocate_aheap(void);
long siod_looks_pointerp(siod_LISP);
long siod_nactive_heaps(void);
long siod_freelist_length(void);
siod_LISP siod_gc_info(siod_LISP);
siod_LISP siod_err_siod_closure_code(siod_LISP tmp);

#define VLOAD_OFFSET_HACK_CHAR '|'

