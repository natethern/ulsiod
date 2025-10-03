/* Scheme In One Defun, but in C this time.
 
 *                      COPYRIGHT (c) 1988-1997, 2009 BY                          *
 *        PARADIGM ASSOCIATES INCORPORATED, CAMBRIDGE, MASSACHUSETTS.       *
 *			   ALL RIGHTS RESERVED                              *

Permission to use, copy, modify, distribute and sell this software
and its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all copies
and that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Paradigm Associates
Inc not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

PARADIGM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
PARADIGM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*/

/*

gjc@world.std.com

   Release 1.0: 24-APR-88
   Release 1.1: 25-APR-88, added: macros, predicates, siod_load. With additions by
    Barak.Pearlmutter@DOGHEN.BOLTZ.CS.CMU.EDU: Full flonum recognizer,
    cleaned up uses of NULL/0. Now distributed with siod.scm.
   Release 1.2: 28-APR-88, name changes as requested by JAR@AI.AI.MIT.EDU,
    siod_plus some bug fixes.
   Release 1.3: 1-MAY-88, changed env to use frames instead of alist.
    define now works properly. vms specific function edit.
   Release 1.4 20-NOV-89. Minor Cleanup and remodularization.
    Now in 3 files, siod.h, slib.c, siod.c. Makes it easier to write your
    own main loops. Some short-int changes for lightspeed C included.
   Release 1.5 29-NOV-89. Added startup flag -g, select stop and copy
    or mark-and-sweep garbage collection, which assumes that the stack/register
    marking code is correct for your architecture. 
   Release 2.0 1-DEC-89. Added siod_repl_hooks, Catch, Throw. This is significantly
    different enough (from 1.3) now that I'm calling it a major release.
   Release 2.1 4-DEC-89. Small reader features, dot, backquote, comma.
   Release 2.2 5-DEC-89. gc,read,print,eval, hooks for user defined datatypes.
   Release 2.3 6-DEC-89. siod_save_forms, obarray siod_intern mechanism. comment char.
   Release 2.3a......... minor speed-ups. i/o interrupt siod_considerations.
   Release 2.4 27-APR-90 gen_readr, for read-from-string.
   Release 2.5 18-SEP-90 arrays added to SIOD.C by popular demand. inums.
   Release 2.6 11-MAR-92 function prototypes, some remodularization.
   Release 2.7 20-MAR-92 hash tables, fassiod_load. Stack check.
   Release 2.8  3-APR-92 Bug fixes, \n syntax in string reading.
   Release 2.9 28-AUG-92 gc sweep bug fix. fseek, ftell, etc. Change to
    siod_envlookup to allow (a . rest) suggested by bowles@is.s.u-tokyo.ac.jp.
   Release 2.9a 10-AUG-93. Minor changes for Windows NT.
   Release 3.0  1-MAY-94. Release it, include changes/cleanup recommended by
    andreasg@nynexst.com for the OS2 C++ compiler. Compilation and running
    tested using DEC C, VAX C. WINDOWS NT. GNU C on SPARC. Storage
    management improvements, more string functions. SQL support.
   Release 3.1? -JUN-95 verbose flag, other integration improvements for htqs.c
    hpux by denson@sdd.hp.com, solaris by pgw9@columbia.edu.
   Release 3.2X MAR-96. dynamic linking, subr siod_closures, other improvements.
   Release 3.2  12-JUN-96. Bug fixes. Call it a release.
   Release 3.3x cleanup for gcc -Wall.
   Release 3.4 win95 cleanup.
   Release 3.5 5-MAY-97 fixes, siod_plus win95 "compiler" to create exe files.

john.sturdy@ul.ie

   Put siod_ at start of names.
   Add IntQuotient, Remainder, ClosureP.
  */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "ulsiod.h"
#include "siodp.h"

static void siod_init_slib_version(void)
{siod_setvar(siod_cintern("*slib-version*"),
	siod_cintern("$Id: slib.c,v 1.16 1997/12/05 14:44:44 gjc Exp $"),
	NIL);}

char * __stdcall siod_version(void)
{return("3.5 5-MAY-97");}

long nheaps = 2;
siod_LISP *heaps;
siod_LISP heap,heap_end,heap_org;
long heap_size = 5000;
long old_heap_used;
long siod_gc_status_flag = 1;
long siod_gc_messages_flag = 0;
char *init_file = (char *) NULL;
char *tkbuffer = NULL;
long gc_kind_copying = 0;
long gc_cells_allocated = 0;
double gc_time_taken;
siod_LISP *stack_start_ptr = NULL;
siod_LISP freelist;
jmp_buf errjmp;
long errjmp_ok = 0;
long nointerrupt = 1;
long interrupt_differed = 0;
siod_LISP oblistvar = NIL;
siod_LISP sym_t = NIL;
siod_LISP eof_val = NIL;
siod_LISP sym_errobj = NIL;
siod_LISP sym_catchall = NIL;
siod_LISP sym_progn = NIL;
siod_LISP sym_lambda = NIL;
siod_LISP sym_quote = NIL;
siod_LISP sym_dot = NIL;
siod_LISP sym_after_gc = NIL;
siod_LISP sym_eval_history_ptr = NIL;
siod_LISP unbound_marker = NIL;
siod_LISP *obarray;
long obarray_dim = 100;
struct catch_frame *catch_framep = (struct catch_frame *) NULL;
void (*siod_repl_puts)(char *) = NULL;
siod_LISP (*siod_repl_read)(void) = NULL;
siod_LISP (*siod_repl_eval)(siod_LISP) = NULL;
void (*siod_repl_print)(siod_LISP) = NULL;
siod_LISP *inums;
long inums_dim = 256;
struct user_type_hooks *user_types = NULL;
long user_tc_next = tc_user_min;
struct siod_gc_protected *protected_registers = NULL;
jmp_buf save_regs_siod_gc_mark;
double gc_rt;
long gc_cells_collected;
char *user_ch_readm = "";
char *user_te_readm = "";
siod_LISP (*user_readm)(int, struct gen_readio *) = NULL;
siod_LISP (*user_readt)(char *,long, int *) = NULL;
void (*fatal_exit_hook)(void) = NULL;
#ifdef THINK_C
int ipoll_counter = 0;
#endif

char *siod_stack_limit_ptr = NULL;
long stack_size = 
#ifdef THINK_C
  10000;
#else
  50000;
#endif

long siod_verbose_level = 4;

char *siod_lib = SIOD_LIB_DEFAULT;

void __stdcall ulsiod_process_cla(int argc,char **argv,int warnflag)
{int k;
 char *ptr;

 static int siod_lib_set = 0;
#if !defined(vms)
 if (!siod_lib_set)
   {
#ifdef WIN32
	 if (argc > 0)
	 {siod_lib = strdup(argv[0]);
	  siod_lib_set = 1;
	  if ((ptr = strrchr(siod_lib,'\\')))
		  ptr[1] = 0;}
#endif
	 if (getenv("SIOD_LIB"))
      {siod_lib = getenv("SIOD_LIB");
       siod_lib_set = 1;}}
#endif
 for(k=1;k<argc;++k)
   {if (strlen(argv[k])<2) continue;
    if (argv[k][0] != '-')
      {if (warnflag) printf("bad arg: %s\n",argv[k]);continue;}
    switch(argv[k][1])
      {case 'l':
	 siod_lib = &argv[k][2];
	 break;
       case 'h':
	 heap_size = atol(&(argv[k][2]));
         if ((ptr = strchr(&(argv[k][2]),':')))
           nheaps = atol(&ptr[1]);
	 break;
       case 'o':
	 obarray_dim = atol(&(argv[k][2]));
	 break;
       case 'i':
	 init_file = &(argv[k][2]);
	 break;
       case 'n':
	 inums_dim = atol(&(argv[k][2]));
	 break;
       case 'g':
	 gc_kind_copying = atol(&(argv[k][2]));
	 break;
       case 's':
	 stack_size = atol(&(argv[k][2]));
	 break;
       case 'v':
	 siod_verbose_level = atol(&(argv[k][2]));
	 break;
       default:
	 if (warnflag) printf("bad arg: %s\n",argv[k]);}}}

void __stdcall ulsiod_print_welcome(void)
{if (siod_verbose_level >= 2)
   {printf("Welcome to SIOD, Scheme In One Defun, Version %s\n",
	   siod_version());
    printf("(C) Copyright 1988-1994 Paradigm Associates Inc.\n");}}

void __stdcall ulsiod_print_hs_1(void)
{if (siod_verbose_level >= 2)
   {printf("%ld heaps. size = %ld cells, %ld bytes. %ld inums. GC is %s\n",
	   nheaps,
	   heap_size,heap_size*sizeof(struct obj),
	   inums_dim,
	   (gc_kind_copying == 1) ? "stop and copy" : "mark and sweep");}}


void __stdcall ulsiod_print_hs_2(void)
{if (siod_verbose_level >= 2)
   {if (gc_kind_copying == 1)
      printf("heaps[0] at %p, heaps[1] at %p\n",heaps[0],heaps[1]);
   else
     printf("heaps[0] at %p\n",heaps[0]);}}

long siod_no_interrupt(long n)
{long x;
 x = nointerrupt;
 nointerrupt = n;
 if ((nointerrupt == 0) && (interrupt_differed == 1))
   {interrupt_differed = 0;
    siod_err_ctrl_c();}
 return(x);}

void siod_handle_sigfpe(int sig SIG_restargs)
{
#ifdef WIN32
 _fpreset();
#endif
 signal(SIGFPE,siod_handle_sigfpe);
 err("floating point exception",NIL);}

void siod_handle_sigint(int sig SIG_restargs)
{signal(SIGINT,siod_handle_sigint);
#if defined(WIN32)
   interrupt_differed = 1;
#else
 if (nointerrupt == 1)
   interrupt_differed = 1;
 else
   siod_err_ctrl_c();
#endif
}

#if defined(WIN32)
void siod_handle_interrupt_differed(void)
{interrupt_differed = 0;
 siod_err_ctrl_c();}
#endif

void siod_err_ctrl_c(void)
{err("control-c interrupt",NIL);}

siod_LISP siod_get_eof_val(void)
{return(eof_val);}

long siod_repl_driver(long want_sigint,long want_init,struct siod_repl_hooks *h)
{int k;
 long rv;
 struct siod_repl_hooks hd;
 siod_LISP stack_start;
 static void (*osigint)(int);
 static void (*osigfpe)(int);
 stack_start_ptr = &stack_start;
 siod_stack_limit_ptr = STACK_LIMIT(stack_start_ptr,stack_size);
 k = setjmp(errjmp);
 if (k == 2)
   {if (want_sigint) signal(SIGINT,osigint);
    signal(SIGFPE,osigfpe);
    stack_start_ptr = NULL;
    siod_stack_limit_ptr = NULL;
    return(2);}
 if (want_sigint) osigint = signal(SIGINT,siod_handle_sigint);
#ifdef WIN32_X
 /* doesn't work, because library functions like atof
    depend on default setting, or some other reason I didn't
    have time to investigate. */
 _controlfp(_EM_INVALID,
	        _MCW_EM);
#endif
 osigfpe = signal(SIGFPE,siod_handle_sigfpe);
 catch_framep = (struct catch_frame *) NULL;
 errjmp_ok = 1;
 interrupt_differed = 0;
 nointerrupt = 0;
 if (want_init && init_file && (k == 0)) siod_vload(init_file,0,1);
 if (!h)
   {hd.siod_repl_puts = siod_repl_puts;
    hd.siod_repl_read = siod_repl_read;
    hd.siod_repl_eval = siod_repl_eval;
    hd.siod_repl_print = siod_repl_print;
    rv = siod_repl(&hd);}
 else
   rv = siod_repl(h);
 if (want_sigint) signal(SIGINT,osigint);
 signal(SIGFPE,osigfpe);
 stack_start_ptr = NULL;
 siod_stack_limit_ptr = NULL;
 return(rv);}

static void siod_ignore_puts(char *st)
{}

static void siod_noprompt_puts(char *st)
{if (strcmp(st,"> ") != 0)
   siod_put_st(st);}

static char *siod_repl_c_string_arg = NULL;
static char *siod_repl_c_string_out = NULL;
static long siod_repl_c_string_flag = 0;
static long siod_repl_c_string_print_len = 0;


static siod_LISP siod_repl_c_string_read(void)
{siod_LISP s;
 if (siod_repl_c_string_arg == NULL)
   return(siod_get_eof_val());
 s = siod_strcons(strlen(siod_repl_c_string_arg),siod_repl_c_string_arg);
 siod_repl_c_string_arg = NULL;
 if (siod_repl_c_string_out) siod_repl_c_string_out[0] = 0;
 return(siod_read_from_string(s));}

static void siod_ignore_print(siod_LISP x)
{siod_repl_c_string_flag = 1;}

static void siod_not_siod_ignore_print(siod_LISP x)
{siod_repl_c_string_flag = 1;
 siod_lprint(x,NIL);}

struct siod_rcsp_puts
{char *ptr;
 char *end;};

static int siod_rcsp_puts(char *from,void *cb)
{long fromlen,intolen,cplen;
 struct siod_rcsp_puts *p = (struct siod_rcsp_puts *) cb;
 fromlen = strlen(from);
 intolen = p->end - p->ptr;
 cplen = (fromlen > intolen) ? intolen : fromlen;
 memcpy(p->ptr,from,cplen);
 p->ptr = &p->ptr[cplen];
 *p->ptr = 0;
 if (cplen != fromlen)
   err("siod_repl_c_string_print overflow",NIL);
 return(1);}

