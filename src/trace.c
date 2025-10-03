/*    COPYRIGHT (c) 1992-1994, 2009 BY
 *    MITECH CORPORATION, ACTON, MASSACHUSETTS.
 *    See the source file SLIB.C for more information.

(trace procedure1 procedure2 ...)
(untrace procedure1 procedure2 ...)

Currently only user-defined procedures can be traced.
Fancy printing features such as indentation based on
recursion level will also have to wait for a future version.


 */

#include <stdio.h>
#include <setjmp.h>
#include "ulsiod.h"
#include "siodp.h"

static void siod_init_trace_version(void)
{siod_setvar(siod_cintern("*trace-version*"),
	siod_cintern("$Id: trace.c,v 1.3 1997/05/11 11:35:47 gjc Exp $"),
	NIL);}


static long tc_closure_traced = 0;

static siod_LISP sym_traced = NIL;
static siod_LISP sym_quote = NIL;
static siod_LISP sym_begin = NIL;

siod_LISP siod_ltrace_fcn_name(siod_LISP body);
siod_LISP siod_ltrace_1(siod_LISP fcn_name,siod_LISP env);
siod_LISP siod_ltrace(siod_LISP fcn_names,siod_LISP env);
siod_LISP siod_luntrace_1(siod_LISP fcn);
siod_LISP siod_luntrace(siod_LISP fcns);
static void siod_ct_gc_scan(siod_LISP ptr);
static siod_LISP siod_ct_siod_gc_mark(siod_LISP ptr);
void siod_ct_prin1(siod_LISP ptr,struct gen_printio *f);
siod_LISP siod_ct_eval(siod_LISP ct,siod_LISP *px,siod_LISP *penv);

siod_LISP siod_ltrace_fcn_name(siod_LISP body)
{siod_LISP tmp;
 if NCONSP(body) return(NIL);
 if NEQ(CAR(body),sym_begin) return(NIL);
 tmp = CDR(body);
 if NCONSP(tmp) return(NIL);
 tmp = CAR(tmp);
 if NCONSP(tmp) return(NIL);
 if NEQ(CAR(tmp),sym_quote) return(NIL);
 tmp = CDR(tmp);
 if NCONSP(tmp) return(NIL);
 return(CAR(tmp));}

siod_LISP siod_ltrace_1(siod_LISP fcn_name,siod_LISP env)
{siod_LISP fcn,code;
 fcn = siod_leval(fcn_name,env);
 if (TYPE(fcn) == tc_closure)
   {code = fcn->storage_as.closure.code;
    if NULLP(siod_ltrace_fcn_name(siod_cdr(code)))
      siod_setcdr(code,siod_cons(sym_begin,
		       siod_cons(siod_cons(sym_quote,siod_cons(fcn_name,NIL)),
			    siod_cons(siod_cdr(code),NIL))));
    fcn->type = (short) tc_closure_traced;}
 else if (TYPE(fcn) == tc_closure_traced)
   ;
 else
   err("not a siod_closure, cannot trace",fcn);
 return(NIL);}

siod_LISP siod_ltrace(siod_LISP fcn_names,siod_LISP env)
{siod_LISP l;
 for(l=fcn_names;NNULLP(l);l=siod_cdr(l))
   siod_ltrace_1(siod_car(l),env);
 return(NIL);}

siod_LISP siod_luntrace_1(siod_LISP fcn)
{if (TYPE(fcn) == tc_closure)
   ;
 else if (TYPE(fcn) == tc_closure_traced)
   fcn->type = tc_closure;
 else
   err("not a siod_closure, cannot untrace",fcn);
 return(NIL);}

siod_LISP siod_luntrace(siod_LISP fcns)
{siod_LISP l;
 for(l=fcns;NNULLP(l);l=siod_cdr(l))
   siod_luntrace_1(siod_car(l));
 return(NIL);}

static void siod_ct_gc_scan(siod_LISP ptr)
{CAR(ptr) = siod_gc_relocate(CAR(ptr));
 CDR(ptr) = siod_gc_relocate(CDR(ptr));}

static siod_LISP siod_ct_siod_gc_mark(siod_LISP ptr)
{siod_gc_mark(ptr->storage_as.closure.code);
 return(ptr->storage_as.closure.env);}

void siod_ct_prin1(siod_LISP ptr,struct gen_printio *f)
{siod_gput_st(f,"#<CLOSURE(TRACED) ");
 siod_lprin1g(siod_car(ptr->storage_as.closure.code),f);
 siod_gput_st(f," ");
 siod_lprin1g(siod_cdr(ptr->storage_as.closure.code),f);
 siod_gput_st(f,">");}

siod_LISP siod_ct_eval(siod_LISP ct,siod_LISP *px,siod_LISP *penv)
{siod_LISP fcn_name,args,env,result,l;
 fcn_name = siod_ltrace_fcn_name(siod_cdr(ct->storage_as.closure.code));
 args = siod_leval_args(CDR(*px),*penv);
 siod_fput_st(stdout,"->");
 siod_lprin1f(fcn_name,stdout);
 for(l=args;NNULLP(l);l=siod_cdr(l))
   {siod_fput_st(stdout," ");
    siod_lprin1f(siod_car(l),stdout);}
 siod_fput_st(stdout,"\n");
 env = siod_extend_env(args,
		  siod_car(ct->storage_as.closure.code),
		  ct->storage_as.closure.env);
 result = siod_leval(siod_cdr(ct->storage_as.closure.code),env);
 siod_fput_st(stdout,"<-");
 siod_lprin1f(fcn_name,stdout);
 siod_fput_st(stdout," ");
 siod_lprin1f(result,stdout);
 siod_fput_st(stdout,"\n");
 *px = result;
 return(NIL);}

void __stdcall siod_init_trace(void)
{long j;
 tc_closure_traced = siod_allocate_user_tc();
 siod_set_gc_hooks(tc_closure_traced,
	      NULL,
	      siod_ct_siod_gc_mark,
	      siod_ct_gc_scan,
	      NULL,
	      &j);
 siod_gc_protect_sym(&sym_traced,"*traced*");
 siod_setvar(sym_traced,NIL,NIL);
 siod_gc_protect_sym(&sym_begin,"begin");
 siod_gc_protect_sym(&sym_quote,"quote");
 siod_set_print_hooks(tc_closure_traced,siod_ct_prin1);
 siod_set_eval_hooks(tc_closure_traced,siod_ct_eval);
 siod_init_fsubr("trace",siod_ltrace);
 siod_init_lsubr("untrace",siod_luntrace);
 siod_init_trace_version();}