static void siod_repl_c_string_print(siod_LISP x)
{struct gen_printio s;
 struct siod_rcsp_puts p;
 s.putc_fcn = NULL;
 s.puts_fcn = siod_rcsp_puts;
 p.ptr = siod_repl_c_string_out;
 p.end = &siod_repl_c_string_out[siod_repl_c_string_print_len - 1];
 s.cb_argument = &p;
 siod_lprin1g(x,&s);
 siod_repl_c_string_flag = 1;}

long __stdcall siod_repl_c_string(char *str, long want_sigint,long want_init,long want_print)
{struct siod_repl_hooks h;
 long retval;
 h.siod_repl_read = siod_repl_c_string_read;
 h.siod_repl_eval = NULL;
 if (want_print > 1)
   {h.siod_repl_puts = siod_ignore_puts;
    h.siod_repl_print = siod_repl_c_string_print;
    siod_repl_c_string_print_len = want_print;
    siod_repl_c_string_out = str;}
 else if (want_print)
   {h.siod_repl_puts = siod_noprompt_puts;
    h.siod_repl_print = siod_not_siod_ignore_print;
    siod_repl_c_string_print_len = 0;
    siod_repl_c_string_out = NULL;}
 else
   {h.siod_repl_puts = siod_ignore_puts;
    h.siod_repl_print = siod_ignore_print;
    siod_repl_c_string_print_len = 0;
    siod_repl_c_string_out = NULL;}
 siod_repl_c_string_arg = str;
 siod_repl_c_string_flag = 0;
 retval = siod_repl_driver(want_sigint,want_init,&h);
 if (retval != 0)
   return(retval);
 else if (siod_repl_c_string_flag == 1)
   return(0);
 else
  return(2);}

#ifdef unix
#include <sys/types.h>
#include <sys/times.h>
#ifdef sun
#include <limits.h>
#endif
#ifndef CLK_TCK
#define CLK_TCK 60
#endif
double siod_myruntime(void)
{double total;
 struct tms b;
 times(&b);
 total = b.tms_utime;
 total += b.tms_stime;
 return(total / (double)CLK_TCK);}
#else
#if defined(THINK_C) | defined(WIN32) | defined(VMS)
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC CLK_TCK
#endif
double siod_myruntime(void)
{return(((double) clock()) / ((double) CLOCKS_PER_SEC));}
#else
double siod_myruntime(void)
{time_t x;
 time(&x);
 return((double) x);}
#endif
#endif

#if defined(__osf__)
#include <sys/timers.h>
#ifndef TIMEOFDAY
#define TIMEOFDAY 1
#endif
double siod_myrealtime(void)
{struct timespec x;
 if (!getclock(TIMEOFDAY,&x))
   return(x.tv_sec + (((double) x.tv_nsec) * 1.0e-9));
 else
   return(0.0);}
#endif

#if defined(VMS)
#include <ssdef.h>
#include <starlet.h>

double siod_myrealtime(void)
{unsigned long x[2];
 static double c = 0.0;
 if (sys$gettim(&x) == SS$_NORMAL)
   {if (c == 0.0) c = pow((double) 2,(double) 31) * 100.0e-9;
    return(x[0] * 100.0e-9 + x[1] * c);}
 else
   return(0.0);}

#endif

#if defined(SUN5) || defined(linux)

#if defined(linux)
#include <sys/time.h>
#endif

double siod_myrealtime(void)
{struct timeval x;
 if (gettimeofday(&x,NULL))
   return(0.0);
 return(((double) x.tv_sec) + ((double) x.tv_usec) * 1.0E-6);}
#endif

#if defined(WIN32)
#include <sys/timeb.h>
double siod_myrealtime(void)
{struct _timeb x;
 _ftime(&x);
 return(x.time + ((double) x.millitm) * 0.001);}
#endif

#if !defined(__osf__) & !defined(VMS) & !defined(SUN5) & !defined(WIN32) &!defined(linux)
double siod_myrealtime(void)
{time_t x;
 time(&x);
 return((double) x);}
#endif

void siod_set_siod_repl_hooks(void (*puts_f)(char *),
		    siod_LISP (*read_f)(void),
		    siod_LISP (*eval_f)(siod_LISP),
		    void (*print_f)(siod_LISP))
{siod_repl_puts = puts_f;
 siod_repl_read = read_f;
 siod_repl_eval = eval_f;
 siod_repl_print = print_f;}

void siod_gput_st(struct gen_printio *f,char *st)
{PUTS_FCN(st,f);}

void siod_fput_st(FILE *f,char *st)
{long flag;
 flag = siod_no_interrupt(1);
 fprintf(f,"%s",st);
 siod_no_interrupt(flag);}

int siod_fputs_fcn(char *st,void *cb)
{siod_fput_st((FILE *)cb,st);
 return(1);}

void siod_put_st(char *st)
{siod_fput_st(stdout,st);}
     
void siod_gsiod_repl_puts(char *st,void (*siod_repl_puts)(char *))
{if (siod_repl_puts == NULL)
   {siod_put_st(st);
    fflush(stdout);}
 else
   (*siod_repl_puts)(st);}
     
long siod_repl(struct siod_repl_hooks *h)
{siod_LISP x,cw = 0;
 double rt,ct;
 while(1)
   {if ((gc_kind_copying == 1) && ((siod_gc_status_flag) || heap >= heap_end))
     {rt = siod_myruntime();
      siod_gc_stop_and_copy();
      if (siod_verbose_level >= 2)
	{sprintf(tkbuffer,
		 "GC took %g seconds, %ld compressed to %ld, %ld free\n",
		 siod_myruntime()-rt,old_heap_used,(long)(heap-heap_org),(long)(heap_end-heap));
	 siod_gsiod_repl_puts(tkbuffer,h->siod_repl_puts);}}
    if (siod_verbose_level >= 2)
      siod_gsiod_repl_puts("> ",h->siod_repl_puts);
    if (h->siod_repl_read == NULL)
      x = siod_lread(NIL);
    else
      x = (*h->siod_repl_read)();
    if EQ(x,eof_val) break;
    rt = siod_myruntime();
    ct = siod_myrealtime();
    if (gc_kind_copying == 1)
      cw = heap;
    else
      {gc_cells_allocated = 0;
       gc_time_taken = 0.0;}
    if (h->siod_repl_eval == NULL)
      x = siod_leval(x,NIL);
    else
      x = (*h->siod_repl_eval)(x);
    if (gc_kind_copying == 1)
      sprintf(tkbuffer,
	      "Evaluation took %g seconds %ld siod_cons work, %g real.\n",
	      siod_myruntime()-rt,
	      (long)(heap-cw),
	      siod_myrealtime()-ct);
    else
      sprintf(tkbuffer,
	     "Evaluation took %g seconds (%g in gc) %ld siod_cons work, %g real.\n",
	      siod_myruntime()-rt,
	      gc_time_taken,
	      gc_cells_allocated,
	      siod_myrealtime()-ct);
    if (siod_verbose_level >= 2)
      siod_gsiod_repl_puts(tkbuffer,h->siod_repl_puts);
    if (h->siod_repl_print == NULL)
      {if (siod_verbose_level >= 2)
	 siod_lprint(x,NIL);}
    else
      (*h->siod_repl_print)(x);}
 return(0);}

void siod_set_fatal_exit_hook(void (*fcn)(void))
{fatal_exit_hook = fcn;}

static long inside_err = 0;

siod_LISP err(const char *message, siod_LISP x)
{struct catch_frame *l;
 long was_inside = inside_err;
 siod_LISP retval,nx;
 const char *msg,*eobj;
 fprintf(stderr, "in err(%s)\n", message ? message : "?");
 nointerrupt = 1;
 if ((!message) && CONSP(x) && TYPEP(CAR(x),tc_string))
   {msg = get_c_string(CAR(x));
    nx = CDR(x);
    retval = x;}
 else
   {msg = message;
    nx = x;
    retval = NIL;}
 if ((eobj = try_get_c_string(nx)) && !memchr(eobj,0,80 ))
   eobj = NULL;
 if ((siod_verbose_level >= 1) && msg)
   {if NULLP(nx)
      printf("ERROR: %s\n",msg);
    else if (eobj)
      printf("ERROR: %s (errobj %s)\n",msg,eobj);
    else
      printf("ERROR: %s (see errobj)\n",msg);}
 if (errjmp_ok == 1)
   {inside_err = 1;
    siod_setvar(sym_errobj,nx,NIL);
    for(l=catch_framep; l; l = (*l).next)
      if (EQ((*l).tag,sym_errobj) ||
	  EQ((*l).tag,sym_catchall))
	{if (!msg) msg = "siod_quit";
	 (*l).retval = (NNULLP(retval) ? retval :
			(was_inside) ? NIL :
			siod_cons(siod_strcons(strlen(msg),msg),nx));
	 nointerrupt = 0;
	 inside_err = 0;
	 longjmp((*l).cframe,2);}
    inside_err = 0;
    longjmp(errjmp,(msg) ? 1 : 2);}
 if (siod_verbose_level >= 1)
   printf("FATAL ERROR DURING STARTUP OR CRITICAL CODE SECTION\n");
 if (fatal_exit_hook)
   (*fatal_exit_hook)();
 else
   exit(10);
 return(NIL);}

siod_LISP siod_errswitch(void)
{return(err("BUG. Reached impossible case",NIL));}

void siod_err_stack(char *ptr)
     /* The user could be given an option to continue here */
{err("the currently assigned stack limit has been exceded",NIL);}

siod_LISP siod_stack_limit(siod_LISP amount,siod_LISP silent)
{if NNULLP(amount)
   {stack_size = siod_get_c_long(amount);
    siod_stack_limit_ptr = STACK_LIMIT(stack_start_ptr,stack_size);}
 if NULLP(silent)
   {sprintf(tkbuffer,"Stack_size = %ld bytes, [%p,%p]\n",
	    stack_size,stack_start_ptr,siod_stack_limit_ptr);
    siod_put_st(tkbuffer);
    return(NIL);}
 else
   return(siod_flocons(stack_size));}

char *try_get_c_string(siod_LISP x)
{if TYPEP(x,tc_symbol)
   return(PNAME(x));
 else if TYPEP(x,tc_string)
   return(x->storage_as.string.data);
 else
   return(NULL);}

char *get_c_string(siod_LISP x)
{if TYPEP(x,tc_symbol)
   return(PNAME(x));
 else if TYPEP(x,tc_string)
   return(x->storage_as.string.data);
 else
   err("not a symbol or string",x);
 return(NULL);}

char *get_c_siod_string_dim(siod_LISP x,long *len)
{switch(TYPE(x))
   {case tc_symbol:
      *len = strlen(PNAME(x));
      return(PNAME(x));
    case tc_string:
    case tc_byte_array:
      *len = x->storage_as.string.dim;
      return(x->storage_as.string.data);
    case tc_long_array:
      *len = x->storage_as.long_array.dim * sizeof(long);
      return((char *)x->storage_as.long_array.data);
    default:
      err("not a symbol or string",x);
      return(NULL);}}

siod_LISP siod_lerr(siod_LISP message, siod_LISP x)
{if (CONSP(message) && TYPEP(CAR(message),tc_string))
   err(NULL,message);
 else
   err(get_c_string(message),x);
 return(NIL);}

void siod_gc_fatal_error(void)
{err("ran out of storage",NIL);}

void debugging_newcell(siod_LISP *into_ptr, int type)
{if (gc_kind_copying == 1)
    {if ((*into_ptr = heap) >= heap_end)
	siod_gc_fatal_error();
      heap = (*into_ptr)+1;}
  else
    {if NULLP(freelist)
	       siod_gc_for_newcell();
      *into_ptr = freelist;
      freelist = CDR(freelist);
      ++gc_cells_allocated;}
  (**into_ptr).siod_gc_mark = 0;
  (**into_ptr).type = (short) type;}

siod_LISP siod_newcell(long type)
{siod_LISP z;
 NEWCELL(z,type);
 return(z);}

siod_LISP siod_cons(siod_LISP x,siod_LISP y)
{siod_LISP z;
 NEWCELL(z,tc_cons);
 CAR(z) = x;
 CDR(z) = y;
 return(z);}

siod_LISP siod_consp(siod_LISP x)
{if CONSP(x) return(sym_t); else return(NIL);}

siod_LISP siod_car(siod_LISP x)
{switch TYPE(x)
   {case tc_nil:
      return(NIL);
    case tc_cons:
      return(CAR(x));
    default:
      return(err("wta to car",x));}}

siod_LISP siod_cdr(siod_LISP x)
{switch TYPE(x)
   {case tc_nil:
      return(NIL);
    case tc_cons:
      return(CDR(x));
    default:
      return(err("wta to cdr",x));}}

siod_LISP siod_setcar(siod_LISP cell, siod_LISP value)
{if NCONSP(cell) err("wta to siod_setcar",cell);
 return(CAR(cell) = value);}

siod_LISP siod_setcdr(siod_LISP cell, siod_LISP value)
{if NCONSP(cell) err("wta to siod_setcdr",cell);
 return(CDR(cell) = value);}

siod_LISP siod_flocons(double x)
{siod_LISP z;
 long n;
 if ((inums_dim > 0) &&
     ((x - (n = (long)x)) == 0) &&
     (x >= 0) &&
     (n < inums_dim))
   return(inums[n]);
 NEWCELL(z,tc_flonum);
 FLONM(z) = x;
 return(z);}

siod_LISP siod_numberp(siod_LISP x)
{if FLONUMP(x) return(sym_t); else return(NIL);}

siod_LISP siod_plus(siod_LISP x,siod_LISP y)
{if NULLP(y)
   return(NULLP(x) ? siod_flocons(0) : x);
 if NFLONUMP(x) err("wta(1st) to siod_plus",x);
 if NFLONUMP(y) err("wta(2nd) to siod_plus",y);
 return(siod_flocons(FLONM(x) + FLONM(y)));}

siod_LISP siod_ltimes(siod_LISP x,siod_LISP y)
{if NULLP(y)
   return(NULLP(x) ? siod_flocons(1) : x);
 if NFLONUMP(x) err("wta(1st) to times",x);
 if NFLONUMP(y) err("wta(2nd) to times",y);
 return(siod_flocons(FLONM(x)*FLONM(y)));}

siod_LISP siod_difference(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to siod_difference",x);
 if NULLP(y)
   return(siod_flocons(-FLONM(x)));
 else
   {if NFLONUMP(y) err("wta(2nd) to siod_difference",y);
    return(siod_flocons(FLONM(x) - FLONM(y)));}}

siod_LISP siod_Quotient(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to quotient",x);
 if NULLP(y)
   return(siod_flocons(1/FLONM(x)));
 else
   {if NFLONUMP(y) err("wta(2nd) to quotient",y);
    return(siod_flocons(FLONM(x)/FLONM(y)));}}

siod_LISP siod_IntQuotient(siod_LISP x, siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to quotient",x);
 if NULLP(y)
   return(siod_flocons(1/floor(FLONM(x))));
 else
   {if NFLONUMP(y) err("wta(2nd) to intquotient",y);
    return(siod_flocons((int)floor(FLONM(x))/(int)floor(FLONM(y))));}}

siod_LISP siod_Remainder(siod_LISP x, siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to remainder",x);
 if NULLP(y)
   return(siod_flocons(1/(int)floor(FLONM(x))));
 else
   {if NFLONUMP(y) err("wta(2nd) to remainder",y);
    return(siod_flocons((int)floor(FLONM(x))%(int)floor(FLONM(y))));}}

siod_LISP siod_lllabs(siod_LISP x)
{double v;
 if NFLONUMP(x) err("wta to abs",x);
 v = FLONM(x);
 if (v < 0)
   return(siod_flocons(-v));
 else
   return(x);}

siod_LISP siod_lsqrt(siod_LISP x)
{if NFLONUMP(x) err("wta to sqrt",x);
 return(siod_flocons(sqrt(FLONM(x))));}

siod_LISP siod_greaterp(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to siod_greaterp",x);
 if NFLONUMP(y) err("wta(2nd) to siod_greaterp",y);
 if (FLONM(x)>FLONM(y)) return(sym_t);
 return(NIL);}

siod_LISP siod_lessp(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to siod_lessp",x);
 if NFLONUMP(y) err("wta(2nd) to siod_lessp",y);
 if (FLONM(x)<FLONM(y)) return(sym_t);
 return(NIL);}

siod_LISP siod_greaterEp(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to siod_greaterp",x);
 if NFLONUMP(y) err("wta(2nd) to siod_greaterp",y);
 if (FLONM(x)>=FLONM(y)) return(sym_t);
 return(NIL);}

siod_LISP siod_lessEp(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to siod_lessp",x);
 if NFLONUMP(y) err("wta(2nd) to siod_lessp",y);
 if (FLONM(x)<=FLONM(y)) return(sym_t);
 return(NIL);}

siod_LISP siod_lmax(siod_LISP x,siod_LISP y)
{if NULLP(y) return(x);
 if NFLONUMP(x) err("wta(1st) to max",x);
 if NFLONUMP(y) err("wta(2nd) to max",y);
 return((FLONM(x) > FLONM(y)) ? x : y);}

siod_LISP siod_lmin(siod_LISP x,siod_LISP y)
{if NULLP(y) return(x);
 if NFLONUMP(x) err("wta(1st) to min",x);
 if NFLONUMP(y) err("wta(2nd) to min",y);
 return((FLONM(x) < FLONM(y)) ? x : y);}

siod_LISP siod_eq(siod_LISP x,siod_LISP y)
{if EQ(x,y) return(sym_t); else return(NIL);}

siod_LISP siod_eql(siod_LISP x,siod_LISP y)
{if EQ(x,y) return(sym_t); else 
 if NFLONUMP(x) return(NIL); else
 if NFLONUMP(y) return(NIL); else
 if (FLONM(x) == FLONM(y)) return(sym_t);
 return(NIL);}

siod_LISP siod_symcons(char *pname,siod_LISP vcell)
{siod_LISP z;
 NEWCELL(z,tc_symbol);
 PNAME(z) = pname;
 VCELL(z) = vcell;
 return(z);}

siod_LISP siod_ClosureP(siod_LISP x)
{if TYPEP(x, tc_closure) return(sym_t); else return(NIL);}

siod_LISP siod_symbolp(siod_LISP x)
{if SYMBOLP(x) return(sym_t); else return(NIL);}

siod_LISP siod_err_ubv(siod_LISP v)
{return(err("unbound variable",v));}

siod_LISP siod_symbol_boundp(siod_LISP x,siod_LISP env)
{siod_LISP tmp;
 if NSYMBOLP(x) err("not a symbol",x);
 tmp = siod_envlookup(x,env);
 if NNULLP(tmp) return(sym_t);
 if EQ(VCELL(x),unbound_marker) return(NIL); else return(sym_t);}

siod_LISP siod_symbol_value(siod_LISP x,siod_LISP env)
{siod_LISP tmp;
 if NSYMBOLP(x) err("not a symbol",x);
 tmp = siod_envlookup(x,env);
 if NNULLP(tmp) return(CAR(tmp));
 tmp = VCELL(x);
 if EQ(tmp,unbound_marker) siod_err_ubv(x);
 return(tmp);}



char *must_malloc(unsigned long size)
{char *tmp;
 tmp = (char *) malloc((size) ? size : 1);
 if (tmp == (char *)NULL) err("failed to allocate storage from system",NIL);
 return(tmp);}

int siod_check_obarray_list(siod_LISP sl)
{
  siod_LISP l;
  int totlen = 0;		/* use strlen to force actual access to strings */
  for(l=sl;NNULLP(l);l=CDR(l)) {
#if 0
    fprintf(stderr, " %p", l);
    fprintf(stderr, "->%p?", CAR(l));
    fprintf(stderr, "->%p?\n", PNAME(CAR(l)));
#endif
    totlen += strlen(PNAME(CAR(l)));
  }
  return totlen;
}

int n_checks = 0;

void siod_check_obarray(const char *label)
{
  if (obarray_dim > 1) {
    int i;
    for (i = 0; i < obarray_dim; i++) {
      siod_check_obarray_list(obarray[i]);
    }
  } else {
      siod_check_obarray_list(oblistvar);
  }
  n_checks++;
}

siod_LISP siod_gen_siod_intern(char *name,long copyp)
{siod_LISP l,sym,sl;
 char *cname;
 long hash=0,n,c,flag;
 flag = siod_no_interrupt(1);
 if (obarray_dim > 1)
   {hash = 0;
    n = obarray_dim;
    cname = name;
    while((c = *cname++)) hash = ((hash * 17) ^ c) % n;
    sl = obarray[hash];}
 else
   sl = oblistvar;
 for(l=sl;NNULLP(l);l=CDR(l))
   {if (strcmp(name,PNAME(CAR(l))) == 0)
     {siod_no_interrupt(flag);
       return(CAR(l));}}
 if (copyp == 1)
   {cname = (char *) must_malloc(strlen(name)+1);
    strcpy(cname,name);}
 else
   cname = name;
 sym = siod_symcons(cname,unbound_marker);
 if (obarray_dim > 1) obarray[hash] = siod_cons(sym,sl);
 oblistvar = siod_cons(sym,oblistvar);
 siod_no_interrupt(flag);
 return(sym);}

siod_LISP siod_cintern(char *name)
{return(siod_gen_siod_intern(name,0));}

siod_LISP siod_rintern(char *name)
{return(siod_gen_siod_intern(name,1));}

siod_LISP siod_intern(siod_LISP name)
{return(siod_rintern(get_c_string(name)));}

siod_LISP siod_subrsiod_cons(long type, char *name, SUBR_FUNC f)
{siod_LISP z;
 NEWCELL(z,type);
 (*z).storage_as.subr.name = name;
 (*z).storage_as.subr0.f = f;
 return(z);}

siod_LISP siod_closure(siod_LISP env,siod_LISP code)
{siod_LISP z;
 NEWCELL(z,tc_closure);
 (*z).storage_as.closure.env = env;
 (*z).storage_as.closure.code = code;
 return(z);}

void siod_gc_protect(siod_LISP *location)
{siod_gc_protect_n(location,1);}

void siod_gc_protect_n(siod_LISP *location,long n)
{struct siod_gc_protected *reg;
 reg = (struct siod_gc_protected *) must_malloc(sizeof(struct siod_gc_protected));
 (*reg).location = location;
 (*reg).length = n;
 (*reg).next = protected_registers;
  protected_registers = reg;}

void siod_gc_protect_sym(siod_LISP *location,char *st)
{*location = siod_cintern(st);
 siod_gc_protect(location);}

void siod_scan_registers(void)
{struct siod_gc_protected *reg;
 siod_LISP *location;
 long j,n;
 for(reg = protected_registers; reg; reg = (*reg).next)
   {location = (*reg).location;
    n = (*reg).length;
    for(j=0;j<n;++j)
      location[j] = siod_gc_relocate(location[j]);}}

void __stdcall siod_init_storage(void)
{long j;
 siod_LISP stack_start;
 if (stack_start_ptr == NULL)
   stack_start_ptr = &stack_start;
 siod_init_storage_1();
 siod_init_storage_a();
 siod_set_gc_hooks(tc_c_file,0,0,0,siod_file_gc_free,&j);
 siod_set_print_hooks(tc_c_file,siod_file_prin1);}

void siod_init_storage_1(void)
{siod_LISP ptr;
 long j;
 tkbuffer = (char *) must_malloc(TKBUFFERN+1);
 if (((gc_kind_copying == 1) && (nheaps != 2)) || (nheaps < 1))
   err("invalid number of heaps",NIL);
 heaps = (siod_LISP *) must_malloc(sizeof(siod_LISP) * nheaps);
 for(j=0;j<nheaps;++j) heaps[j] = NULL;
 heaps[0] = (siod_LISP) must_malloc(sizeof(struct obj)*heap_size);
 heap = heaps[0];
 heap_org = heap;
 heap_end = heap + heap_size;
 if (gc_kind_copying == 1)
   heaps[1] = (siod_LISP) must_malloc(sizeof(struct obj)*heap_size);
 else
   freelist = NIL;
 siod_gc_protect(&oblistvar);
 if (obarray_dim > 1)
   {obarray = (siod_LISP *) must_malloc(sizeof(siod_LISP) * obarray_dim);
    for(j=0;j<obarray_dim;++j)
      obarray[j] = NIL;
    siod_gc_protect_n(obarray,obarray_dim);}
 unbound_marker = siod_cons(siod_cintern("**unbound-marker**"),NIL);
 siod_gc_protect(&unbound_marker);
 eof_val = siod_cons(siod_cintern("eof"),NIL);
 siod_gc_protect(&eof_val);
 siod_gc_protect_sym(&sym_t,"t");
 siod_setvar(sym_t,sym_t,NIL);
 siod_setvar(siod_cintern("nil"),NIL,NIL);
 siod_setvar(siod_cintern("let"),siod_cintern("let-siod_internal-macro"),NIL);
 siod_setvar(siod_cintern("let*"),siod_cintern("let*-macro"),NIL);
 siod_setvar(siod_cintern("letrec"),siod_cintern("letrec-macro"),NIL);
 siod_gc_protect_sym(&sym_errobj,"errobj");
 siod_setvar(sym_errobj,NIL,NIL);
 siod_gc_protect_sym(&sym_catchall,"all");
 siod_gc_protect_sym(&sym_progn,"begin");
 siod_gc_protect_sym(&sym_lambda,"lambda");
 siod_gc_protect_sym(&sym_quote,"quote");
 siod_gc_protect_sym(&sym_dot,".");
 siod_gc_protect_sym(&sym_after_gc,"*after-gc*");
 siod_setvar(sym_after_gc,NIL,NIL);
 siod_gc_protect_sym(&sym_eval_history_ptr,"*eval-history-ptr*");
 siod_setvar(sym_eval_history_ptr,NIL,NIL);
 if (inums_dim > 0)
   {inums = (siod_LISP *) must_malloc(sizeof(siod_LISP) * inums_dim);
    for(j=0;j<inums_dim;++j)
      {NEWCELL(ptr,tc_flonum);
       FLONM(ptr) = j;
       inums[j] = ptr;}
    siod_gc_protect_n(inums,inums_dim);}}
 
void siod_init_subr(char *name, long type, SUBR_FUNC fcn)
{siod_setvar(siod_cintern(name),siod_subrsiod_cons(type,name,fcn),NIL);}

void siod_init_subr_0(char *name, siod_LISP (*fcn)(void))
{siod_init_subr(name,tc_subr_0,(SUBR_FUNC)fcn);}

void siod_init_subr_1(char *name, siod_LISP (*fcn)(siod_LISP))
{siod_init_subr(name,tc_subr_1,(SUBR_FUNC)fcn);}

void siod_init_subr_2(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP))
{siod_init_subr(name,tc_subr_2,(SUBR_FUNC)fcn);}

void siod_init_subr_2n(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP))
{siod_init_subr(name,tc_subr_2n,(SUBR_FUNC)fcn);}

void siod_init_subr_3(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP,siod_LISP))
{siod_init_subr(name,tc_subr_3,(SUBR_FUNC)fcn);}

void siod_init_subr_4(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP,siod_LISP,siod_LISP))
{siod_init_subr(name,tc_subr_4,(SUBR_FUNC)fcn);}

void siod_init_subr_5(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP,siod_LISP,siod_LISP,siod_LISP))
{siod_init_subr(name,tc_subr_5,(SUBR_FUNC)fcn);}

void siod_init_lsubr(char *name, siod_LISP (*fcn)(siod_LISP))
{siod_init_subr(name,tc_lsubr,(SUBR_FUNC)fcn);}

void siod_init_fsubr(char *name, siod_LISP (*fcn)(siod_LISP,siod_LISP))
{siod_init_subr(name,tc_fsubr,(SUBR_FUNC)fcn);}

void siod_init_msubr(char *name, siod_LISP (*fcn)(siod_LISP *,siod_LISP *))
{siod_init_subr(name,tc_msubr,(SUBR_FUNC)fcn);}

siod_LISP siod_assq(siod_LISP x,siod_LISP alist)
{siod_LISP l,tmp;
 for(l=alist;CONSP(l);l=CDR(l))
   {tmp = CAR(l);
    if (CONSP(tmp) && EQ(CAR(tmp),x)) return(tmp);
    INTERRUPT_CHECK();}
 if EQ(l,NIL) return(NIL);
 return(err("improper list to siod_assq",alist));}


struct user_type_hooks *get_user_type_hooks(long type)
{long n;
 if (user_types == NULL)
   {n = sizeof(struct user_type_hooks) * tc_table_dim;
    user_types = (struct user_type_hooks *) must_malloc(n);
    memset(user_types,0,n);}
 if ((type >= 0) && (type < tc_table_dim))
   return(&user_types[type]);
 else
   err("type number out of range",NIL);
 return(NULL);}

long siod_allocate_user_tc(void)
{long x = user_tc_next;
 if (x > tc_user_max)
   err("ran out of user type codes",NIL);
 ++user_tc_next;
 return(x);}
 
void siod_set_gc_hooks(long type,
		  siod_LISP (*rel)(siod_LISP),
		  siod_LISP (*mark)(siod_LISP),
		  void (*scan)(siod_LISP),
		  void (*free)(siod_LISP),
		  long *kind)
{struct user_type_hooks *p;
 p = get_user_type_hooks(type);
 p->siod_gc_relocate = rel;
 p->gc_scan = scan;
 p->siod_gc_mark = mark;
 p->gc_free = free;
 *kind = gc_kind_copying;}

siod_LISP siod_gc_relocate(siod_LISP x)
{siod_LISP nw;
 struct user_type_hooks *p;
 if EQ(x,NIL) return(NIL);
 if ((*x).siod_gc_mark == 1) return(CAR(x));
 switch TYPE(x)
   {case tc_flonum:
    case tc_cons:
    case tc_symbol:
    case tc_closure:
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_2n:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      if ((nw = heap) >= heap_end) siod_gc_fatal_error();
      heap = nw+1;
      memcpy(nw,x,sizeof(struct obj));
      break;
    default:
      p = get_user_type_hooks(TYPE(x));
      if (p->siod_gc_relocate)
	nw = (*p->siod_gc_relocate)(x);
      else
	{if ((nw = heap) >= heap_end) siod_gc_fatal_error();
	 heap = nw+1;
	 memcpy(nw,x,sizeof(struct obj));}}
 (*x).siod_gc_mark = 1;
 CAR(x) = nw;
 return(nw);}

siod_LISP siod_get_newspace(void)
{siod_LISP newspace;
 if (heap_org == heaps[0])
   newspace = heaps[1];
 else
   newspace = heaps[0];
 heap = newspace;
 heap_org = heap;
 heap_end = heap + heap_size;
 return(newspace);}

void siod_scan_newspace(siod_LISP newspace)
{siod_LISP ptr;
 struct user_type_hooks *p;
 for(ptr=newspace; ptr < heap; ++ptr)
   {switch TYPE(ptr)
      {case tc_cons:
       case tc_closure:
	 CAR(ptr) = siod_gc_relocate(CAR(ptr));
	 CDR(ptr) = siod_gc_relocate(CDR(ptr));
	 break;
       case tc_symbol:
	 VCELL(ptr) = siod_gc_relocate(VCELL(ptr));
	 break;
       case tc_flonum:
       case tc_subr_0:
       case tc_subr_1:
       case tc_subr_2:
       case tc_subr_2n:
       case tc_subr_3:
       case tc_subr_4:
       case tc_subr_5:
       case tc_lsubr:
       case tc_fsubr:
       case tc_msubr:
	 break;
       default:
	 p = get_user_type_hooks(TYPE(ptr));
	 if (p->gc_scan) (*p->gc_scan)(ptr);}}}

void siod_free_oldspace(siod_LISP space,siod_LISP end)
{siod_LISP ptr;
 struct user_type_hooks *p;
 for(ptr=space; ptr < end; ++ptr)
   if (ptr->siod_gc_mark == 0)
     switch TYPE(ptr)
       {case tc_cons:
	case tc_closure:
	case tc_symbol:
	case tc_flonum:
	case tc_subr_0:
	case tc_subr_1:
	case tc_subr_2:
	case tc_subr_2n:
	case tc_subr_3:
	case tc_subr_4:
	case tc_subr_5:
	case tc_lsubr:
	case tc_fsubr:
	case tc_msubr:
	  break;
	default:
	  p = get_user_type_hooks(TYPE(ptr));
	  if (p->gc_free) (*p->gc_free)(ptr);}}
      
void siod_gc_stop_and_copy(void)
{siod_LISP newspace,oldspace,end;
 long flag;
 flag = siod_no_interrupt(1);
 errjmp_ok = 0;
 oldspace = heap_org;
 end = heap;
 old_heap_used = end - oldspace;
 newspace = siod_get_newspace();
 siod_scan_registers();
 siod_scan_newspace(newspace);
 siod_free_oldspace(oldspace,end);
 errjmp_ok = 1;
 siod_no_interrupt(flag);}

siod_LISP siod_allocate_aheap(void)
{long j,flag;
 siod_LISP ptr,end,next;
 siod_gc_kind_check();
 for(j=0;j<nheaps;++j)
   if (!heaps[j])
     {flag = siod_no_interrupt(1);
      if (siod_gc_messages_flag)
	printf("[allocating heap %ld]\n",j);
      heaps[j] = (siod_LISP) must_malloc(sizeof(struct obj)*heap_size);
      ptr = heaps[j];
      end = heaps[j] + heap_size;
      while(1)
	{(*ptr).type = tc_free_cell;
	 next = ptr + 1;
	 if (next < end)
	   {CDR(ptr) = next;
	    ptr = next;}
	 else
	   {CDR(ptr) = freelist;
	    break;}}
      freelist = heaps[j];
      flag = siod_no_interrupt(flag);
      return(sym_t);}
 return(NIL);}

void siod_gc_for_newcell(void)
{long flag,n;
 siod_LISP l;
 if (heap < heap_end)
   {freelist = heap;
    CDR(freelist) = NIL;
    ++heap;
    return;}
 if (errjmp_ok == 0) siod_gc_fatal_error();
 flag = siod_no_interrupt(1);
 errjmp_ok = 0;
 siod_gc_mark_and_sweep();
 errjmp_ok = 1;
 siod_no_interrupt(flag);
 for(n=0,l=freelist;(n < 100) && NNULLP(l); ++n) l = CDR(l);
 if (n == 0)
   {if NULLP(siod_allocate_aheap())
      siod_gc_fatal_error();}
 else if ((n == 100) && NNULLP(sym_after_gc))
   siod_leval(siod_leval(sym_after_gc,NIL),NIL);
 else
   siod_allocate_aheap();}

int siod_gc_count = 0;

void siod_gc_mark_and_sweep(void)
{siod_LISP stack_end;
 siod_gc_count++;
 siod_gc_ms_stats_start();
 while(heap < heap_end)
   {heap->type = tc_free_cell;
    heap->siod_gc_mark = 0;
    ++heap;}
 setjmp(save_regs_siod_gc_mark);
 siod_mark_locations((siod_LISP *) save_regs_siod_gc_mark,
		(siod_LISP *) (((char *) save_regs_siod_gc_mark) + sizeof(save_regs_siod_gc_mark)));
 siod_mark_protected_registers();
 siod_mark_locations((siod_LISP *) stack_start_ptr,
		(siod_LISP *) &stack_end);
#ifdef THINK_C
 siod_mark_locations((siod_LISP *) ((char *) stack_start_ptr + 2),
		(siod_LISP *) ((char *) &stack_end + 2));
#endif
 siod_gc_sweep();
 siod_gc_ms_stats_end();}

void siod_gc_ms_stats_start(void)
{gc_rt = siod_myruntime();
 gc_cells_collected = 0;
 if (siod_gc_messages_flag)
   printf("[starting GC]\n");}

void siod_gc_ms_stats_end(void)
{gc_rt = siod_myruntime() - gc_rt;
 gc_time_taken = gc_time_taken + gc_rt;
 if (siod_gc_messages_flag)
   printf("[GC took %g cpu seconds, %ld cells collected]\n",
	  gc_rt,
	  gc_cells_collected);}

void siod_gc_mark(siod_LISP ptr)
{struct user_type_hooks *p;
 siod_gc_mark_loop:
 if NULLP(ptr) return;
 if ((*ptr).siod_gc_mark) return;
 (*ptr).siod_gc_mark = 1;
 switch ((*ptr).type)
   {case tc_flonum:
      break;
    case tc_cons:
      siod_gc_mark(CAR(ptr));
      ptr = CDR(ptr);
      goto siod_gc_mark_loop;
    case tc_symbol:
      ptr = VCELL(ptr);
      goto siod_gc_mark_loop;
    case tc_closure:
      siod_gc_mark((*ptr).storage_as.closure.code);
      ptr = (*ptr).storage_as.closure.env;
      goto siod_gc_mark_loop;
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_2n:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      break;
    default:
      p = get_user_type_hooks(TYPE(ptr));
      if (p->siod_gc_mark)
	ptr = (*p->siod_gc_mark)(ptr);}}

void siod_mark_protected_registers(void)
{struct siod_gc_protected *reg;
 siod_LISP *location;
 long j,n;
 for(reg = protected_registers; reg; reg = (*reg).next)
   {location = (*reg).location;
    n = (*reg).length;
    for(j=0;j<n;++j)
      siod_gc_mark(location[j]);}}

void siod_mark_locations(siod_LISP *start,siod_LISP *end)
{siod_LISP *tmp;
 long n;
 if (start > end)
   {tmp = start;
    start = end;
    end = tmp;}
 n = end - start;
 siod_mark_locations_array(start,n);}

long siod_looks_pointerp(siod_LISP p)
{long j;
 siod_LISP h;
 for(j=0;j<nheaps;++j)
   if ((h = heaps[j]) &&
       (p >= h) &&
       (p < (h + heap_size)) &&
       (((((char *)p) - ((char *)h)) % sizeof(struct obj)) == 0) &&
       NTYPEP(p,tc_free_cell))
     return(1);
 return(0);}

void siod_mark_locations_array(siod_LISP *x,long n)
{int j;
 siod_LISP p;
 for(j=0;j<n;++j)
   {p = x[j];
    if (siod_looks_pointerp(p))
      siod_gc_mark(p);}}

void siod_gc_sweep(void)
{siod_LISP ptr,end,nfreelist,org;
 long n,k;
 struct user_type_hooks *p;
 end = heap_end;
 n = 0;
 nfreelist = NIL;
 for(k=0;k<nheaps;++k)
   if (heaps[k])
     {org = heaps[k];
      end = org + heap_size;
      for(ptr=org; ptr < end; ++ptr)
	if (((*ptr).siod_gc_mark == 0))
	  {switch((*ptr).type)
	     {case tc_free_cell:
	      case tc_cons:
	      case tc_closure:
	      case tc_symbol:
	      case tc_flonum:
	      case tc_subr_0:
	      case tc_subr_1:
	      case tc_subr_2:
	      case tc_subr_2n:
	      case tc_subr_3:
	      case tc_subr_4:
	      case tc_subr_5:
	      case tc_lsubr:
	      case tc_fsubr:
	      case tc_msubr:
		break;
	      default:
		p = get_user_type_hooks(TYPE(ptr));
		if (p->gc_free)
		  (*p->gc_free)(ptr);}
	   ++n;
	   (*ptr).type = tc_free_cell;
	   CDR(ptr) = nfreelist;
	   nfreelist = ptr;}
	else
	  (*ptr).siod_gc_mark = 0;}
 gc_cells_collected = n;
 freelist = nfreelist;}

void siod_gc_kind_check(void)
{if (gc_kind_copying == 1)
   err("cannot perform operation with stop-and-copy GC mode. Use -g0\n",
       NIL);}

siod_LISP siod_user_gc(siod_LISP args)
{long old_status_flag,flag;
 siod_gc_kind_check();
 flag = siod_no_interrupt(1);
 errjmp_ok = 0;
 old_status_flag = siod_gc_status_flag;
 if NNULLP(args) {
     if NULLP(siod_car(args)) siod_gc_messages_flag = siod_gc_status_flag = 0; else siod_gc_messages_flag = siod_gc_status_flag = 1;}
 siod_gc_mark_and_sweep();
 siod_gc_status_flag = old_status_flag;
 errjmp_ok = 1;
 siod_no_interrupt(flag);
 return(NIL);}

long siod_nactive_heaps(void)
{long m;
 for(m=0;(m < nheaps) && heaps[m];++m);
 return(m);}

long siod_freelist_length(void)
{long n;
 siod_LISP l;
 for(n=0,l=freelist;NNULLP(l); ++n) l = CDR(l);
 n += (heap_end - heap);
 return(n);}

siod_LISP siod_gc_messages(siod_LISP args)
{
  if NNULLP(args) {
      if NULLP(siod_car(args)) {
	  siod_gc_messages_flag = 0;
	} else {
	siod_gc_messages_flag = 1;
      }
    }
  return siod_gc_messages_flag ? sym_t : NIL;
}
 
siod_LISP siod_gc_status(siod_LISP args)
{long n,m;
  if NNULLP(args) {
      if NULLP(siod_car(args)) siod_gc_status_flag = 0; else siod_gc_status_flag = 1;}
 if (gc_kind_copying == 1)
   {if (siod_gc_status_flag)
      siod_put_st("garbage collection is on\n");
   else
     siod_put_st("garbage collection is off\n");
    sprintf(tkbuffer,"%ld allocated %ld free\n",
	    (long)(heap - heap_org), (long)(heap_end - heap));
    siod_put_st(tkbuffer);}
 else
   {if (siod_gc_status_flag)
      siod_put_st("garbage collection verbose\n");
    else
      siod_put_st("garbage collection silent\n");
    {m = siod_nactive_heaps();
     n = siod_freelist_length();
     sprintf(tkbuffer,"%ld/%ld heaps, %ld allocated %ld free\n",
	     m,nheaps,m*heap_size - n,n);
     siod_put_st(tkbuffer);}}
 return(NIL);}

siod_LISP siod_gc_info(siod_LISP arg)
{switch(siod_get_c_long(arg))
   {case 0:
      return((gc_kind_copying == 1) ? sym_t : NIL);
    case 1:
      return(siod_flocons(siod_nactive_heaps()));
    case 2:
      return(siod_flocons(nheaps));
    case 3:
      return(siod_flocons(heap_size));
    case 4:
      return(siod_flocons((gc_kind_copying == 1)
		     ? (long) (heap_end - heap)
		     : siod_freelist_length()));
    case 5:
      return(siod_flocons(siod_gc_count));
    default:
      return(NIL);}}

siod_LISP siod_leval_args(siod_LISP l,siod_LISP env)
{siod_LISP result,v1,v2,tmp;
 if NULLP(l) return(NIL);
 if NCONSP(l) err("bad syntax argument list",l);
 result = siod_cons(siod_leval(CAR(l),env),NIL);
 for(v1=result,v2=CDR(l);
     CONSP(v2);
     v1 = tmp, v2 = CDR(v2))
  {tmp = siod_cons(siod_leval(CAR(v2),env),NIL);
   CDR(v1) = tmp;}
 if NNULLP(v2) err("bad syntax argument list",l);
 return(result);}

siod_LISP siod_extend_env(siod_LISP actuals,siod_LISP formals,siod_LISP env)
{if SYMBOLP(formals)
   return(siod_cons(siod_cons(siod_cons(formals,NIL),siod_cons(actuals,NIL)),env));
 return(siod_cons(siod_cons(formals,actuals),env));}

#define ENVLOOKUP_TRICK 1

siod_LISP siod_envlookup(siod_LISP var,siod_LISP env)
{siod_LISP frame,al,fl,tmp;
 for(frame=env;CONSP(frame);frame=CDR(frame))
   {tmp = CAR(frame);
    if NCONSP(tmp) err("damaged frame",tmp);
    for(fl=CAR(tmp),al=CDR(tmp);CONSP(fl);fl=CDR(fl),al=CDR(al))
      {if NCONSP(al) err("too few arguments",tmp);
       if EQ(CAR(fl),var) return(al);}
    /* suggested by a user. It works for reference (although siod_conses)
       but doesn't allow for set! to work properly... */
#if (ENVLOOKUP_TRICK)
    if (SYMBOLP(fl) && EQ(fl, var)) return(siod_cons(al, NIL));
#endif
  }
 if NNULLP(frame) {
     err("damaged env",env);}
 return(NIL);}

void siod_set_eval_hooks(long type,siod_LISP (*fcn)(siod_LISP, siod_LISP *,siod_LISP *))
{struct user_type_hooks *p;
 p = get_user_type_hooks(type);
 p->siod_leval = fcn;}

siod_LISP siod_err_siod_closure_code(siod_LISP tmp)
{return(err("siod_closure code type not valid",tmp));}

siod_LISP siod_leval(siod_LISP x,siod_LISP env)
{siod_LISP tmp,arg1;
 struct user_type_hooks *p;
 if (siod_verbose_level >= 6) {
   printf("eval: "); siod_lprint(x,NIL);
 }
 STACK_CHECK(&x);
 loop:
 INTERRUPT_CHECK();
 tmp = VCELL(sym_eval_history_ptr);
 if TYPEP(tmp,tc_cons)
   {CAR(tmp) = x;
    VCELL(sym_eval_history_ptr) = CDR(tmp);}
 switch TYPE(x)
   {case tc_symbol:
      tmp = siod_envlookup(x,env);
      if NNULLP(tmp) return(CAR(tmp));
      tmp = VCELL(x);
      if EQ(tmp,unbound_marker) siod_err_ubv(x);
      return(tmp);
    case tc_cons:
      tmp = CAR(x);
      switch TYPE(tmp)
	{case tc_symbol:
	   tmp = siod_envlookup(tmp,env);
	   if NNULLP(tmp)
	     {tmp = CAR(tmp);
	      break;}
	   tmp = VCELL(CAR(x));
	   if EQ(tmp,unbound_marker) siod_err_ubv(CAR(x));
	   break;
	 case tc_cons:
	   tmp = siod_leval(tmp,env);
	   break;}
      switch TYPE(tmp)
	{case tc_subr_0:
	   return(SUBR0(tmp)());
	 case tc_subr_1:
	   return(SUBR1(tmp)(siod_leval(siod_car(CDR(x)),env)));
	 case tc_subr_2:
	   x = CDR(x);
	   arg1 = siod_leval(siod_car(x),env);
	   x = NULLP(x) ? NIL : CDR(x);
	   return(SUBR2(tmp)(arg1,
			     siod_leval(siod_car(x),env)));
	 case tc_subr_2n:
	   x = CDR(x);
	   arg1 = siod_leval(siod_car(x),env);
	   x = NULLP(x) ? NIL : CDR(x);
	   arg1 = SUBR2(tmp)(arg1,
			     siod_leval(siod_car(x),env));
	   for(x=siod_cdr(x);CONSP(x);x=CDR(x))
	     arg1 = SUBR2(tmp)(arg1,siod_leval(CAR(x),env));
	   return(arg1);
	 case tc_subr_3:
	   x = CDR(x);
	   arg1 = siod_leval(siod_car(x),env);
	   x = NULLP(x) ? NIL : CDR(x);
	   return(SUBR3(tmp)(arg1,
			     siod_leval(siod_car(x),env),
			     siod_leval(siod_car(siod_cdr(x)),env)));

	 case tc_subr_4:
	   x = CDR(x);
	   arg1 = siod_leval(siod_car(x),env);
	   x = NULLP(x) ? NIL : CDR(x);
	   return(SUBR4(tmp)(arg1,
			     siod_leval(siod_car(x),env),
			     siod_leval(siod_car(siod_cdr(x)),env),
			     siod_leval(siod_car(siod_cdr(siod_cdr(x))),env)));

	 case tc_subr_5:
	   x = CDR(x);
	   arg1 = siod_leval(siod_car(x),env);
	   x = NULLP(x) ? NIL : CDR(x);
	   return(SUBR5(tmp)(arg1,
			     siod_leval(siod_car(x),env),
			     siod_leval(siod_car(siod_cdr(x)),env),
			     siod_leval(siod_car(siod_cdr(siod_cdr(x))),env),
			     siod_leval(siod_car(siod_cdr(siod_cdr(siod_cdr(x)))),env)));

	 case tc_lsubr:
	   return(SUBR1(tmp)(siod_leval_args(CDR(x),env)));
	 case tc_fsubr:
	   return(SUBR2(tmp)(CDR(x),env));
	 case tc_msubr:
	   if NULLP(SUBRM(tmp)(&x,&env)) return(x);
	   goto loop;
	 case tc_closure:
	   switch TYPE((*tmp).storage_as.closure.code)
	     {case tc_cons:
		env = siod_extend_env(siod_leval_args(CDR(x),env),
				 CAR((*tmp).storage_as.closure.code),
				 (*tmp).storage_as.closure.env);
		x = CDR((*tmp).storage_as.closure.code);
		goto loop;
	      case tc_subr_1:
		return(SUBR1(tmp->storage_as.closure.code)
		       (tmp->storage_as.closure.env));
	      case tc_subr_2:
		x = CDR(x);
		arg1 = siod_leval(siod_car(x),env);
		return(SUBR2(tmp->storage_as.closure.code)
		       (tmp->storage_as.closure.env,arg1));
	      case tc_subr_3:
		x = CDR(x);
		arg1 = siod_leval(siod_car(x),env);
		x = NULLP(x) ? NIL : CDR(x);
		return(SUBR3(tmp->storage_as.closure.code)
		       (tmp->storage_as.closure.env,
			arg1,
			siod_leval(siod_car(x),env)));
	      case tc_subr_4:
		x = CDR(x);
		arg1 = siod_leval(siod_car(x),env);
		x = NULLP(x) ? NIL : CDR(x);
		return(SUBR4(tmp->storage_as.closure.code)
		       (tmp->storage_as.closure.env,
			arg1,
			siod_leval(siod_car(x),env),
			siod_leval(siod_car(siod_cdr(x)),env)));
	      case tc_subr_5:
		x = CDR(x);
		arg1 = siod_leval(siod_car(x),env);
		x = NULLP(x) ? NIL : CDR(x);
		return(SUBR5(tmp->storage_as.closure.code)
		       (tmp->storage_as.closure.env,
			arg1,
			siod_leval(siod_car(x),env),
			siod_leval(siod_car(siod_cdr(x)),env),
			siod_leval(siod_car(siod_cdr(siod_cdr(x))),env)));

	      case tc_lsubr:
		return(SUBR1(tmp->storage_as.closure.code)
		       (siod_cons(tmp->storage_as.closure.env,
			     siod_leval_args(CDR(x),env))));
	      default:
		siod_err_siod_closure_code(tmp);}
	   break;
	 case tc_symbol:
	   x = siod_cons(tmp,siod_cons(siod_cons(sym_quote,siod_cons(x,NIL)),NIL));
	   x = siod_leval(x,NIL);
	   goto loop;
	 default:
	   p = get_user_type_hooks(TYPE(tmp));
	   if (p->siod_leval)
	     {if NULLP((*p->siod_leval)(tmp,&x,&env)) return(x); else goto loop;}
	   err("bad function",tmp);}
    default:
      return(x);}}

siod_LISP siod_lapply(siod_LISP fcn,siod_LISP args)
{struct user_type_hooks *p;
 siod_LISP acc;
 STACK_CHECK(&fcn);
 INTERRUPT_CHECK();
 switch TYPE(fcn)
   {case tc_subr_0:
      return(SUBR0(fcn)());
    case tc_subr_1:
      return(SUBR1(fcn)(siod_car(args)));
    case tc_subr_2:
      return(SUBR2(fcn)(siod_car(args),siod_car(siod_cdr(args))));
    case tc_subr_2n:
      acc = SUBR2(fcn)(siod_car(args),siod_car(siod_cdr(args)));
      for(args=siod_cdr(siod_cdr(args));CONSP(args);args=CDR(args))
	acc = SUBR2(fcn)(acc,CAR(args));
      return(acc);
    case tc_subr_3:
      return(SUBR3(fcn)(siod_car(args),siod_car(siod_cdr(args)),siod_car(siod_cdr(siod_cdr(args)))));
    case tc_subr_4:
      return(SUBR4(fcn)(siod_car(args),siod_car(siod_cdr(args)),siod_car(siod_cdr(siod_cdr(args))),
			siod_car(siod_cdr(siod_cdr(siod_cdr(args))))));
    case tc_subr_5:
      return(SUBR5(fcn)(siod_car(args),siod_car(siod_cdr(args)),siod_car(siod_cdr(siod_cdr(args))),
			siod_car(siod_cdr(siod_cdr(siod_cdr(args)))),
			siod_car(siod_cdr(siod_cdr(siod_cdr(siod_cdr(args)))))));
    case tc_lsubr:
      return(SUBR1(fcn)(args));
    case tc_fsubr:
    case tc_msubr:
    case tc_symbol:
      return(err("cannot be applied",fcn));
    case tc_closure:
      switch TYPE(fcn->storage_as.closure.code)
	{case tc_cons:
	   return(siod_leval(siod_cdr(fcn->storage_as.closure.code),
			siod_extend_env(args,
				   siod_car(fcn->storage_as.closure.code),
				   fcn->storage_as.closure.env)));
	 case tc_subr_1:
	   return(SUBR1(fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env));
	 case tc_subr_2:
	   return(SUBR2(fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   siod_car(args)));
	 case tc_subr_3:
	   return(SUBR3(fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   siod_car(args),siod_car(siod_cdr(args))));
	 case tc_subr_4:
	   return(SUBR4(fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   siod_car(args),siod_car(siod_cdr(args)),siod_car(siod_cdr(siod_cdr(args)))));
	 case tc_subr_5:
	   return(SUBR5(fcn->storage_as.closure.code)
		  (fcn->storage_as.closure.env,
		   siod_car(args),siod_car(siod_cdr(args)),siod_car(siod_cdr(siod_cdr(args))),
		   siod_car(siod_cdr(siod_cdr(siod_cdr(args))))));
	 case tc_lsubr:
	   return(SUBR1(fcn->storage_as.closure.code)
		  (siod_cons(fcn->storage_as.closure.env,args)));
	 default:
	   return(siod_err_siod_closure_code(fcn));}
    default:
      p = get_user_type_hooks(TYPE(fcn));
      if (p->siod_leval)
	return(err("have eval, dont know apply",fcn));
      else
	return(err("cannot be applied",fcn));}}

siod_LISP siod_setvar(siod_LISP var,siod_LISP val,siod_LISP env)
{siod_LISP tmp;
 if NSYMBOLP(var) err("wta(non-symbol) to siod_setvar",var);
 tmp = siod_envlookup(var,env);
 if NULLP(tmp) return(VCELL(var) = val);
 return(CAR(tmp)=val);}
 
siod_LISP siod_leval_set(siod_LISP args,siod_LISP env)
{return(siod_setvar(siod_leval(siod_car(args),env),siod_leval(siod_car(siod_cdr(args)),env),env));}
 
siod_LISP siod_leval_setq(siod_LISP args,siod_LISP env)
{return(siod_setvar(siod_car(args),siod_leval(siod_car(siod_cdr(args)),env),env));}

siod_LISP siod_syntax_define(siod_LISP args)
{if SYMBOLP(siod_car(args)) return(args);
 return(siod_syntax_define(
        siod_cons(siod_car(siod_car(args)),
	siod_cons(siod_cons(sym_lambda,
	     siod_cons(siod_cdr(siod_car(args)),
		  siod_cdr(args))),
	     NIL))));}
      
siod_LISP siod_leval_define(siod_LISP args,siod_LISP env)
{siod_LISP tmp,var,val;
 tmp = siod_syntax_define(args);
 var = siod_car(tmp);
 if NSYMBOLP(var) err("wta(non-symbol) to define",var);
 val = siod_leval(siod_car(siod_cdr(tmp)),env);
 tmp = siod_envlookup(var,env);
 if NNULLP(tmp) return(CAR(tmp) = val);
 if NULLP(env) return(VCELL(var) = val);
 tmp = siod_car(env);
 siod_setcar(tmp,siod_cons(var,siod_car(tmp)));
 siod_setcdr(tmp,siod_cons(val,siod_cdr(tmp)));
 return(val);}
 
siod_LISP siod_leval_if(siod_LISP *pform,siod_LISP *penv)
{siod_LISP args,env;
 args = siod_cdr(*pform);
 env = *penv;
 if NNULLP(siod_leval(siod_car(args),env)) 
    *pform = siod_car(siod_cdr(args)); else *pform = siod_car(siod_cdr(siod_cdr(args)));
 return(sym_t);}

siod_LISP siod_leval_lambda(siod_LISP args,siod_LISP env)
{siod_LISP body;
 if NULLP(siod_cdr(siod_cdr(args)))
   body = siod_car(siod_cdr(args));
  else body = siod_cons(sym_progn,siod_cdr(args));
 return(siod_closure(env,siod_cons(siod_arglchk(siod_car(args)),body)));}
                         
siod_LISP siod_leval_progn(siod_LISP *pform,siod_LISP *penv)
{siod_LISP env,l,next;
 env = *penv;
 l = siod_cdr(*pform);
 next = siod_cdr(l);
 while(NNULLP(next)) {siod_leval(siod_car(l),env);l=next;next=siod_cdr(next);}
 *pform = siod_car(l); 
 return(sym_t);}

siod_LISP siod_leval_or(siod_LISP *pform,siod_LISP *penv)
{siod_LISP env,l,next,val;
 env = *penv;
 l = siod_cdr(*pform);
 next = siod_cdr(l);
 while(NNULLP(next))
   {val = siod_leval(siod_car(l),env);
    if NNULLP(val) {*pform = val; return(NIL);}
    l=next;next=siod_cdr(next);}
 *pform = siod_car(l); 
 return(sym_t);}

siod_LISP siod_leval_and(siod_LISP *pform,siod_LISP *penv)
{siod_LISP env,l,next;
 env = *penv;
 l = siod_cdr(*pform);
 if NULLP(l) {*pform = sym_t; return(NIL);}
 next = siod_cdr(l);
 while(NNULLP(next))
   {if NULLP(siod_leval(siod_car(l),env)) {*pform = NIL; return(NIL);}
    l=next;next=siod_cdr(next);}
 *pform = siod_car(l); 
 return(sym_t);}

siod_LISP siod_leval_catch_1(siod_LISP forms,siod_LISP env)
{siod_LISP l,val = NIL;
 for(l=forms; NNULLP(l); l = siod_cdr(l))
   val = siod_leval(siod_car(l),env);
 catch_framep = catch_framep->next;
 return(val);}

siod_LISP siod_leval_catch(siod_LISP args,siod_LISP env)
{struct catch_frame frame;
 int k;
 frame.tag = siod_leval(siod_car(args),env);
 frame.next = catch_framep;
 k = setjmp(frame.cframe);
 catch_framep = &frame;
 if (k == 2)
   {catch_framep = frame.next;
    return(frame.retval);}
 return(siod_leval_catch_1(siod_cdr(args),env));}

siod_LISP siod_lthrow(siod_LISP tag,siod_LISP value)
{struct catch_frame *l;
 for(l=catch_framep; l; l = (*l).next)
   if (EQ((*l).tag,tag) ||
       EQ((*l).tag,sym_catchall))
     {(*l).retval = value;
      longjmp((*l).cframe,2);}
 err("no *catch found with this tag",tag);
 return(NIL);}

siod_LISP siod_leval_let(siod_LISP *pform,siod_LISP *penv)
{siod_LISP env,l;
 l = siod_cdr(*pform);
 env = *penv;
 *penv = siod_extend_env(siod_leval_args(siod_car(siod_cdr(l)),env),siod_car(l),env);
 *pform = siod_car(siod_cdr(siod_cdr(l)));
 return(sym_t);}

siod_LISP siod_letstar_macro(siod_LISP form)
{siod_LISP bindings = siod_cadr(form);
 if (NNULLP(bindings) && NNULLP(siod_cdr(bindings)))
   siod_setcdr(form,siod_cons(siod_cons(siod_car(bindings),NIL),
		    siod_cons(siod_cons(siod_cintern("let*"),
			      siod_cons(siod_cdr(bindings),
				   siod_cddr(form))),
			 NIL)));
 siod_setcar(form,siod_cintern("let"));
 return(form);}

siod_LISP siod_letrec_macro(siod_LISP form)
{siod_LISP letb,setb,l;
 for(letb=NIL,setb=siod_cddr(form),l=siod_cadr(form);NNULLP(l);l=siod_cdr(l))
   {letb = siod_cons(siod_cons(siod_caar(l),NIL),letb);
    setb = siod_cons(siod_listn(3,siod_cintern("set!"),siod_caar(l),siod_cadar(l)),setb);}
 siod_setcdr(form,siod_cons(letb,setb));
 siod_setcar(form,siod_cintern("let"));
 return(form);}

siod_LISP siod_reverse(siod_LISP l)
{siod_LISP n,p;
 n = NIL;
 for(p=l;NNULLP(p);p=siod_cdr(p)) n = siod_cons(siod_car(p),n);
 return(n);}

siod_LISP siod_let_macro(siod_LISP form)
{siod_LISP p,fl,al,tmp;
 fl = NIL;
 al = NIL;
 for(p=siod_car(siod_cdr(form));NNULLP(p);p=siod_cdr(p))
  {tmp = siod_car(p);
   if SYMBOLP(tmp) {fl = siod_cons(tmp,fl); al = siod_cons(NIL,al);}
   else {fl = siod_cons(siod_car(tmp),fl); al = siod_cons(siod_car(siod_cdr(tmp)),al);}}
 p = siod_cdr(siod_cdr(form));
 if NULLP(siod_cdr(p)) p = siod_car(p); else p = siod_cons(sym_progn,p);
 siod_setcdr(form,siod_cons(siod_reverse(fl),siod_cons(siod_reverse(al),siod_cons(p,NIL))));
 siod_setcar(form,siod_cintern("let-siod_internal"));
 return(form);}
   
siod_LISP siod_leval_quote(siod_LISP args,siod_LISP env)
{return(siod_car(args));}

siod_LISP siod_leval_tenv(siod_LISP args,siod_LISP env)
{return(env);}

siod_LISP siod_leval_while(siod_LISP args,siod_LISP env)
{siod_LISP l;
 while NNULLP(siod_leval(siod_car(args),env))
   for(l=siod_cdr(args);NNULLP(l);l=siod_cdr(l))
     siod_leval(siod_car(l),env);
 return(NIL);}

siod_LISP siod_symbolconc(siod_LISP args)
{long size;
 siod_LISP l,s;
 size = 0;
 tkbuffer[0] = 0;
 for(l=args;NNULLP(l);l=siod_cdr(l))
   {s = siod_car(l);
    if NSYMBOLP(s) err("wta(non-symbol) to siod_symbolconc",s);
    size = size + strlen(PNAME(s));
    if (size >  TKBUFFERN) err("siod_symbolconc buffer overflow",NIL);
    strcat(tkbuffer,PNAME(s));}
 return(siod_rintern(tkbuffer));}

void siod_set_print_hooks(long type,void (*fcn)(siod_LISP,struct gen_printio *))
{struct user_type_hooks *p;
 p = get_user_type_hooks(type);
 p->prin1 = fcn;}

char *subr_kind_str(long n)
{switch(n)
   {case tc_subr_0: return("subr_0");
    case tc_subr_1: return("subr_1");
    case tc_subr_2: return("subr_2");
    case tc_subr_2n: return("subr_2n");
    case tc_subr_3: return("subr_3");
    case tc_subr_4: return("subr_4");
    case tc_subr_5: return("subr_5");
    case tc_lsubr: return("lsubr");
    case tc_fsubr: return("fsubr");
    case tc_msubr: return("msubr");
    default: return("???");}}

siod_LISP siod_lprin1g(siod_LISP exp,struct gen_printio *f)
{siod_LISP tmp;
 long n;
 struct user_type_hooks *p;
 STACK_CHECK(&exp);
 INTERRUPT_CHECK();
 switch TYPE(exp)
   {case tc_nil:
      siod_gput_st(f,"()");
      break;
   case tc_cons:
      siod_gput_st(f,"(");
      siod_lprin1g(siod_car(exp),f);
      for(tmp=siod_cdr(exp);CONSP(tmp);tmp=siod_cdr(tmp))
	{siod_gput_st(f," ");siod_lprin1g(siod_car(tmp),f);}
      if NNULLP(tmp) {siod_gput_st(f," . ");siod_lprin1g(tmp,f);}
      siod_gput_st(f,")");
      break;
    case tc_flonum:
      n = (long) FLONM(exp);
      if (((double) n) == FLONM(exp))
	sprintf(tkbuffer,"%ld",n);
      else
	sprintf(tkbuffer,"%g",FLONM(exp));
      siod_gput_st(f,tkbuffer);
      break;
    case tc_symbol:
      siod_gput_st(f,PNAME(exp));
      break;
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_2n:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      sprintf(tkbuffer,"#<%s ",subr_kind_str(TYPE(exp)));
      siod_gput_st(f,tkbuffer);
      siod_gput_st(f,(*exp).storage_as.subr.name);
      siod_gput_st(f,">");
      break;
    case tc_closure:
      siod_gput_st(f,"#<CLOSURE ");
      if CONSP((*exp).storage_as.closure.code)
	{siod_lprin1g(siod_car((*exp).storage_as.closure.code),f);
	 siod_gput_st(f," ");
	 siod_lprin1g(siod_cdr((*exp).storage_as.closure.code),f);}
      else
	siod_lprin1g((*exp).storage_as.closure.code,f);
      siod_gput_st(f,">");
      break;
    default:
      p = get_user_type_hooks(TYPE(exp));
      if (p->prin1)
	(*p->prin1)(exp,f);
      else
	{sprintf(tkbuffer,"#<UNKNOWN %d %p>",TYPE(exp),exp);
	 siod_gput_st(f,tkbuffer);}}
 return(NIL);}

siod_LISP siod_lprint(siod_LISP exp,siod_LISP lf)
{FILE *f = siod_get_c_file(lf,stdout);
 siod_lprin1f(exp,f);
 siod_fput_st(f,"\n");
 return(NIL);}

siod_LISP siod_lprin1(siod_LISP exp,siod_LISP lf)
{FILE *f = siod_get_c_file(lf,stdout);
 siod_lprin1f(exp,f);
 return(NIL);}

siod_LISP siod_lprin1f(siod_LISP exp,FILE *f)
{struct gen_printio s;
 s.putc_fcn = NULL;
 s.puts_fcn = siod_fputs_fcn;
 s.cb_argument = f;
 siod_lprin1g(exp,&s);
 return(NIL);}

siod_LISP siod_lread(siod_LISP f)
{return(siod_lreadf(siod_get_c_file(f,stdin)));}

int siod_f_getc(FILE *f)
{long iflag,dflag;
 int c;
 iflag = siod_no_interrupt(1);
 dflag = interrupt_differed;
 c = getc(f);
#ifdef VMS
 if ((dflag == 0) & interrupt_differed & (f == stdin))
   while((c != 0) & (c != EOF)) c = getc(f);
#endif
 siod_no_interrupt(iflag);
 return(c);}

void siod_f_ungetc(int c, FILE *f)
{ungetc(c,f);}

int siod_flush_ws(struct gen_readio *f,char *eosiod_ferr)
{int c,commentp;
 commentp = 0;
 while(1)
   {c = GETC_FCN(f);
     if (c == EOF) { if (eosiod_ferr) err(eosiod_ferr,NIL); else return(c);}
    if (commentp) {if (c == '\n') commentp = 0;}
    else if (c == ';') commentp = 1;
    else if (!isspace(c)) return(c);}}

siod_LISP siod_lreadf(FILE *f)
{struct gen_readio s;
 s.getc_fcn = (int (*)(void *)) siod_f_getc;
 s.ungetc_fcn = (void (*)(int,void *)) siod_f_ungetc;
 s.cb_argument = (char *) f;
 return(siod_readtl(&s));}

siod_LISP siod_readtl(struct gen_readio *f)
{int c;
 c = siod_flush_ws(f,(char *)NULL);
 if (c == EOF) return(eof_val);
 UNGETC_FCN(c,f);
 return(siod_lreadr(f));}

void siod_set_read_hooks(char *all_set,char *end_set,
		    siod_LISP (*fcn1)(int, struct gen_readio *),
		    siod_LISP (*fcn2)(char *,long, int *))
{user_ch_readm = all_set;
 user_te_readm = end_set;
 user_readm = fcn1;
 user_readt = fcn2;}

siod_LISP siod_lreadr(struct gen_readio *f)
{int c,j;
 char *p,*buffer=tkbuffer;
 STACK_CHECK(&f);
 p = buffer;
 c = siod_flush_ws(f,"end of file inside read");
 switch (c)
   {case '(':
      return(siod_lreadparen(f));
    case ')':
      err("unexpected close paren",NIL);
    case '\'':
      return(siod_cons(sym_quote,siod_cons(siod_lreadr(f),NIL)));
    case '`':
      return(siod_cons(siod_cintern("+siod_internal-backquote"),siod_lreadr(f)));
    case ',':
      c = GETC_FCN(f);
      switch(c)
	{case '@':
	   p = "+siod_internal-comma-atsign";
	   break;
	 case '.':
	   p = "+siod_internal-comma-dot";
	   break;
	 default:
	   p = "+siod_internal-comma";
	   UNGETC_FCN(c,f);}
      return(siod_cons(siod_cintern(p),siod_lreadr(f)));
    case '"':
      return(siod_lreadstring(f));
    case '#':
      return(siod_lreadsharp(f));
    default:
      if ((user_readm != NULL) && strchr(user_ch_readm,c))
	return((*user_readm)(c,f));}
 *p++ = c;
 for(j = 1; j<TKBUFFERN; ++j)
   {c = GETC_FCN(f);
    if (c == EOF) return(siod_lreadtk(buffer,j));
    if (isspace(c)) return(siod_lreadtk(buffer,j));
    if (strchr("()'`,;\"",c) || strchr(user_te_readm,c))
      {UNGETC_FCN(c,f);return(siod_lreadtk(buffer,j));}
    *p++ = c;}
 return(err("token larger than TKBUFFERN",NIL));}

siod_LISP siod_lreadparen(struct gen_readio *f)
{int c;
 siod_LISP tmp;
 c = siod_flush_ws(f,"end of file inside list");
 if (c == ')') return(NIL);
 UNGETC_FCN(c,f);
 tmp = siod_lreadr(f);
 if EQ(tmp,sym_dot)
   {tmp = siod_lreadr(f);
    c = siod_flush_ws(f,"end of file inside list");
    if (c != ')') err("missing close paren",NIL);
    return(tmp);}
 return(siod_cons(tmp,siod_lreadparen(f)));}

siod_LISP siod_lreadtk(char *buffer,long j)
{int flag;
 siod_LISP tmp;
 int adigit;
 char *p = buffer;
 p[j] = 0;
 if (user_readt != NULL)
   {tmp = (*user_readt)(p,j,&flag);
    if (flag) return(tmp);}
 if (*p == '-') p+=1;
 adigit = 0;
 while(isdigit(*p)) {p+=1; adigit=1;}
 if (*p=='.')
   {p += 1;
    while(isdigit(*p)) {p+=1; adigit=1;}}
 if (!adigit) goto a_symbol;
 if (*p=='e')
   {p+=1;
    if (*p=='-'||*p=='+') p+=1;
    if (!isdigit(*p)) goto a_symbol; else p+=1;
    while(isdigit(*p)) p+=1;}
 if (*p) goto a_symbol;
 return(siod_flocons(atof(buffer)));
 a_symbol:
 return(siod_rintern(buffer));}
      
siod_LISP siod_copy_list(siod_LISP x)
{if NULLP(x) return(NIL);
 STACK_CHECK(&x);
 return(siod_cons(siod_car(x),siod_copy_list(siod_cdr(x))));}

siod_LISP siod_apropos(siod_LISP matchl)
{siod_LISP result = NIL,l,ml;
 char *pname;
 for(l=oblistvar;CONSP(l);l=CDR(l))
   {pname = get_c_string(CAR(l));
    ml=matchl;
    while(CONSP(ml) && strstr(pname,get_c_string(CAR(ml))))
      ml=CDR(ml);
    if NULLP(ml)
      result = siod_cons(CAR(l),result);}
 return(result);}

siod_LISP siod_fopen_cg(FILE *(*fcn)(const char *,const char *),char *name,char *how)
{siod_LISP sym;
 long flag;
 char errmsg[256];
 flag = siod_no_interrupt(1);
 sym = siod_newcell(tc_c_file);
 sym->storage_as.c_file.f = (FILE *)NULL;
 sym->storage_as.c_file.name = (char *)NULL;
 if (!(sym->storage_as.c_file.f = (*fcn)(name,how)))
   {SAFE_STRCPY(errmsg,"could not open ");
    SAFE_STRCAT(errmsg,name);
    err(errmsg,siod_llast_c_errmsg(-1));}
 sym->storage_as.c_file.name = (char *) must_malloc(strlen(name)+1);
 strcpy(sym->storage_as.c_file.name,name);
 siod_no_interrupt(flag);
 return(sym);}

siod_LISP siod_fopen_c(char *name,char *how)
{return(siod_fopen_cg(fopen,name,how));}

siod_LISP siod_fopen_l(siod_LISP name,siod_LISP how)
{return(siod_fopen_c(get_c_string(name),NULLP(how) ? "r" : get_c_string(how)));}

siod_LISP siod_delq(siod_LISP elem,siod_LISP l)
{if NULLP(l) return(l);
 STACK_CHECK(&elem);
 if EQ(elem,siod_car(l)) return(siod_delq(elem,siod_cdr(l)));
 siod_setcdr(l,siod_delq(elem,siod_cdr(l)));
 return(l);}

siod_LISP siod_fclose_l(siod_LISP p)
{long flag;
 flag = siod_no_interrupt(1);
 if NTYPEP(p,tc_c_file) err("not a file",p);
 siod_file_gc_free(p);
 siod_no_interrupt(flag);
 return(NIL);}

siod_LISP siod_vload(char *ofname,long cflag,long rflag)
{siod_LISP form,result,tail,lf,reader = NIL;
 FILE *f;
 int c;
 long j,len;
 char buffer[512],*key = "parser:",*start,*end,*ftype=".scm",*fname;
 if ((start = strchr(ofname,VLOAD_OFFSET_HACK_CHAR)))
  {len = atol(ofname);
   fname = &start[1];}
 else
  {len = 0;
   fname = ofname;}
 if (rflag)
   {int iflag;
    iflag = siod_no_interrupt(1);
    if ((f = fopen(fname,"r")))
      fclose(f);
    else if ((fname[0] != '/') &&
	     ((strlen(siod_lib) + strlen(fname) + 1)
	      < sizeof(buffer)))
      {strcpy(buffer,siod_lib);
#ifdef unix
       strcat(buffer,"/");
#endif
       strcat(buffer,fname);
       if ((f = fopen(buffer,"r")))
	 {fname = buffer;
	  fclose(f);}}
    siod_no_interrupt(iflag);}
 if (siod_verbose_level >= 3)
   {siod_put_st("siod_loading ");
    siod_put_st(fname);
    siod_put_st("\n");}
 lf = siod_fopen_c(fname,(len) ? "rb" : "r");
 f = lf->storage_as.c_file.f;
 result = NIL;
 tail = NIL;
 for(j=0;j<len;++j) getc(f);
 j = 0;
 buffer[0] = 0;
 c = getc(f);
 while((c == '#') || (c == ';'))
   {while(((c = getc(f)) != EOF) && (c != '\n'))
      if ((j+1)<sizeof(buffer))
	{buffer[j] = c;
	 buffer[++j] = 0;}
    if (c == '\n')
      c = getc(f);}
 if (c != EOF)
   ungetc(c,f);
 if ((start = strstr(buffer,key)))
   {for(end = &start[strlen(key)];
	*end && isalnum(*end);
	++end);
    j = end - start;
    memmove(buffer,start,j);
    buffer[strlen(key)-1] = '_';
    buffer[j] = 0;
    strcat(buffer,ftype);
    siod_require(siod_strcons(-1,buffer));
    buffer[j] = 0;
    reader = siod_rintern(buffer);
    reader = siod_funcall1(siod_leval(reader,NIL),reader);
    if (siod_verbose_level >= 5)
      {siod_put_st("parser:");
       siod_lprin1(reader,NIL);
       siod_put_st("\n");}}
 while(1)
   {form = NULLP(reader) ? siod_lread(lf) : siod_funcall1(reader,lf);
     if EQ(form,eof_val) break;
     if (siod_verbose_level >= 5)
       siod_lprint(form,NIL);
     if (cflag)
       {form = siod_cons(form,NIL);
	 if NULLP(result)
		   result = tail = form;
	 else
	   tail = siod_setcdr(tail,form);}
     else {
       siod_leval(form,NIL);
     }
   }
 siod_fclose_l(lf);
 if (siod_verbose_level >= 3)
   siod_put_st("done.\n");
 return(result);}

siod_LISP siod_load(siod_LISP fname,siod_LISP cflag,siod_LISP rflag)
{return(siod_vload(get_c_string(fname),NULLP(cflag) ? 0 : 1,NULLP(rflag) ? 0 : 1));}

siod_LISP siod_require(siod_LISP fname)
{siod_LISP sym;
 sym = siod_intern(siod_string_append(siod_cons(siod_cintern("*"),
				 siod_cons(fname,
				      siod_cons(siod_cintern("-siod_loaded*"),NIL)))));
 if (NULLP(siod_symbol_boundp(sym,NIL)) ||
     NULLP(siod_symbol_value(sym,NIL)))
   {siod_load(fname,NIL,sym_t);
    siod_setvar(sym,sym_t,NIL);}
 return(sym);}

siod_LISP siod_save_forms(siod_LISP fname,siod_LISP forms,siod_LISP how)
{char *cname,*chow = NULL;
 siod_LISP l,lf;
 FILE *f;
 cname = get_c_string(fname);
 if EQ(how,NIL) chow = "w";
 else if EQ(how,siod_cintern("a")) chow = "a";
 else err("bad argument to save-forms",how);
 if (siod_verbose_level >= 3)
   {siod_put_st((*chow == 'a') ? "siod_appending" : "saving");
    siod_put_st(" forms to ");
    siod_put_st(cname);
    siod_put_st("\n");}
 lf = siod_fopen_c(cname,chow);
 f = lf->storage_as.c_file.f;
 for(l=forms;NNULLP(l);l=siod_cdr(l))
   {siod_lprin1f(siod_car(l),f);
    putc('\n',f);}
 siod_fclose_l(lf);
 if (siod_verbose_level >= 3)
   siod_put_st("done.\n");
 return(sym_t);}

siod_LISP siod_quit(void)
{return(err(NULL,NIL));}

siod_LISP siod_nullp(siod_LISP x)
{if EQ(x,NIL) return(sym_t); else return(NIL);}

siod_LISP siod_arglchk(siod_LISP x)
{
#if (!ENVLOOKUP_TRICK)
 siod_LISP l;
 if SYMBOLP(x) return(x);
 for(l=x;CONSP(l);l=CDR(l));
 if NNULLP(l) err("improper formal argument list",x);
#endif
 return(x);}

void siod_file_gc_free(siod_LISP ptr)
{if (ptr->storage_as.c_file.f)
   {fclose(ptr->storage_as.c_file.f);
    ptr->storage_as.c_file.f = (FILE *) NULL;}
 if (ptr->storage_as.c_file.name)
   {free(ptr->storage_as.c_file.name);
    ptr->storage_as.c_file.name = NULL;}}
   
void siod_file_prin1(siod_LISP ptr,struct gen_printio *f)
{char *name;
 name = ptr->storage_as.c_file.name;
 siod_gput_st(f,"#<FILE ");
 sprintf(tkbuffer," %p",ptr->storage_as.c_file.f);
 siod_gput_st(f,tkbuffer);
 if (name)
   {siod_gput_st(f," ");
    siod_gput_st(f,name);}
 siod_gput_st(f,">");}

FILE *siod_get_c_file(siod_LISP p,FILE *deflt)
{if (NULLP(p) && deflt) return(deflt);
 if NTYPEP(p,tc_c_file) err("not a file",p);
 if (!p->storage_as.c_file.f) err("file is closed",p);
 return(p->storage_as.c_file.f);}

siod_LISP siod_lgetc(siod_LISP p)
{int i;
 i = siod_f_getc(siod_get_c_file(p,stdin));
 return((i == EOF) ? NIL : siod_flocons((double)i));}

siod_LISP siod_lungetc(siod_LISP ii,siod_LISP p)
{int i;
 if NNULLP(ii)
   {i = siod_get_c_long(ii);
    siod_f_ungetc(i,siod_get_c_file(p,stdin));}
 return(NIL);}

siod_LISP siod_lputc(siod_LISP c,siod_LISP p)
{long flag;
 int i;
 FILE *f;
 f = siod_get_c_file(p,stdout);
 if FLONUMP(c)
   i = (int)FLONM(c);
 else
   i = *get_c_string(c);
 flag = siod_no_interrupt(1);
 putc(i,f);
 siod_no_interrupt(flag);
 return(NIL);}
     
siod_LISP siod_lputs(siod_LISP str,siod_LISP p)
{siod_fput_st(siod_get_c_file(p,stdout),get_c_string(str));
 return(NIL);}

siod_LISP siod_lftell(siod_LISP file)
{return(siod_flocons((double)ftell(siod_get_c_file(file,NULL))));}

siod_LISP siod_lfseek(siod_LISP file,siod_LISP offset,siod_LISP direction)
{return((fseek(siod_get_c_file(file,NULL),siod_get_c_long(offset),siod_get_c_long(direction)))
	? NIL : sym_t);}

siod_LISP siod_parse_number(siod_LISP x)
{char *c;
 c = get_c_string(x);
 return(siod_flocons(atof(c)));}

void __stdcall siod_init_subrs(void)
{siod_init_subrs_1();
 siod_init_subrs_a();}

siod_LISP siod_closure_code(siod_LISP exp)
{return(exp->storage_as.closure.code);}

siod_LISP siod_closure_env(siod_LISP exp)
{return(exp->storage_as.closure.env);}

siod_LISP siod_lwhile(siod_LISP form,siod_LISP env)
{siod_LISP l;
 while(NNULLP(siod_leval(siod_car(form),env)))
   for(l=siod_cdr(form);NNULLP(l);l=siod_cdr(l))
     siod_leval(siod_car(l),env);
 return(NIL);}

siod_LISP siod_nreverse(siod_LISP x)
{siod_LISP newp,oldp,nextp;
 newp = NIL;
 for(oldp=x;CONSP(oldp);oldp=nextp)
  {nextp=CDR(oldp);
   CDR(oldp) = newp;
   newp = oldp;}
 return(newp);}

siod_LISP siod_verbose(siod_LISP arg)
{if NNULLP(arg)
   siod_verbose_level = siod_get_c_long(siod_car(arg));
 return(siod_flocons(siod_verbose_level));}

int __stdcall siod_verbose_check(int level)
{return((siod_verbose_level >= level) ? 1 : 0);}

siod_LISP siod_lruntime(void)
{return(siod_cons(siod_flocons(siod_myruntime()),
	     siod_cons(siod_flocons(gc_time_taken),NIL)));}

siod_LISP siod_lrealtime(void)
{return(siod_flocons(siod_myrealtime()));}

siod_LISP siod_caar(siod_LISP x)
{return(siod_car(siod_car(x)));}

siod_LISP siod_cadr(siod_LISP x)
{return(siod_car(siod_cdr(x)));}

siod_LISP siod_cdar(siod_LISP x)
{return(siod_cdr(siod_car(x)));}

siod_LISP siod_cddr(siod_LISP x)
{return(siod_cdr(siod_cdr(x)));}

siod_LISP siod_lrand(siod_LISP m)
{long res;
 res = rand();
 if NULLP(m)
   return(siod_flocons(res));
 else
   return(siod_flocons(res % siod_get_c_long(m)));}

siod_LISP siod_lsrand(siod_LISP s)
{srand(siod_get_c_long(s));
 return(NIL);}

siod_LISP siod_a_true_value(void)
{return(sym_t);}

siod_LISP siod_poparg(siod_LISP *ptr,siod_LISP defaultv)
{siod_LISP value;
 if NULLP(*ptr)
   return(defaultv);
 value = siod_car(*ptr);
 *ptr = siod_cdr(*ptr);
 return(value);}

char *last_c_errmsg(int num)
{int xerrno = (num < 0) ? errno : num;
 static char serrmsg[100];
 char *errmsg;
 errmsg = strerror(xerrno);
 if (!errmsg)
   {sprintf(serrmsg,"errno %d",xerrno);
    errmsg = serrmsg;}
 return(errmsg);}

siod_LISP siod_llast_c_errmsg(int num)
{int xerrno = (num < 0) ? errno : num;
 char *errmsg = strerror(xerrno);
 if (!errmsg) return(siod_flocons(xerrno));
 return(siod_cintern(errmsg));}

siod_LISP siod_lsiod_llast_c_errmsg(void)
{return(siod_llast_c_errmsg(-1));}

size_t siod_safe_strlen(const char *s,size_t size)
{char *end;
 if ((end = (char *)memchr(s,0,size)))
   return(end - s);
 else
   return(size);}

char *safe_strcpy(char *s1,size_t size1,const char *s2)
{size_t len2;
 if (size1 == 0) return(s1);
 len2 = strlen(s2);
 if (len2 < size1)
   {if (len2) memcpy(s1,s2,len2);
    s1[len2] = 0;}
 else
   {memcpy(s1,s2,size1);
    s1[size1-1] = 0;}
 return(s1);}

char *safe_strcat(char *s1,size_t size1,const char *s2)
{size_t len1;
 len1 = siod_safe_strlen(s1,size1);
 safe_strcpy(&s1[len1],size1 - len1,s2);
 return(s1);}

static siod_LISP siod_parser_read(siod_LISP ignore)
{return(siod_leval(siod_cintern("read"),NIL));}

static siod_LISP siod_os_classification(void)
{
#ifdef unix
  return(siod_cintern("unix"));
#endif
#ifdef WIN32
  return(siod_cintern("win32"));
#endif
#ifdef VMS
  return(siod_cintern("vms"));
#endif
  return(NIL);}

void siod_init_subrs_1(void)
{
 siod_init_subr_2("cons",siod_cons);
 siod_init_subr_1("car",siod_car);
 siod_init_subr_1("cdr",siod_cdr);
 siod_init_subr_2("set-car!",siod_setcar);
 siod_init_subr_2("set-cdr!",siod_setcdr);
 siod_init_subr_2n("+",siod_plus);
 siod_init_subr_2n("-",siod_difference);
 siod_init_subr_2n("*",siod_ltimes);
 siod_init_subr_2n("/",siod_Quotient);
 siod_init_subr_2n("//",siod_IntQuotient);
 siod_init_subr_2n("%",siod_Remainder);
 siod_init_subr_1("closure?",siod_ClosureP);
 siod_init_subr_2n("min",siod_lmin);
 siod_init_subr_2n("max",siod_lmax);
 siod_init_subr_1("abs",siod_lllabs);
 siod_init_subr_1("sqrt",siod_lsqrt);
 siod_init_subr_2(">",siod_greaterp);
 siod_init_subr_2("<",siod_lessp);
 siod_init_subr_2(">=",siod_greaterEp);
 siod_init_subr_2("<=",siod_lessEp);
 siod_init_subr_2("eq?",siod_eq);
 siod_init_subr_2("eqv?",siod_eql);
 siod_init_subr_2("=",siod_eql);
 siod_init_subr_2("assq",siod_assq);
 siod_init_subr_2("delq",siod_delq);
 siod_init_subr_1("read",siod_lread);
 siod_init_subr_1("parser_read",siod_parser_read);
 siod_setvar(siod_cintern("*parser_read.scm-loaded*"),sym_t,NIL);
 siod_init_subr_0("eof-val",siod_get_eof_val);
 siod_init_subr_2("print",siod_lprint);
 siod_init_subr_2("prin1",siod_lprin1);
 siod_init_subr_2("eval",siod_leval);
 siod_init_subr_2("apply",siod_lapply);
 siod_init_fsubr("define",siod_leval_define);
 siod_init_fsubr("lambda",siod_leval_lambda);
 siod_init_msubr("if",siod_leval_if);
 siod_init_fsubr("while",siod_leval_while);
 siod_init_msubr("begin",siod_leval_progn);
 siod_init_fsubr("set",siod_leval_set);
 siod_init_fsubr("set!",siod_leval_setq);
 siod_init_msubr("or",siod_leval_or);
 siod_init_msubr("and",siod_leval_and);
 siod_init_fsubr("*catch",siod_leval_catch);
 siod_init_subr_2("*throw",siod_lthrow);
 siod_init_fsubr("quote",siod_leval_quote);
 siod_init_lsubr("apropos",siod_apropos);
 siod_init_lsubr("verbose",siod_verbose);
 siod_init_subr_1("copy-list",siod_copy_list);
 siod_init_lsubr("gc-messages", siod_gc_messages);
 siod_init_lsubr("gc-status",siod_gc_status);
 siod_init_lsubr("gc",siod_user_gc);
 siod_init_subr_3("load",siod_load);
 siod_init_subr_1("require",siod_require);
 siod_init_subr_1("pair?",siod_consp);
 siod_init_subr_1("symbol?",siod_symbolp);
 siod_init_subr_1("number?",siod_numberp);
 siod_init_msubr("let-siod_internal",siod_leval_let);
 siod_init_subr_1("let-siod_internal-macro",siod_let_macro);
 siod_init_subr_1("let*-macro",siod_letstar_macro);
 siod_init_subr_1("letrec-macro",siod_letrec_macro);
 siod_init_subr_2("symbol-bound?",siod_symbol_boundp);
 siod_init_subr_2("symbol-value",siod_symbol_value);
 siod_init_subr_3("set-symbol-value!",siod_setvar);
 siod_init_fsubr("the-environment",siod_leval_tenv);
 siod_init_subr_2("error",siod_lerr);
 siod_init_subr_0("quit",siod_quit);
 siod_init_subr_1("not",siod_nullp);
 siod_init_subr_1("null?",siod_nullp);
 siod_init_subr_2("env-lookup",siod_envlookup);
 siod_init_subr_1("reverse",siod_reverse);
 siod_init_lsubr("symbolconc",siod_symbolconc);
 siod_init_subr_3("save-forms",siod_save_forms);
 siod_init_subr_2("fopen",siod_fopen_l);
 siod_init_subr_1("fclose",siod_fclose_l);
 siod_init_subr_1("getc",siod_lgetc);
 siod_init_subr_2("ungetc",siod_lungetc);
 siod_init_subr_2("putc",siod_lputc);
 siod_init_subr_2("puts",siod_lputs);
 siod_init_subr_1("ftell",siod_lftell);
 siod_init_subr_3("fseek",siod_lfseek);
 siod_init_subr_1("parse-number",siod_parse_number);
 siod_init_subr_2("%%stack-limit",siod_stack_limit);
 siod_init_subr_1("intern",siod_intern);
 siod_init_subr_2("%%closure",siod_closure);
 siod_init_subr_1("%%closure-code",siod_closure_code);
 siod_init_subr_1("%%closure-env",siod_closure_env);
 siod_init_fsubr("while",siod_lwhile);
 siod_init_subr_1("nreverse",siod_nreverse);
 siod_init_subr_0("allocate-heap",siod_allocate_aheap);
 siod_init_subr_1("gc-info",siod_gc_info);
 siod_init_subr_0("runtime",siod_lruntime);
 siod_init_subr_0("realtime",siod_lrealtime);
 siod_init_subr_1("caar",siod_caar);
 siod_init_subr_1("cadr",siod_cadr);
 siod_init_subr_1("cdar",siod_cdar);
 siod_init_subr_1("cddr",siod_cddr);
 siod_init_subr_1("rand",siod_lrand);
 siod_init_subr_1("srand",siod_lsrand);
 siod_init_subr_0("last-c-error",siod_lsiod_llast_c_errmsg);
 siod_init_subr_0("os-classification",siod_os_classification);
 siod_init_slib_version();}


/* siod_err0,pr,prp are convenient to call from the C-language debugger */

void siod_err0(void)
{err("0",NIL);}

void pr(siod_LISP p)
{if (siod_looks_pointerp(p))
   siod_lprint(p,NIL);
 else
   siod_put_st("invalid\n");}

void prp(siod_LISP *p)
{if (!p) return;
 pr(*p);}


