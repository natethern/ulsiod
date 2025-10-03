/*  
 *                   COPYRIGHT (c) 1988-1996, 2009 BY                             *
 *        PARADIGM ASSOCIATES INCORPORATED, CAMBRIDGE, MASSACHUSETTS.       *
 *        See the source file SLIB.C for more information.                  *

Array-hacking code moved to another source file.

*/

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#include "ulsiod.h"
#include "siodp.h"

static void siod_init_sliba_version(void)
{siod_setvar(siod_cintern("*sliba-version*"),
	siod_cintern("$Id: sliba.c,v 1.1 2001/02/03 15:38:49 gjcarret Exp gjcarret $"),
	NIL);}

static siod_LISP sym_plists = NIL;
static siod_LISP bashnum = NIL;
static siod_LISP sym_e = NIL;
static siod_LISP sym_f = NIL;

void siod_init_storage_a1(long type)
{long j;
 struct user_type_hooks *p;
 siod_set_gc_hooks(type,
	      siod_array_siod_gc_relocate,
	      siod_array_siod_gc_mark,
	      siod_array_gc_scan,
	      siod_array_gc_free,
	      &j);
 siod_set_print_hooks(type,siod_array_prin1);
 p = get_user_type_hooks(type);
 p->siod_fast_print = siod_array_siod_fast_print;
 p->siod_fast_read = siod_array_siod_fast_read;
 p->siod_equal = siod_array_siod_equal;
 p->siod_c_siod_sxhash = siod_array_siod_sxhash;}

void siod_init_storage_a(void)
{siod_gc_protect(&bashnum);
 bashnum = siod_newcell(tc_flonum);
 siod_init_storage_a1(tc_string);
 siod_init_storage_a1(tc_double_array);
 siod_init_storage_a1(tc_long_array);
 siod_init_storage_a1(tc_lisp_array);
 siod_init_storage_a1(tc_byte_array);}

siod_LISP siod_array_siod_gc_relocate(siod_LISP ptr)
{siod_LISP nw;
 if ((nw = heap) >= heap_end) siod_gc_fatal_error();
 heap = nw+1;
 memcpy(nw,ptr,sizeof(struct obj));
 return(nw);}

void siod_array_gc_scan(siod_LISP ptr)
{long j;
 if TYPEP(ptr,tc_lisp_array)
   for(j=0;j < ptr->storage_as.lisp_array.dim; ++j)
     ptr->storage_as.lisp_array.data[j] =     
       siod_gc_relocate(ptr->storage_as.lisp_array.data[j]);}

siod_LISP siod_array_siod_gc_mark(siod_LISP ptr)
{long j;
 if TYPEP(ptr,tc_lisp_array)
   for(j=0;j < ptr->storage_as.lisp_array.dim; ++j)
     siod_gc_mark(ptr->storage_as.lisp_array.data[j]);
 return(NIL);}

void siod_array_gc_free(siod_LISP ptr)
{switch (ptr->type)
   {case tc_string:
    case tc_byte_array:
#if 1
      memset(ptr->storage_as.string.data,'*',ptr->storage_as.string.dim);
#endif
      free(ptr->storage_as.string.data);
      break;
    case tc_double_array:
      free(ptr->storage_as.double_array.data);
      break;
    case tc_long_array:
      free(ptr->storage_as.long_array.data);
      break;
    case tc_lisp_array:
      free(ptr->storage_as.lisp_array.data);
      break;}
#if 1
 ptr->storage_as.lisp_array.data = NULL;
#endif
 
   }

void siod_array_prin1(siod_LISP ptr,struct gen_printio *f)
{int j;
 switch (ptr->type)
   {case tc_string:
      siod_gput_st(f,"\"");
      if (strcspn(ptr->storage_as.string.data,"\"\\\n\r\t") ==
	  strlen(ptr->storage_as.string.data))
	siod_gput_st(f,ptr->storage_as.string.data);
      else
	{int n,c;
	 char cbuff[3];
	 n = strlen(ptr->storage_as.string.data);
	 for(j=0;j<n;++j)
	   switch(c = ptr->storage_as.string.data[j])
	     {case '\\':
	      case '"':
		cbuff[0] = '\\';
		cbuff[1] = c;
		cbuff[2] = 0;
		siod_gput_st(f,cbuff);
		break;
	      case '\n':
		siod_gput_st(f,"\\n");
		break;
	      case '\r':
		siod_gput_st(f,"\\r");
		break;
	      case '\t':
		siod_gput_st(f,"\\t");
		break;
	      default:
		cbuff[0] = c;
		cbuff[1] = 0;
		siod_gput_st(f,cbuff);
		break;}}
      siod_gput_st(f,"\"");
      break;
    case tc_double_array:
      siod_gput_st(f,"#(");
      for(j=0; j < ptr->storage_as.double_array.dim; ++j)
	{sprintf(tkbuffer,"%g",ptr->storage_as.double_array.data[j]);
	 siod_gput_st(f,tkbuffer);
	 if ((j + 1) < ptr->storage_as.double_array.dim)
	   siod_gput_st(f," ");}
      siod_gput_st(f,")");
      break;
    case tc_long_array:
      siod_gput_st(f,"#(");
      for(j=0; j < ptr->storage_as.long_array.dim; ++j)
	{sprintf(tkbuffer,"%ld",ptr->storage_as.long_array.data[j]);
	 siod_gput_st(f,tkbuffer);
	 if ((j + 1) < ptr->storage_as.long_array.dim)
	   siod_gput_st(f," ");}
      siod_gput_st(f,")");
    case tc_byte_array:
      sprintf(tkbuffer,"#%ld\"",ptr->storage_as.string.dim);
      siod_gput_st(f,tkbuffer);
      for(j=0; j < ptr->storage_as.string.dim; ++j)
	{sprintf(tkbuffer,"%02x",ptr->storage_as.string.data[j] & 0xFF);
	 siod_gput_st(f,tkbuffer);}
      siod_gput_st(f,"\"");
      break;
    case tc_lisp_array:
      siod_gput_st(f,"#(");
      for(j=0; j < ptr->storage_as.lisp_array.dim; ++j)
	{siod_lprin1g(ptr->storage_as.lisp_array.data[j],f);
	 if ((j + 1) < ptr->storage_as.lisp_array.dim)
	   siod_gput_st(f," ");}
      siod_gput_st(f,")");
      break;}}

siod_LISP siod_strcons(long length,const char *data)
{long flag;
 siod_LISP s;
 flag = siod_no_interrupt(1);
 s = siod_cons(NIL,NIL);
 s->type = tc_string;
 if (length == -1) length = strlen(data);
 s->storage_as.string.data = must_malloc(length+1);
 s->storage_as.string.dim = length;
 if (data)
   memcpy(s->storage_as.string.data,data,length);
 s->storage_as.string.data[length] = 0;
 siod_no_interrupt(flag);
 return(s);}

int siod_rfs_getc(unsigned char **p)
{int i;
 i = **p;
 if (!i) return(EOF);
 *p = *p + 1;
 return(i);}

void siod_rfs_ungetc(unsigned char c,unsigned char **p)
{*p = *p - 1;}

siod_LISP siod_read_from_string(siod_LISP x)
{char *p;
 struct gen_readio s;
 p = get_c_string(x);
 s.getc_fcn = (int (*)(void *))siod_rfs_getc;
 s.ungetc_fcn = (void (*)(int,void *))siod_rfs_ungetc;
 s.cb_argument = (char *) &p;
 return(siod_readtl(&s));}

int siod_ptsiod_s_puts(char *from,void *cb)
{siod_LISP into;
 size_t fromlen,intolen,intosize,fitsize;
 into = (siod_LISP) cb;
 fromlen = strlen(from);
 intolen = strlen(into->storage_as.string.data);
 intosize = into->storage_as.string.dim  - intolen;
 fitsize = (fromlen < intosize) ? fromlen : intosize;
 memcpy(&into->storage_as.string.data[intolen],from,fitsize);
 into->storage_as.string.data[intolen+fitsize] = 0;
 if (fitsize < fromlen)
   err("print to string overflow",NIL);
 return(1);}

siod_LISP siod_err_wta_str(siod_LISP exp)
{return(err("not a string",exp));}

siod_LISP siod_print_to_string(siod_LISP exp,siod_LISP str,siod_LISP nostart)
{struct gen_printio s;
 if NTYPEP(str,tc_string) siod_err_wta_str(str);
 s.putc_fcn = NULL;
 s.puts_fcn = siod_ptsiod_s_puts;
 s.cb_argument = str;
 if NULLP(nostart)
   str->storage_as.string.data[0] = 0;
 siod_lprin1g(exp,&s);
 return(str);}

siod_LISP siod_aref1(siod_LISP a,siod_LISP i)
{long k;
 if NFLONUMP(i) err("bad index to aref",i);
 k = (long) FLONM(i);
 if (k < 0) err("negative index to aref",i);
 switch TYPE(a)
   {case tc_string:
     if (k >= a->storage_as.string.dim) err("index too large",i);
     return(siod_flocons((double) a->storage_as.u_string.data[k]));
    case tc_byte_array:
      if (k >= a->storage_as.string.dim) err("index too large",i);
      return(siod_flocons((double) a->storage_as.string.data[k]));
    case tc_double_array:
      if (k >= a->storage_as.double_array.dim) err("index too large",i);
      return(siod_flocons(a->storage_as.double_array.data[k]));
    case tc_long_array:
      if (k >= a->storage_as.long_array.dim) err("index too large",i);
      return(siod_flocons(a->storage_as.long_array.data[k]));
    case tc_lisp_array:
      if (k >= a->storage_as.lisp_array.dim) err("index too large",i);
      return(a->storage_as.lisp_array.data[k]);
    default:
      return(err("invalid argument to aref",a));}}

void siod_err1_siod_aset1(siod_LISP i)
{err("index to aset too large",i);}

void siod_err2_siod_aset1(siod_LISP v)
{err("bad value to store in array",v);}

siod_LISP siod_aset1(siod_LISP a,siod_LISP i,siod_LISP v)
{long k;
 if NFLONUMP(i) err("bad index to aset",i);
 k = (long) FLONM(i);
 if (k < 0) err("negative index to aset",i);
 switch TYPE(a)
   {case tc_string:
    case tc_byte_array:
      if NFLONUMP(v) siod_err2_siod_aset1(v);
      if (k >= a->storage_as.string.dim) siod_err1_siod_aset1(i);
      a->storage_as.string.data[k] = (char) FLONM(v);
      return(v);
    case tc_double_array:
      if NFLONUMP(v) siod_err2_siod_aset1(v);
      if (k >= a->storage_as.double_array.dim) siod_err1_siod_aset1(i);
      a->storage_as.double_array.data[k] = FLONM(v);
      return(v);
    case tc_long_array:
      if NFLONUMP(v) siod_err2_siod_aset1(v);
      if (k >= a->storage_as.long_array.dim) siod_err1_siod_aset1(i);
      a->storage_as.long_array.data[k] = (long) FLONM(v);
      return(v);
    case tc_lisp_array:
      if (k >= a->storage_as.lisp_array.dim) siod_err1_siod_aset1(i);
      a->storage_as.lisp_array.data[k] = v;
      return(v);
    default:
      return(err("invalid argument to aset",a));}}

siod_LISP siod_ar_cons(long typecode,long n,long initp)
{siod_LISP a;
 long flag,j;
 flag = siod_no_interrupt(1);
 a = siod_cons(NIL,NIL);
 switch(typecode)
   {case tc_double_array:
      a->storage_as.double_array.dim = n;
      a->storage_as.double_array.data = (double *) must_malloc(n *
							       sizeof(double));
      if (initp)
	for(j=0;j<n;++j) a->storage_as.double_array.data[j] = 0.0;
      break;
    case tc_long_array:
      a->storage_as.long_array.dim = n;
      a->storage_as.long_array.data = (long *) must_malloc(n * sizeof(long));
      if (initp)
	for(j=0;j<n;++j) a->storage_as.long_array.data[j] = 0;
      break;
    case tc_string:
      a->storage_as.string.dim = n;
      a->storage_as.string.data = (char *) must_malloc(n+1);
      a->storage_as.string.data[n] = 0;
      if (initp)
	for(j=0;j<n;++j) a->storage_as.string.data[j] = ' ';
    case tc_byte_array:
      a->storage_as.string.dim = n;
      a->storage_as.string.data = (char *) must_malloc(n);
      if (initp)
	for(j=0;j<n;++j) a->storage_as.string.data[j] = 0;
      break;
    case tc_lisp_array:
      a->storage_as.lisp_array.dim = n;
      a->storage_as.lisp_array.data = (siod_LISP *) must_malloc(n * sizeof(siod_LISP));
      for(j=0;j<n;++j) a->storage_as.lisp_array.data[j] = NIL;
      break;
    default:
      siod_errswitch();}
 a->type = (short) typecode;
 siod_no_interrupt(flag);
 return(a);}

siod_LISP siod_mallocl(void *place,long size)
{long n,r;
 siod_LISP retval;
 n = size / sizeof(long);
 r = size % sizeof(long);
 if (r) ++n;
 retval = siod_ar_cons(tc_long_array,n,0);
 *(long **)place = retval->storage_as.long_array.data;
 return(retval);}

siod_LISP siod_cons_array(siod_LISP dim,siod_LISP kind)
{siod_LISP a;
 long flag,n,j;
 if (NFLONUMP(dim) || (FLONM(dim) < 0))
   return(err("bad dimension to siod_cons-array",dim));
 else
   n = (long) FLONM(dim);
 flag = siod_no_interrupt(1);
 a = siod_cons(NIL,NIL);
 if EQ(siod_cintern("double"),kind)
   {a->type = tc_double_array;
    a->storage_as.double_array.dim = n;
    a->storage_as.double_array.data = (double *) must_malloc(n *
							     sizeof(double));
    for(j=0;j<n;++j) a->storage_as.double_array.data[j] = 0.0;}
 else if EQ(siod_cintern("long"),kind)
   {a->type = tc_long_array;
    a->storage_as.long_array.dim = n;
    a->storage_as.long_array.data = (long *) must_malloc(n * sizeof(long));
    for(j=0;j<n;++j) a->storage_as.long_array.data[j] = 0;}
 else if EQ(siod_cintern("string"),kind)
   {a->type = tc_string;
    a->storage_as.string.dim = n;
    a->storage_as.string.data = (char *) must_malloc(n+1);
    a->storage_as.string.data[n] = 0;
    for(j=0;j<n;++j) a->storage_as.string.data[j] = ' ';}
 else if EQ(siod_cintern("byte"),kind)
   {a->type = tc_byte_array;
    a->storage_as.string.dim = n;
    a->storage_as.string.data = (char *) must_malloc(n);
    for(j=0;j<n;++j) a->storage_as.string.data[j] = 0;}
 else if (EQ(siod_cintern("lisp"),kind) || NULLP(kind))
   {a->type = tc_lisp_array;
    a->storage_as.lisp_array.dim = n;
    a->storage_as.lisp_array.data = (siod_LISP *) must_malloc(n * sizeof(siod_LISP));
    for(j=0;j<n;++j) a->storage_as.lisp_array.data[j] = NIL;}
 else
   err("bad type of array",kind);
 siod_no_interrupt(flag);
 return(a);}

siod_LISP siod_string_append(siod_LISP args)
{long size;
 siod_LISP l,s;
 char *data;
 size = 0;
 for(l=args;NNULLP(l);l=siod_cdr(l))
   size += strlen(get_c_string(siod_car(l)));
 s = siod_strcons(size,NULL);
 data = s->storage_as.string.data;
 data[0] = 0;
 for(l=args;NNULLP(l);l=siod_cdr(l))
   strcat(data,get_c_string(siod_car(l)));
 return(s);}

siod_LISP siod_bytes_append(siod_LISP args)
{long size,n,j;
 siod_LISP l,s;
 char *data,*ptr;
 size = 0;
 for(l=args;NNULLP(l);l=siod_cdr(l))
   {get_c_siod_string_dim(siod_car(l),&n);
    size += n;}
 s = siod_ar_cons(tc_byte_array,size,0);
 data = s->storage_as.string.data;
 for(j=0,l=args;NNULLP(l);l=siod_cdr(l))
   {ptr = get_c_siod_string_dim(siod_car(l),&n);
    memcpy(&data[j],ptr,n);
    j += n;}
 return(s);}

siod_LISP siod_substring(siod_LISP str,siod_LISP start,siod_LISP end)
{long s,e,n;
 char *data;
 data = get_c_siod_string_dim(str,&n);
 s = siod_get_c_long(start);
 if NULLP(end)
   e = n;
 else
   e = siod_get_c_long(end);
 if ((s < 0) || (s > e)) err("bad start index",start);
 if ((e < 0) || (e > n)) err("bad end index",end);
 return(siod_strcons(e-s,&data[s]));}

siod_LISP siod_string_search(siod_LISP token,siod_LISP str)
{char *s1,*s2,*ptr;
 s1 = get_c_string(str);
 s2 = get_c_string(token);
 ptr = strstr(s1,s2);
 if (ptr)
   return(siod_flocons(ptr - s1));
 else
   return(NIL);}

#define IS_TRIM_SPACE(_x) (strchr(" \t\r\n",(_x)))

siod_LISP siod_string_trim(siod_LISP str)
{char *start,*end;
 start = get_c_string(str);
 while(*start && IS_TRIM_SPACE(*start)) ++start;
 end = &start[strlen(start)];
 while((end > start) && IS_TRIM_SPACE(*(end-1))) --end;
 return(siod_strcons(end-start,start));}

siod_LISP siod_string_trim_left(siod_LISP str)
{char *start,*end;
 start = get_c_string(str);
 while(*start && IS_TRIM_SPACE(*start)) ++start;
 end = &start[strlen(start)];
 return(siod_strcons(end-start,start));}

siod_LISP siod_string_trim_right(siod_LISP str)
{char *start,*end;
 start = get_c_string(str);
 end = &start[strlen(start)];
 while((end > start) && IS_TRIM_SPACE(*(end-1))) --end;
 return(siod_strcons(end-start,start));}

siod_LISP siod_string_upcase(siod_LISP str)
{siod_LISP result;
 char *s1,*s2;
 long j,n;
 s1 = get_c_string(str);
 n = strlen(s1);
 result = siod_strcons(n,s1);
 s2 = get_c_string(result);
 for(j=0;j<n;++j) s2[j] = toupper(s2[j]);
 return(result);}

siod_LISP siod_string_downcase(siod_LISP str)
{siod_LISP result;
 char *s1,*s2;
 long j,n;
 s1 = get_c_string(str);
 n = strlen(s1);
 result = siod_strcons(n,s1);
 s2 = get_c_string(result);
 for(j=0;j<n;++j) s2[j] = tolower(s2[j]);
 return(result);}

siod_LISP siod_lreadstring(struct gen_readio *f)
{int j,c,n;
 char *p;
 j = 0;
 p = tkbuffer;
 while(((c = GETC_FCN(f)) != '"') && (c != EOF))
   {if (c == '\\')
      {c = GETC_FCN(f);
       if (c == EOF) err("eof after \\",NIL);
       switch(c)
	 {case 'n':
	    c = '\n';
	    break;
	  case 't':
	    c = '\t';
	    break;
	  case 'r':
	    c = '\r';
	    break;
	  case 'd':
	    c = 0x04;
	    break;
	  case 'N':
	    c = 0;
	    break;
	  case 's':
	    c = ' ';
	    break;
	  case '0':
	    n = 0;
	    while(1)
	      {c = GETC_FCN(f);
	       if (c == EOF) err("eof after \\0",NIL);
	       if (isdigit(c))
		 n = n * 8 + c - '0';
	       else
		 {UNGETC_FCN(c,f);
		  break;}}
	    c = n;}}
    if ((j + 1) >= TKBUFFERN) err("read string overflow",NIL);
    ++j;
    *p++ = c;}
 *p = 0;
 return(siod_strcons(j,tkbuffer));}


siod_LISP siod_lreadsharp(struct gen_readio *f)
{siod_LISP obj,l,result;
 long j,n;
 int c;
 c = GETC_FCN(f);
 switch(c)
   {case '(':
      UNGETC_FCN(c,f);
      obj = siod_lreadr(f);
      n = siod_nlength(obj);
      result = siod_ar_cons(tc_lisp_array,n,1);
      for(l=obj,j=0;j<n;l=siod_cdr(l),++j)
	result->storage_as.lisp_array.data[j] = siod_car(l);
      return(result);
    case '.':
      obj = siod_lreadr(f);
      return(siod_leval(obj,NIL));
    case 'f':
      return(NIL);
    case 't':
      return(siod_flocons(1));
    default:
      return(err("readsharp syntax not handled",NIL));}}

#define HASH_COMBINE(_h1,_h2,_mod) ((((_h1) * 17 + 1) ^ (_h2)) % (_mod))

long siod_c_siod_sxhash(siod_LISP obj,long n)
{long hash;
 unsigned char *s;
 siod_LISP tmp;
 struct user_type_hooks *p;
 STACK_CHECK(&obj);
 INTERRUPT_CHECK();
 switch TYPE(obj)
   {case tc_nil:
      return(0);
    case tc_cons:
      hash = siod_c_siod_sxhash(CAR(obj),n);
      for(tmp=CDR(obj);CONSP(tmp);tmp=CDR(tmp))
	hash = HASH_COMBINE(hash,siod_c_siod_sxhash(CAR(tmp),n),n);
      hash = HASH_COMBINE(hash,siod_c_siod_sxhash(tmp,n),n);
      return(hash);
    case tc_symbol:
      for(hash=0,s=(unsigned char *)PNAME(obj);*s;++s)
	hash = HASH_COMBINE(hash,*s,n);
      return(hash);
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      for(hash=0,s=(unsigned char *) obj->storage_as.subr.name;*s;++s)
	hash = HASH_COMBINE(hash,*s,n);
      return(hash);
    case tc_flonum:
      return(((unsigned long)FLONM(obj)) % n);
    default:
      p = get_user_type_hooks(TYPE(obj));
      if (p->siod_c_siod_sxhash)
	return((*p->siod_c_siod_sxhash)(obj,n));
      else
	return(0);}}

siod_LISP siod_sxhash(siod_LISP obj,siod_LISP n)
{return(siod_flocons(siod_c_siod_sxhash(obj,FLONUMP(n) ? (long) FLONM(n) : 10000)));}

siod_LISP siod_equal(siod_LISP a,siod_LISP b)
{struct user_type_hooks *p;
 long atype;
 STACK_CHECK(&a);
 loop:
 INTERRUPT_CHECK();
 if EQ(a,b) return(sym_t);
 atype = TYPE(a);
 if (atype != TYPE(b)) return(NIL);
 switch(atype)
   {case tc_cons:
      if NULLP(siod_equal(siod_car(a),siod_car(b))) return(NIL);
      a = siod_cdr(a);
      b = siod_cdr(b);
      goto loop;
    case tc_flonum:
      return((FLONM(a) == FLONM(b)) ? sym_t : NIL);
    case tc_symbol:
      return(NIL);
    default:
      p = get_user_type_hooks(atype);
      if (p->siod_equal)
	return((*p->siod_equal)(a,b));
      else
	return(NIL);}}

siod_LISP siod_array_siod_equal(siod_LISP a,siod_LISP b)
{long j,len;
 switch(TYPE(a))
   {case tc_string:
    case tc_byte_array:
      len = a->storage_as.string.dim;
      if (len != b->storage_as.string.dim) return(NIL);
      if (memcmp(a->storage_as.string.data,b->storage_as.string.data,len) == 0)
	return(sym_t);
      else
	return(NIL);
    case tc_long_array:
      len = a->storage_as.long_array.dim;
      if (len != b->storage_as.long_array.dim) return(NIL);
      if (memcmp(a->storage_as.long_array.data,
		 b->storage_as.long_array.data,
		 len * sizeof(long)) == 0)
	return(sym_t);
      else
	return(NIL);
    case tc_double_array:
      len = a->storage_as.double_array.dim;
      if (len != b->storage_as.double_array.dim) return(NIL);
      for(j=0;j<len;++j)
	if (a->storage_as.double_array.data[j] !=
	    b->storage_as.double_array.data[j])
	  return(NIL);
      return(sym_t);
    case tc_lisp_array:
      len = a->storage_as.lisp_array.dim;
      if (len != b->storage_as.lisp_array.dim) return(NIL);
      for(j=0;j<len;++j)
	if NULLP(siod_equal(a->storage_as.lisp_array.data[j],
		       b->storage_as.lisp_array.data[j]))
	  return(NIL);
      return(sym_t);
    default:
      return(siod_errswitch());}}

long siod_array_siod_sxhash(siod_LISP a,long n)
{long j,len,hash;
 unsigned char *char_data;
 unsigned long *long_data;
 double *double_data;
 switch(TYPE(a))
   {case tc_string:
    case tc_byte_array:
      len = a->storage_as.string.dim;
      for(j=0,hash=0,char_data=(unsigned char *)a->storage_as.string.data;
	  j < len;
	  ++j,++char_data)
	hash = HASH_COMBINE(hash,*char_data,n);
      return(hash);
    case tc_long_array:
      len = a->storage_as.long_array.dim;
      for(j=0,hash=0,long_data=(unsigned long *)a->storage_as.long_array.data;
	  j < len;
	  ++j,++long_data)
	hash = HASH_COMBINE(hash,*long_data % n,n);
      return(hash);
    case tc_double_array:
      len = a->storage_as.double_array.dim;
      for(j=0,hash=0,double_data=a->storage_as.double_array.data;
	  j < len;
	  ++j,++double_data)
	hash = HASH_COMBINE(hash,(unsigned long)*double_data % n,n);
      return(hash);
    case tc_lisp_array:
      len = a->storage_as.lisp_array.dim;
      for(j=0,hash=0; j < len; ++j)
	hash = HASH_COMBINE(hash,
			    siod_c_siod_sxhash(a->storage_as.lisp_array.data[j],n),
			    n);
      return(hash);
    default:
      siod_errswitch();
      return(0);}}

long siod_href_index(siod_LISP table,siod_LISP key)
{long index;
 if NTYPEP(table,tc_lisp_array) err("not a hash table",table);
 index = siod_c_siod_sxhash(key,table->storage_as.lisp_array.dim);
 if ((index < 0) || (index >= table->storage_as.lisp_array.dim))
   {err("siod_sxhash insiod_consistency",table);
    return(0);}
 else
   return(index);}
 
siod_LISP siod_href(siod_LISP table,siod_LISP key)
{return(siod_cdr(siod_assoc(key,
		  table->storage_as.lisp_array.data[siod_href_index(table,key)])));}

siod_LISP siod_hset(siod_LISP table,siod_LISP key,siod_LISP value)
{long index;
 siod_LISP cell,l;
 index = siod_href_index(table,key);
 l = table->storage_as.lisp_array.data[index];
 if NNULLP(cell = siod_assoc(key,l))
   return(siod_setcdr(cell,value));
 cell = siod_cons(key,value);
 table->storage_as.lisp_array.data[index] = siod_cons(cell,l);
 return(value);}

siod_LISP siod_assoc(siod_LISP x,siod_LISP alist)
{siod_LISP l,tmp;
 for(l=alist;CONSP(l);l=CDR(l))
   {tmp = CAR(l);
    if (CONSP(tmp) && siod_equal(CAR(tmp),x)) return(tmp);
    INTERRUPT_CHECK();}
 if EQ(l,NIL) return(NIL);
 return(err("improper list to siod_assoc",alist));}

siod_LISP siod_assv(siod_LISP x,siod_LISP alist)
{siod_LISP l,tmp;
 for(l=alist;CONSP(l);l=CDR(l))
   {tmp = CAR(l);
    if (CONSP(tmp) && NNULLP(siod_eql(CAR(tmp),x))) return(tmp);
    INTERRUPT_CHECK();}
 if EQ(l,NIL) return(NIL);
 return(err("improper list to assv",alist));}

void siod_put_long(long i,FILE *f)
{fwrite(&i,sizeof(long),1,f);}

long siod_get_long(FILE *f)
{long i;
 fread(&i,sizeof(long),1,f);
 return(i);}

long siod_fast_print_table(siod_LISP obj,siod_LISP table)
{FILE *f;
 siod_LISP ht,index;
 f = siod_get_c_file(siod_car(table),(FILE *) NULL);
 if NULLP(ht = siod_car(siod_cdr(table)))
   return(1);
 index = siod_href(ht,obj);
 if NNULLP(index)
   {putc(FO_fetch,f);
    siod_put_long(siod_get_c_long(index),f);
    return(0);}
 if NULLP(index = siod_car(siod_cdr(siod_cdr(table))))
   return(1);
 siod_hset(ht,obj,index);
 FLONM(bashnum) = 1.0;
 siod_setcar(siod_cdr(siod_cdr(table)),siod_plus(index,bashnum));
 putc(FO_store,f);
 siod_put_long(siod_get_c_long(index),f);
 return(1);}

siod_LISP siod_fast_print(siod_LISP obj,siod_LISP table)
{FILE *f;
 long len;
 siod_LISP tmp;
 struct user_type_hooks *p;
 STACK_CHECK(&obj);
 f = siod_get_c_file(siod_car(table),(FILE *) NULL);
 switch(TYPE(obj))
   {case tc_nil:
      putc(tc_nil,f);
      return(NIL);
    case tc_cons:
      for(len=0,tmp=obj;CONSP(tmp);tmp=CDR(tmp)) {INTERRUPT_CHECK();++len;}
      if (len == 1)
	{putc(tc_cons,f);
	 siod_fast_print(siod_car(obj),table);
	 siod_fast_print(siod_cdr(obj),table);}
      else if NULLP(tmp)
	{putc(FO_list,f);
	 siod_put_long(len,f);
	 for(tmp=obj;CONSP(tmp);tmp=CDR(tmp))
	   siod_fast_print(CAR(tmp),table);}
      else
	{putc(FO_listd,f);
	 siod_put_long(len,f);
	 for(tmp=obj;CONSP(tmp);tmp=CDR(tmp))
	   siod_fast_print(CAR(tmp),table);
	 siod_fast_print(tmp,table);}
      return(NIL);
    case tc_flonum:
      putc(tc_flonum,f);
      fwrite(&obj->storage_as.flonum.data,
	     sizeof(obj->storage_as.flonum.data),
	     1,
	     f);
      return(NIL);
    case tc_symbol:
      if (siod_fast_print_table(obj,table))
	{putc(tc_symbol,f);
	 len = strlen(PNAME(obj));
	 if (len >= TKBUFFERN)
	   err("symbol name too long",obj);
	 siod_put_long(len,f);
	 fwrite(PNAME(obj),len,1,f);
	 return(sym_t);}
      else
	return(NIL);
    default:
      p = get_user_type_hooks(TYPE(obj));
      if (p->siod_fast_print)
	return((*p->siod_fast_print)(obj,table));
      else
	return(err("cannot fast-print",obj));}}

siod_LISP siod_fast_read(siod_LISP table)
{FILE *f;
 siod_LISP tmp,l;
 struct user_type_hooks *p;
 int c;
 long len;
 f = siod_get_c_file(siod_car(table),(FILE *) NULL);
 c = getc(f);
 if (c == EOF) return(table);
 switch(c)
   {case FO_comment:
      while((c = getc(f)))
	switch(c)
	  {case EOF:
	     return(table);
	   case '\n':
	     return(siod_fast_read(table));}
    case FO_fetch:
      len = siod_get_long(f);
      FLONM(bashnum) = len;
      return(siod_href(siod_car(siod_cdr(table)),bashnum));
    case FO_store:
      len = siod_get_long(f);
      tmp = siod_fast_read(table);
      siod_hset(siod_car(siod_cdr(table)),siod_flocons(len),tmp);
      return(tmp);
    case tc_nil:
      return(NIL);
    case tc_cons:
      tmp = siod_fast_read(table);
      return(siod_cons(tmp,siod_fast_read(table)));
    case FO_list:
    case FO_listd:
      len = siod_get_long(f);
      FLONM(bashnum) = len;
      l = siod_make_list(bashnum,NIL);
      tmp = l;
      while(len > 1)
	{CAR(tmp) = siod_fast_read(table);
	 tmp = CDR(tmp);
	 --len;}
      CAR(tmp) = siod_fast_read(table);
      if (c == FO_listd)
	CDR(tmp) = siod_fast_read(table);
      return(l);
    case tc_flonum:
      tmp = siod_newcell(tc_flonum);
      fread(&tmp->storage_as.flonum.data,
	    sizeof(tmp->storage_as.flonum.data),
	    1,
	    f);
      return(tmp);
    case tc_symbol:
      len = siod_get_long(f);
      if (len >= TKBUFFERN)
	err("symbol name too long",NIL);
      fread(tkbuffer,len,1,f);
      tkbuffer[len] = 0;
      return(siod_rintern(tkbuffer));
    default:
      p = get_user_type_hooks(c);
      if (p->siod_fast_read)
	return(*p->siod_fast_read)(c,table);
      else
	return(err("unknown fast-read opcode",siod_flocons(c)));}}

siod_LISP siod_array_siod_fast_print(siod_LISP ptr,siod_LISP table)
{int j,len;
 FILE *f;
 f = siod_get_c_file(siod_car(table),(FILE *) NULL);
 switch (ptr->type)
   {case tc_string:
    case tc_byte_array:
      putc(ptr->type,f);
      len = ptr->storage_as.string.dim;
      siod_put_long(len,f);
      fwrite(ptr->storage_as.string.data,len,1,f);
      return(NIL);
    case tc_double_array:
      putc(tc_double_array,f);
      len = ptr->storage_as.double_array.dim * sizeof(double);
      siod_put_long(len,f);
      fwrite(ptr->storage_as.double_array.data,len,1,f);
      return(NIL);
    case tc_long_array:
      putc(tc_long_array,f);
      len = ptr->storage_as.long_array.dim * sizeof(long);
      siod_put_long(len,f);
      fwrite(ptr->storage_as.long_array.data,len,1,f);
      return(NIL);
    case tc_lisp_array:
      putc(tc_lisp_array,f);
      len = ptr->storage_as.lisp_array.dim;
      siod_put_long(len,f);
      for(j=0; j < len; ++j)
	siod_fast_print(ptr->storage_as.lisp_array.data[j],table);
      return(NIL);
    default:
      return(siod_errswitch());}}

siod_LISP siod_array_siod_fast_read(int code,siod_LISP table)
{long j,len,iflag;
 FILE *f;
 siod_LISP ptr;
 f = siod_get_c_file(siod_car(table),(FILE *) NULL);
 switch (code)
   {case tc_string:
      len = siod_get_long(f);
      ptr = siod_strcons(len,NULL);
      fread(ptr->storage_as.string.data,len,1,f);
      ptr->storage_as.string.data[len] = 0;
      return(ptr);
    case tc_byte_array:
      len = siod_get_long(f);
      iflag = siod_no_interrupt(1);
      ptr = siod_newcell(tc_byte_array);
      ptr->storage_as.string.dim = len;
      ptr->storage_as.string.data =
	(char *) must_malloc(len);
      fread(ptr->storage_as.string.data,len,1,f);
      siod_no_interrupt(iflag);
      return(ptr);
    case tc_double_array:
      len = siod_get_long(f);
      iflag = siod_no_interrupt(1);
      ptr = siod_newcell(tc_double_array);
      ptr->storage_as.double_array.dim = len;
      ptr->storage_as.double_array.data =
	(double *) must_malloc(len * sizeof(double));
      fread(ptr->storage_as.double_array.data,sizeof(double),len,f);
      siod_no_interrupt(iflag);
      return(ptr);
    case tc_long_array:
      len = siod_get_long(f);
      iflag = siod_no_interrupt(1);
      ptr = siod_newcell(tc_long_array);
      ptr->storage_as.long_array.dim = len;
      ptr->storage_as.long_array.data =
	(long *) must_malloc(len * sizeof(long));
      fread(ptr->storage_as.long_array.data,sizeof(long),len,f);
      siod_no_interrupt(iflag);
      return(ptr);
    case tc_lisp_array:
      len = siod_get_long(f);
      FLONM(bashnum) = len;
      ptr = siod_cons_array(bashnum,NIL);
      for(j=0; j < len; ++j)
	ptr->storage_as.lisp_array.data[j] = siod_fast_read(table);
      return(ptr);
    default:
      return(siod_errswitch());}}

long siod_get_c_long(siod_LISP x)
{if NFLONUMP(x) err("not a number",x);
 return((long)FLONM(x));}

double siod_get_c_double(siod_LISP x)
{if NFLONUMP(x) err("not a number",x);
 return(FLONM(x));}

siod_LISP siod_make_list(siod_LISP x,siod_LISP v)
{long n;
 siod_LISP l;
 n = siod_get_c_long(x);
 l = NIL;
 while(n > 0)
   {l = siod_cons(v,l); --n;}
 return(l);}

siod_LISP siod_lfread(siod_LISP size,siod_LISP file)
{long flag,n,ret,m;
 char *buffer;
 siod_LISP s;
 FILE *f;
 f = siod_get_c_file(file,stdin);
 flag = siod_no_interrupt(1);
 switch(TYPE(size))
   {case tc_string:
    case tc_byte_array:
      s = size;
      buffer = s->storage_as.string.data;
      n = s->storage_as.string.dim;
      m = 0;
      break;
    default:
      n = siod_get_c_long(size);
      buffer = (char *) must_malloc(n+1);
      buffer[n] = 0;
      m = 1;}
 ret = fread(buffer,1,n,f);
 if (ret == 0)
   {if (m)
      free(buffer);
    siod_no_interrupt(flag);
    return(NIL);}
 if (m)
   {if (ret == n)
      {s = siod_cons(NIL,NIL);
       s->type = tc_string;
       s->storage_as.string.data = buffer;
       s->storage_as.string.dim = n;}
    else
      {s = siod_strcons(ret,NULL);
       memcpy(s->storage_as.string.data,buffer,ret);
       free(buffer);}
    siod_no_interrupt(flag);
    return(s);}
 siod_no_interrupt(flag);
 return(siod_flocons((double)ret));}

siod_LISP siod_lfwrite(siod_LISP string,siod_LISP file)
{FILE *f;
 long flag;
 char *data;
 long dim,len;
 f = siod_get_c_file(file,stdout);
 data = get_c_siod_string_dim(CONSP(string) ? siod_car(string) : string,&dim);
 len = CONSP(string) ? siod_get_c_long(siod_cadr(string)) : dim;
 if (len <= 0) return(NIL);
 if (len > dim) err("write length too long",string);
 flag = siod_no_interrupt(1);
 fwrite(data,1,len,f);
 siod_no_interrupt(flag);
 return(NIL);}

siod_LISP siod_lfflush(siod_LISP file)
{FILE *f;
 long flag;
 f = siod_get_c_file(file,stdout);
 flag = siod_no_interrupt(1);
 fflush(f);
 siod_no_interrupt(flag);
 return(NIL);}

siod_LISP siod_string_length(siod_LISP string)
{if NTYPEP(string,tc_string) siod_err_wta_str(string);
 return(siod_flocons(strlen(string->storage_as.string.data)));}

siod_LISP siod_string_dim(siod_LISP string)
{if NTYPEP(string,tc_string) siod_err_wta_str(string);
 return(siod_flocons((double)string->storage_as.string.dim));}

long siod_nlength(siod_LISP obj)
{siod_LISP l;
 long n;
 switch TYPE(obj)
   {case tc_string:
      return(strlen(obj->storage_as.string.data));
    case tc_byte_array:
      return(obj->storage_as.string.dim);
    case tc_double_array:
      return(obj->storage_as.double_array.dim);
    case tc_long_array:
      return(obj->storage_as.long_array.dim);
    case tc_lisp_array:
      return(obj->storage_as.lisp_array.dim);
    case tc_nil:
      return(0);
    case tc_cons:
      for(l=obj,n=0;CONSP(l);l=CDR(l),++n) INTERRUPT_CHECK();
      if NNULLP(l) err("improper list to length",obj);
      return(n);
    default:
      err("wta to length",obj);
      return(0);}}

siod_LISP siod_llength(siod_LISP obj)
{return(siod_flocons(siod_nlength(obj)));}

siod_LISP siod_number2string(siod_LISP x,siod_LISP b,siod_LISP w,siod_LISP p)
{char buffer[1000];
 double y;
 int base,width,prec;
 if NFLONUMP(x) err("wta",x);
 y = FLONM(x);
 width = NNULLP(w) ? siod_get_c_long(w) : -1;
 if (width > 100) err("width too long",w);
 prec = NNULLP(p) ? siod_get_c_long(p) : -1;
 if (prec > 100) err("precision too large",p);
 if (NULLP(b) || EQ(sym_e,b) || EQ(sym_f,b))
   {if ((width >= 0) && (prec >= 0))
      sprintf(buffer,
	      NULLP(b) ? "% *.*g" : EQ(sym_e,b) ? "% *.*e" : "% *.*f",
	      width,
	      prec,
	      y);
    else if (width >= 0)
      sprintf(buffer,
	      NULLP(b) ? "% *g" : EQ(sym_e,b) ? "% *e" : "% *f",
	      width,
	      y);
    else if (prec >= 0)
      sprintf(buffer,
	      NULLP(b) ? "%.*g" : EQ(sym_e,b) ? "%.*e" : "%.*f",
	      prec,
	      y);
    else
      sprintf(buffer,
	      NULLP(b) ? "%g" : EQ(sym_e,b) ? "%e" : "%f",
	      y);}
 else if (((base = siod_get_c_long(b)) == 10) || (base == 8) || (base == 16))
   {if (width >= 0)
      sprintf(buffer,
	      (base == 10) ? "%0*ld" : (base == 8) ? "%0*lo" : "%0*lX",
	      width,
	      (long) y);
    else
      sprintf(buffer,
	      (base == 10) ? "%ld" : (base == 8) ? "%lo" : "%lX",
	      (long) y);}
 else
   err("number base not handled",b);
 return(siod_strcons(strlen(buffer),buffer));}

siod_LISP siod_string2number(siod_LISP x,siod_LISP b)
{char *str;
 long base,value = 0;
 double result;
 str = get_c_string(x);
 if NULLP(b)
   result = atof(str);
 else if ((base = siod_get_c_long(b)) == 10)
   {sscanf(str,"%ld",&value);
    result = (double) value;}
 else if (base == 8)
   {sscanf(str,"%lo",&value);
    result = (double) value;}
 else if (base == 16)
   {sscanf(str,"%lx",&value);
    result = (double) value;}
 else if ((base >= 1) && (base <= 16))
   {for(result = 0.0;*str;++str)
     if (isdigit(*str))
       result = result * base + *str - '0';
     else if (isxdigit(*str))
       result = result * base + toupper(*str) - 'A' + 10;}
 else
   return(err("number base not handled",b));
 return(siod_flocons(result));}

siod_LISP siod_lstrcmp(siod_LISP s1,siod_LISP s2)
{return(siod_flocons(strcmp(get_c_string(s1),get_c_string(s2))));}

void siod_chk_string(siod_LISP s,char **data,long *dim)
{if TYPEP(s,tc_string)
   {*data = s->storage_as.string.data;
    *dim = s->storage_as.string.dim;}
 else
   siod_err_wta_str(s);}

siod_LISP siod_lstrcpy(siod_LISP dest,siod_LISP src)
{long ddim,slen;
 char *d,*s;
 siod_chk_string(dest,&d,&ddim);
 s = get_c_string(src);
 slen = strlen(s);
 if (slen > ddim)
   err("string too long",src);
 memcpy(d,s,slen);
 d[slen] = 0;
 return(NIL);}

siod_LISP siod_lstrcat(siod_LISP dest,siod_LISP src)
{long ddim,dlen,slen;
 char *d,*s;
 siod_chk_string(dest,&d,&ddim);
 s = get_c_string(src);
 slen = strlen(s);
 dlen = strlen(d);
 if ((slen + dlen) > ddim)
   err("string too long",src);
 memcpy(&d[dlen],s,slen);
 d[dlen+slen] = 0;
 return(NIL);}

siod_LISP siod_lstrbreakup(siod_LISP str,siod_LISP lmarker)
{char *start,*end,*marker;
 size_t k;
 siod_LISP result = NIL;
 start = get_c_string(str);
 marker = get_c_string(lmarker);
 k = strlen(marker);
 while(*start)
   {if (!(end = strstr(start,marker))) end = &start[strlen(start)];
    result = siod_cons(siod_strcons(end-start,start),result);
    start = (*end) ? end+k : end;}
 return(siod_nreverse(result));}

siod_LISP siod_lstrunbreakup(siod_LISP elems,siod_LISP lmarker)
{siod_LISP result,l;
 for(l=elems,result=NIL;NNULLP(l);l=siod_cdr(l))
   if EQ(l,elems)
     result = siod_cons(siod_car(l),result);
   else
     result = siod_cons(siod_car(l),siod_cons(lmarker,result));
 return(siod_string_append(siod_nreverse(result)));}

siod_LISP siod_stringp(siod_LISP x)
{return(TYPEP(x,tc_string) ? sym_t : NIL);}

static char *base64_encode_table = "\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\
abcdefghijklmnopqrstuvwxyz\
0123456789+/=";

static char *base64_decode_table = NULL;

static void siod_init_base64_table(void)
{int j;
 base64_decode_table = (char *) malloc(256);
 memset(base64_decode_table,-1,256);
 for(j=0;j<65;++j)
   base64_decode_table[(int)(base64_encode_table[j])] = j;}

#define BITMSK(N) ((1 << (N)) - 1)

#define ITEM1(X)   (X >> 2) & BITMSK(6)
#define ITEM2(X,Y) ((X & BITMSK(2)) << 4) | ((Y >> 4) & BITMSK(4))
#define ITEM3(X,Y) ((X & BITMSK(4)) << 2) | ((Y >> 6) & BITMSK(2))
#define ITEM4(X)   X & BITMSK(6)

siod_LISP siod_base64encode(siod_LISP in)
{char *s,*t = base64_encode_table;
 unsigned char *p1,*p2;
 siod_LISP out;
 long j,m,n,chunks,leftover;
 s = get_c_siod_string_dim(in,&n);
 chunks = n / 3;
 leftover = n % 3;
 m = (chunks + ((leftover) ? 1 : 0)) * 4;
 out = siod_strcons(m,NULL);
 p2 = (unsigned char *) get_c_string(out);
 for(j=0,p1=(unsigned char *)s;j<chunks;++j,p1 += 3)
   {*p2++ = t[ITEM1(p1[0])];
    *p2++ = t[ITEM2(p1[0],p1[1])];
    *p2++ = t[ITEM3(p1[1],p1[2])];
    *p2++ = t[ITEM4(p1[2])];}
 switch(leftover)
   {case 0:
      break;
    case 1:
      *p2++ = t[ITEM1(p1[0])];
      *p2++ = t[ITEM2(p1[0],0)];
      *p2++ = base64_encode_table[64];
      *p2++ = base64_encode_table[64];
      break;
    case 2:
      *p2++ = t[ITEM1(p1[0])];
      *p2++ = t[ITEM2(p1[0],p1[1])];
      *p2++ = t[ITEM3(p1[1],0)];
      *p2++ = base64_encode_table[64];
      break;
    default:
      siod_errswitch();}
 return(out);}

siod_LISP siod_base64decode(siod_LISP in)
{char *s,*t = base64_decode_table;
 siod_LISP out;
 unsigned char *p1,*p2;
 long j,m,n,chunks,leftover,item1,item2,item3,item4;
 s = get_c_string(in);
 n = strlen(s);
 if (n == 0) return(siod_strcons(0,NULL));
 if (n % 4)
   err("illegal base64 data length",in);
 if (s[n-1] == base64_encode_table[64])
   if (s[n-2] == base64_encode_table[64])
     leftover = 1;
   else
     leftover = 2;
 else
   leftover = 0;
 chunks = (n / 4 ) - ((leftover) ? 1 : 0);
 m = (chunks * 3) + leftover;
 out = siod_strcons(m,NULL);
 p2 = (unsigned char *) get_c_string(out);
 for(j=0,p1=(unsigned char *)s;j<chunks;++j,p1 += 4)
   {if ((item1 = t[p1[0]]) & ~BITMSK(6)) return(NIL);
    if ((item2 = t[p1[1]]) & ~BITMSK(6)) return(NIL);
    if ((item3 = t[p1[2]]) & ~BITMSK(6)) return(NIL);
    if ((item4 = t[p1[3]]) & ~BITMSK(6)) return(NIL);
    *p2++ = (unsigned char) ((item1 << 2) | (item2 >> 4));
    *p2++ = (unsigned char) ((item2 << 4) | (item3 >> 2));
    *p2++ = (unsigned char) ((item3 << 6) | item4);}
 switch(leftover)
   {case 0:
      break;
    case 1:
      if ((item1 = t[p1[0]]) & ~BITMSK(6)) return(NIL);
      if ((item2 = t[p1[1]]) & ~BITMSK(6)) return(NIL);
      *p2++ = (unsigned char) ((item1 << 2) | (item2 >> 4));
      break;
    case 2:
      if ((item1 = t[p1[0]]) & ~BITMSK(6)) return(NIL);
      if ((item2 = t[p1[1]]) & ~BITMSK(6)) return(NIL);
      if ((item3 = t[p1[2]]) & ~BITMSK(6)) return(NIL);
      *p2++ = (unsigned char) ((item1 << 2) | (item2 >> 4));
      *p2++ = (unsigned char) ((item2 << 4) | (item3 >> 2));
      break;
    default:
      siod_errswitch();}
 return(out);}

siod_LISP siod_memq(siod_LISP x,siod_LISP il)
{siod_LISP l,tmp;
 for(l=il;CONSP(l);l=CDR(l))
   {tmp = CAR(l);
    if EQ(x,tmp) return(l);
    INTERRUPT_CHECK();}
 if EQ(l,NIL) return(NIL);
 return(err("improper list to memq",il));}

siod_LISP siod_member(siod_LISP x,siod_LISP il)
{siod_LISP l,tmp;
 for(l=il;CONSP(l);l=CDR(l))
   {tmp = CAR(l);
    if NNULLP(siod_equal(x,tmp)) return(l);
    INTERRUPT_CHECK();}
 if EQ(l,NIL) return(NIL);
 return(err("improper list to siod_member",il));}

siod_LISP siod_memv(siod_LISP x,siod_LISP il)
{siod_LISP l,tmp;
 for(l=il;CONSP(l);l=CDR(l))
   {tmp = CAR(l);
    if NNULLP(siod_eql(x,tmp)) return(l);
    INTERRUPT_CHECK();}
 if EQ(l,NIL) return(NIL);
 return(err("improper list to memv",il));}


siod_LISP siod_nth(siod_LISP x,siod_LISP li)
{siod_LISP l;
 long j,n = siod_get_c_long(x);
 for(j = 0, l = li; (j < n) && CONSP(l); ++j) l = CDR(l);
 if CONSP(l)
   return(CAR(l));
 else
   return(err("bad arg to nth",x));}

/* these lxxx_default functions are convenient for manipulating
   command-line argument lists */

siod_LISP siod_lref_default(siod_LISP li,siod_LISP x,siod_LISP fcn)
{siod_LISP l;
 long j,n = siod_get_c_long(x);
 for(j = 0, l = li; (j < n) && CONSP(l); ++j) l = CDR(l);
 if CONSP(l)
   return(CAR(l));
 else if NNULLP(fcn)
   return(siod_lapply(fcn,NIL));
 else
   return(NIL);}

siod_LISP siod_larg_default(siod_LISP li,siod_LISP x,siod_LISP dval)
{siod_LISP l = li,elem;
 long j=0,n = siod_get_c_long(x);
 while NNULLP(l)
   {elem = siod_car(l);
    if (TYPEP(elem,tc_string) && strchr("-:",*get_c_string(elem)))
      l = siod_cdr(l);
    else if (j == n)
      return(elem);
    else
      {l = siod_cdr(l);
       ++j;}}
 return(dval);}

siod_LISP siod_lkey_default(siod_LISP li,siod_LISP key,siod_LISP dval)
{siod_LISP l = li,elem;
 char *ckey,*celem;
 long n;
 ckey = get_c_string(key);
 n = strlen(ckey);
 while NNULLP(l)
   {elem = siod_car(l);
    if (TYPEP(elem,tc_string) && (*(celem = get_c_string(elem)) == ':') &&
	(strncmp(&celem[1],ckey,n) == 0) && (celem[n+1] == '='))
      return(siod_strcons(strlen(&celem[n+2]),&celem[n+2]));
    l = siod_cdr(l);}
 return(dval);}


siod_LISP siod_llist(siod_LISP l)
{return(l);}

siod_LISP siod_writes1(FILE *f,siod_LISP l)
{siod_LISP v;
 STACK_CHECK(&v);
 INTERRUPT_CHECK();
 for(v=l;CONSP(v);v=CDR(v))
   siod_writes1(f,CAR(v));
 switch TYPE(v)
   {case tc_nil:
      break;
    case tc_symbol:
    case tc_string:
      siod_fput_st(f,get_c_string(v));
      break;
    default:
      siod_lprin1f(v,f);
      break;}
 return(NIL);}

siod_LISP siod_writes(siod_LISP args)
{return(siod_writes1(siod_get_c_file(siod_car(args),stdout),siod_cdr(args)));}

siod_LISP siod_last(siod_LISP l)
{siod_LISP v1,v2;
 v1 = l;
 v2 = CONSP(v1) ? CDR(v1) : err("bad arg to last",l);
 while(CONSP(v2))
   {INTERRUPT_CHECK();
    v1 = v2;
    v2 = CDR(v2);}
 return(v1);}

siod_LISP siod_butlast(siod_LISP l)
{INTERRUPT_CHECK();
 STACK_CHECK(&l);
 if NULLP(l) err("list is empty",l);
 if CONSP(l){
   if NULLP(CDR(l))
     return(NIL);
   else
     return(siod_cons(CAR(l),siod_butlast(CDR(l))));}
 return(err("not a list",l));}

siod_LISP siod_nconc(siod_LISP a,siod_LISP b)
{if NULLP(a)
   return(b);
 siod_setcdr(siod_last(a),b);
 return(a);}

siod_LISP siod_funcall1(siod_LISP fcn,siod_LISP a1)
{switch TYPE(fcn)
   {case tc_subr_1:
      STACK_CHECK(&fcn);
      INTERRUPT_CHECK();
      return(SUBR1(fcn)(a1));
    case tc_closure:
      if TYPEP(fcn->storage_as.closure.code,tc_subr_2)
	{STACK_CHECK(&fcn);
	 INTERRUPT_CHECK();
	 return(SUBR2(fcn->storage_as.closure.code)
		(fcn->storage_as.closure.env,a1));}
    default:
      return(siod_lapply(fcn,siod_cons(a1,NIL)));}}

siod_LISP siod_funcall2(siod_LISP fcn,siod_LISP a1,siod_LISP a2)
{switch TYPE(fcn)
   {case tc_subr_2:
    case tc_subr_2n:
      STACK_CHECK(&fcn);
      INTERRUPT_CHECK();
      return(SUBR2(fcn)(a1,a2));
    default:
      return(siod_lapply(fcn,siod_cons(a1,siod_cons(a2,NIL))));}}

siod_LISP siod_lqsort(siod_LISP l,siod_LISP f,siod_LISP g)
     /* this is a stupid recursive qsort */
{int j,n;
 siod_LISP v,mark,less,notless;
 for(v=l,n=0;CONSP(v);v=CDR(v),++n) INTERRUPT_CHECK();
 if NNULLP(v) err("bad list to qsort",l);
 if (n == 0)
   return(NIL);
 j = rand() % n;
 for(v=l,n=0;n<j;++n) v=CDR(v);
 mark = CAR(v);
 for(less=NIL,notless=NIL,v=l,n=0;NNULLP(v);v=CDR(v),++n)
   if (j != n)
     {if NNULLP(siod_funcall2(f,
			 NULLP(g) ? CAR(v) : siod_funcall1(g,CAR(v)),
			 NULLP(g) ? mark   : siod_funcall1(g,mark)))
	less = siod_cons(CAR(v),less);
      else
	notless = siod_cons(CAR(v),notless);}
 return(siod_nconc(siod_lqsort(less,f,g),
	      siod_cons(mark,
		   siod_lqsort(notless,f,g))));}

siod_LISP siod_string_lessp(siod_LISP s1,siod_LISP s2)
{if (strcmp(get_c_string(s1),get_c_string(s2)) < 0)
   return(sym_t);
 else
   return(NIL);}

siod_LISP siod_benchmark_siod_funcall1(siod_LISP ln,siod_LISP f,siod_LISP a1)
{long j,n;
 siod_LISP value = NIL;
 n = siod_get_c_long(ln);
 for(j=0;j<n;++j)
   value = siod_funcall1(f,a1);
 return(value);}

siod_LISP siod_benchmark_siod_funcall2(siod_LISP l)
{long j,n;
 siod_LISP ln = siod_car(l);siod_LISP f = siod_car(siod_cdr(l)); siod_LISP a1 = siod_car(siod_cdr(siod_cdr(l)));
 siod_LISP a2 = siod_car(siod_cdr(siod_cdr(siod_cdr(l))));
 siod_LISP value = NIL;
 n = siod_get_c_long(ln);
 for(j=0;j<n;++j)
   value = siod_funcall2(f,a1,a2);
 return(value);}

siod_LISP siod_benchmark_eval(siod_LISP ln,siod_LISP exp,siod_LISP env)
{long j,n;
 siod_LISP value = NIL;
 n = siod_get_c_long(ln);
 for(j=0;j<n;++j)
   value = siod_leval(exp,env);
 return(value);}

siod_LISP siod_mapcar1(siod_LISP fcn,siod_LISP in)
{siod_LISP res,ptr,l;
 if NULLP(in) return(NIL);
 res = ptr = siod_cons(siod_funcall1(fcn,siod_car(in)),NIL);
 for(l=siod_cdr(in);CONSP(l);l=CDR(l))
   ptr = CDR(ptr) = siod_cons(siod_funcall1(fcn,CAR(l)),CDR(ptr));
 return(res);}

siod_LISP siod_mapcar2(siod_LISP fcn,siod_LISP in1,siod_LISP in2)
{siod_LISP res,ptr,l1,l2;
 if (NULLP(in1) || NULLP(in2)) return(NIL);
 res = ptr = siod_cons(siod_funcall2(fcn,siod_car(in1),siod_car(in2)),NIL);
 for(l1=siod_cdr(in1),l2=siod_cdr(in2);CONSP(l1) && CONSP(l2);l1=CDR(l1),l2=CDR(l2))
   ptr = CDR(ptr) = siod_cons(siod_funcall2(fcn,CAR(l1),CAR(l2)),CDR(ptr));
 return(res);}

siod_LISP siod_mapcar(siod_LISP l)
{siod_LISP fcn = siod_car(l);
 switch(siod_get_c_long(siod_llength(l)))
   {case 2:
      return(siod_mapcar1(fcn,siod_car(siod_cdr(l))));
    case 3:
      return(siod_mapcar2(fcn,siod_car(siod_cdr(l)),siod_car(siod_cdr(siod_cdr(l)))));
    default:
      return(err("siod_mapcar case not handled",l));}}

siod_LISP siod_lfmod(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to fmod",x);
 if NFLONUMP(y) err("wta(2nd) to fmod",y);
 return(siod_flocons(fmod(FLONM(x),FLONM(y))));}

siod_LISP siod_lsubset(siod_LISP fcn,siod_LISP l)
{siod_LISP result = NIL,v;
 for(v=l;CONSP(v);v=CDR(v))
   if NNULLP(siod_funcall1(fcn,CAR(v)))
     result = siod_cons(CAR(v),result);
 return(siod_nreverse(result));}

siod_LISP siod_ass(siod_LISP x,siod_LISP alist,siod_LISP fcn)
{siod_LISP l,tmp;
 for(l=alist;CONSP(l);l=CDR(l))
   {tmp = CAR(l);
    if (CONSP(tmp) && NNULLP(siod_funcall2(fcn,CAR(tmp),x))) return(tmp);
    INTERRUPT_CHECK();}
 if EQ(l,NIL) return(NIL);
 return(err("improper list to ass",alist));}

siod_LISP siod_append2(siod_LISP l1,siod_LISP l2)
{long n;
 siod_LISP result = NIL,p1,p2;
 n = siod_nlength(l1) + siod_nlength(l2);
 while(n > 0) {result = siod_cons(NIL,result); --n;}
 for(p1=result,p2=l1;NNULLP(p2);p1=siod_cdr(p1),p2=siod_cdr(p2)) siod_setcar(p1,siod_car(p2));
 for(p2=l2;NNULLP(p2);p1=siod_cdr(p1),p2=siod_cdr(p2)) siod_setcar(p1,siod_car(p2));
 return(result);}

siod_LISP siod_append(siod_LISP l)
{STACK_CHECK(&l);
 INTERRUPT_CHECK();
 if NULLP(l)
   return(NIL);
 else if NULLP(siod_cdr(l))
   return(siod_car(l));
 else if NULLP(siod_cddr(l))
   return(siod_append2(siod_car(l),siod_cadr(l)));
 else
   return(siod_append2(siod_car(l),siod_append(siod_cdr(l))));}

siod_LISP siod_listn(long n, ...)
{siod_LISP result,ptr;
 long j;
 va_list args;
 for(j=0,result=NIL;j<n;++j)  result = siod_cons(NIL,result);
 va_start(args,n);
 for(j=0,ptr=result;j<n;ptr=siod_cdr(ptr),++j)
   siod_setcar(ptr,va_arg(args,siod_LISP));
 va_end(args);
 return(result);}


siod_LISP siod_fast_load(siod_LISP lfname,siod_LISP noeval)
{char *fname;
 siod_LISP stream;
 siod_LISP result = NIL,form;
 fname = get_c_string(lfname);
 if (siod_verbose_level >= 3)
   {siod_put_st("fast siod_loading ");
    siod_put_st(fname);
    siod_put_st("\n");}
 stream = siod_listn(3,
		siod_fopen_c(fname,"rb"),
		siod_cons_array(siod_flocons(100),NIL),
		siod_flocons(0));
 while(NEQ(stream,form = siod_fast_read(stream)))
   {if (siod_verbose_level >= 5)
      siod_lprint(form,NIL);
    if NULLP(noeval)
      siod_leval(form,NIL);
    else
      result = siod_cons(form,result);}
 siod_fclose_l(siod_car(stream));
 if (siod_verbose_level >= 3)
   siod_put_st("done.\n");
 return(siod_nreverse(result));}

static void siod_ssiod_hexstr(char *outstr,void *buff,size_t len)
{unsigned char *data = buff;
 size_t j;
 for(j=0;j<len;++j)
   sprintf(&outstr[j*2],"%02X",data[j]);}

siod_LISP siod_fast_save(siod_LISP fname,siod_LISP forms,siod_LISP nohash,siod_LISP comment,siod_LISP fmode)
{char *cname,msgbuff[100],databuff[50];
 siod_LISP stream,l;
 FILE *f;
 long l_one = 1;
 double d_one = 1.0;
 cname = get_c_string(fname);
 if (siod_verbose_level >= 3)
   {siod_put_st("fast saving forms to ");
    siod_put_st(cname);
    siod_put_st("\n");}
 stream = siod_listn(3,
		siod_fopen_c(cname,NNULLP(fmode) ? get_c_string(fmode) : "wb"),
		NNULLP(nohash) ? NIL : siod_cons_array(siod_flocons(100),NIL),
		siod_flocons(0));
 f = siod_get_c_file(siod_car(stream),NULL);
 if NNULLP(comment)
   siod_fput_st(f,get_c_string(comment));
 sprintf(msgbuff,"# Siod Binary Object Save File\n");
 siod_fput_st(f,msgbuff);
 sprintf(msgbuff,"# sizeof(long) = %d\n# sizeof(double) = %d\n",
	 sizeof(long),sizeof(double));
 siod_fput_st(f,msgbuff);
 siod_ssiod_hexstr(databuff,&l_one,sizeof(l_one));
 sprintf(msgbuff,"# 1 = %s\n",databuff);
 siod_fput_st(f,msgbuff);
 siod_ssiod_hexstr(databuff,&d_one,sizeof(d_one));
 sprintf(msgbuff,"# 1.0 = %s\n",databuff);
 siod_fput_st(f,msgbuff);
 for(l=forms;NNULLP(l);l=siod_cdr(l))
   siod_fast_print(siod_car(l),stream);
 siod_fclose_l(siod_car(stream));
 if (siod_verbose_level >= 3)
   siod_put_st("done.\n");
 return(NIL);}

void siod_swrite1(siod_LISP stream,siod_LISP data)
{FILE *f = siod_get_c_file(stream,stdout);
 switch TYPE(data)
   {case tc_symbol:
    case tc_string:
      siod_fput_st(f,get_c_string(data));
      break;
    default:
      siod_lprin1f(data,f);
      break;}}

static siod_LISP siod_swrite2(siod_LISP name,siod_LISP table)
{siod_LISP value,key;
 if (SYMBOLP(name) && (PNAME(name)[0] == '.'))
   key = siod_rintern(&PNAME(name)[1]);
 else
   key = name;
 value = siod_href(table,key);
 if (CONSP(value))
   {if (CONSP(CDR(value)) && EQ(name,key))
     siod_hset(table,key,CDR(value));
    return(CAR(value));}
 else if (NULLP(value))
   return(name);
 else
   return(value);}

siod_LISP siod_swrite(siod_LISP stream,siod_LISP table,siod_LISP data)
{long j,k,m,n;
 switch(TYPE(data))
   {case tc_symbol:
      siod_swrite1(stream,siod_swrite2(data,table));
      break;
    case tc_lisp_array:
      n = data->storage_as.lisp_array.dim;
      if (n < 1)
	err("no object repeat count",data);
      m = siod_get_c_long(siod_swrite2(data->storage_as.lisp_array.data[0],
			     table));
      for(k=0;k<m;++k)
	for(j=1;j<n;++j)
	  siod_swrite(stream,table,data->storage_as.lisp_array.data[j]);
      break;
    case tc_cons:
      /* this should be handled similar to the array case */
      break;
    default:
      siod_swrite1(stream,data);}
 return(NIL);}

siod_LISP siod_lpow(siod_LISP x,siod_LISP y)
{if NFLONUMP(x) err("wta(1st) to pow",x);
 if NFLONUMP(y) err("wta(2nd) to pow",y);
 return(siod_flocons(pow(FLONM(x),FLONM(y))));}

siod_LISP siod_lexp(siod_LISP x)
{return(siod_flocons(exp(siod_get_c_double(x))));}

siod_LISP siod_llog(siod_LISP x)
{return(siod_flocons(log(siod_get_c_double(x))));}

siod_LISP siod_lsin(siod_LISP x)
{return(siod_flocons(sin(siod_get_c_double(x))));}

siod_LISP siod_lcos(siod_LISP x)
{return(siod_flocons(cos(siod_get_c_double(x))));}

siod_LISP siod_ltan(siod_LISP x)
{return(siod_flocons(tan(siod_get_c_double(x))));}

siod_LISP siod_lasin(siod_LISP x)
{return(siod_flocons(asin(siod_get_c_double(x))));}

siod_LISP siod_lacos(siod_LISP x)
{return(siod_flocons(acos(siod_get_c_double(x))));}

siod_LISP siod_latan(siod_LISP x)
{return(siod_flocons(atan(siod_get_c_double(x))));}

siod_LISP siod_latan2(siod_LISP x,siod_LISP y)
{return(siod_flocons(atan2(siod_get_c_double(x),siod_get_c_double(y))));}

siod_LISP siod_lsinh(siod_LISP x)
{return(siod_flocons(sinh(siod_get_c_double(x))));}

siod_LISP siod_lcosh(siod_LISP x)
{return(siod_flocons(cosh(siod_get_c_double(x))));}

siod_LISP siod_ltanh(siod_LISP x)
{return(siod_flocons(tanh(siod_get_c_double(x))));}

siod_LISP siod_lasinh(siod_LISP x)
{return(siod_flocons(asinh(siod_get_c_double(x))));}

siod_LISP siod_lacosh(siod_LISP x)
{return(siod_flocons(acosh(siod_get_c_double(x))));}

siod_LISP siod_latanh(siod_LISP x)
{return(siod_flocons(atanh(siod_get_c_double(x))));}

siod_LISP siod_hexstr(siod_LISP a)
{unsigned char *in;
 char *out;
 siod_LISP result;
 long j,dim;
 in = (unsigned char *) get_c_siod_string_dim(a,&dim);
 result = siod_strcons(dim*2,NULL);
 for(out=get_c_string(result),j=0;j<dim;++j,out += 2)
   sprintf(out,"%02x",in[j]);
 return(result);}

static int siod_xdigitvalue(int c)
{if (isdigit(c))
   return(c - '0');
 if (isxdigit(c))
   return(toupper(c) - 'A' + 10);
 return(0);}

siod_LISP siod_hexstr2bytes(siod_LISP a)
{char *in;
 unsigned char *out;
 siod_LISP result;
 long j,dim;
 in = get_c_string(a);
 dim = strlen(in) / 2; 
 result = siod_ar_cons(tc_byte_array,dim,0);
 out = (unsigned char *) result->storage_as.string.data;
 for(j=0;j<dim;++j)
   out[j] = siod_xdigitvalue(in[j*2]) * 16 + siod_xdigitvalue(in[j*2+1]);
 return(result);}

siod_LISP siod_getprop(siod_LISP plist,siod_LISP key)
{siod_LISP l;
 for(l=siod_cdr(plist);NNULLP(l);l=siod_cddr(l))
   if EQ(siod_car(l),key)
     return(siod_cadr(l));
   else
     INTERRUPT_CHECK();
 return(NIL);}

siod_LISP siod_setprop(siod_LISP plist,siod_LISP key,siod_LISP value)
{err("not implemented",NIL);
 return(NIL);}

siod_LISP siod_putprop(siod_LISP plist,siod_LISP value,siod_LISP key)
{return(siod_setprop(plist,key,value));}

siod_LISP siod_ltypeof(siod_LISP obj)
{long x;
 x = TYPE(obj);
 switch(x)
   {case tc_nil: return(siod_cintern("tc_nil"));
    case tc_cons: return(siod_cintern("tc_cons"));
    case tc_flonum: return(siod_cintern("tc_flonum"));
    case tc_symbol: return(siod_cintern("tc_symbol"));
    case tc_subr_0: return(siod_cintern("tc_subr_0"));
    case tc_subr_1: return(siod_cintern("tc_subr_1"));
    case tc_subr_2: return(siod_cintern("tc_subr_2"));
    case tc_subr_2n: return(siod_cintern("tc_subr_2n"));
    case tc_subr_3: return(siod_cintern("tc_subr_3"));
    case tc_subr_4: return(siod_cintern("tc_subr_4"));
    case tc_subr_5: return(siod_cintern("tc_subr_5"));
    case tc_lsubr: return(siod_cintern("tc_lsubr"));
    case tc_fsubr: return(siod_cintern("tc_fsubr"));
    case tc_msubr: return(siod_cintern("tc_msubr"));
    case tc_closure: return(siod_cintern("tc_closure"));
    case tc_free_cell: return(siod_cintern("tc_free_cell"));
    case tc_string: return(siod_cintern("tc_string"));
    case tc_byte_array: return(siod_cintern("tc_byte_array"));
    case tc_double_array: return(siod_cintern("tc_double_array"));
    case tc_long_array: return(siod_cintern("tc_long_array"));
    case tc_lisp_array: return(siod_cintern("tc_lisp_array"));
    case tc_c_file: return(siod_cintern("tc_c_file"));
    default: return(siod_flocons(x));}}

siod_LISP siod_caaar(siod_LISP x)
{return(siod_car(siod_car(siod_car(x))));}

siod_LISP siod_caadr(siod_LISP x)
{return(siod_car(siod_car(siod_cdr(x))));}

siod_LISP siod_cadar(siod_LISP x)
{return(siod_car(siod_cdr(siod_car(x))));}

siod_LISP siod_caddr(siod_LISP x)
{return(siod_car(siod_cdr(siod_cdr(x))));}

siod_LISP siod_cdaar(siod_LISP x)
{return(siod_cdr(siod_car(siod_car(x))));}

siod_LISP siod_cdadr(siod_LISP x)
{return(siod_cdr(siod_car(siod_cdr(x))));}

siod_LISP siod_cddar(siod_LISP x)
{return(siod_cdr(siod_cdr(siod_car(x))));}

siod_LISP siod_cdddr(siod_LISP x)
{return(siod_cdr(siod_cdr(siod_cdr(x))));}

siod_LISP siod_ash(siod_LISP value,siod_LISP n)
{long m,k;
 m = siod_get_c_long(value);
 k = siod_get_c_long(n);
 if (k > 0)
   m = m << k;
 else
   m = m >> (-k);
 return(siod_flocons(m));}

siod_LISP siod_bitand(siod_LISP a,siod_LISP b)
{return(siod_flocons(siod_get_c_long(a) & siod_get_c_long(b)));}

siod_LISP siod_bitor(siod_LISP a,siod_LISP b)
{return(siod_flocons(siod_get_c_long(a) | siod_get_c_long(b)));}

siod_LISP siod_bitxor(siod_LISP a,siod_LISP b)
{return(siod_flocons(siod_get_c_long(a) ^ siod_get_c_long(b)));}

siod_LISP siod_bitnot(siod_LISP a)
{return(siod_flocons(~siod_get_c_long(a)));}

siod_LISP siod_leval_prog1(siod_LISP args,siod_LISP env)
{siod_LISP retval,l;
 retval = siod_leval(siod_car(args),env);
 for(l=siod_cdr(args);NNULLP(l);l=siod_cdr(l))
   siod_leval(siod_car(l),env);
 return(retval);}

siod_LISP siod_leval_cond(siod_LISP *pform,siod_LISP *penv)
{siod_LISP args,env,clause,value,next;
 args = siod_cdr(*pform);
 env = *penv;
 if NULLP(args)
   {*pform = NIL;
    return(NIL);}
 next = siod_cdr(args);
 while NNULLP(next)
   {clause = siod_car(args);
    value = siod_leval(siod_car(clause),env);
    if NNULLP(value)
      {clause = siod_cdr(clause);
       if NULLP(clause)
	 {*pform = value;
	  return(NIL);}
       else
	 {next = siod_cdr(clause);
	  while(NNULLP(next))
	    {siod_leval(siod_car(clause),env);
	     clause=next;
	     next=siod_cdr(next);}
	  *pform = siod_car(clause); 
	  return(sym_t);}}
    args = next;
    next = siod_cdr(next);}
 clause = siod_car(args);
 next = siod_cdr(clause);
 if NULLP(next)
   {*pform = siod_car(clause);
    return(sym_t);}
 value = siod_leval(siod_car(clause),env);
 if NULLP(value)
   {*pform = NIL;
    return(NIL);}
 clause = next;
 next = siod_cdr(next);
 while(NNULLP(next))
   {siod_leval(siod_car(clause),env);
    clause=next;
    next=siod_cdr(next);}
 *pform = siod_car(clause);
 return(sym_t);}

siod_LISP siod_lstrspn(siod_LISP str1,siod_LISP str2)
{return(siod_flocons(strspn(get_c_string(str1),get_c_string(str2))));}

siod_LISP siod_lstrcspn(siod_LISP str1,siod_LISP str2)
{return(siod_flocons(strcspn(get_c_string(str1),get_c_string(str2))));}

siod_LISP siod_substring_equal(siod_LISP str1,siod_LISP str2,siod_LISP start,siod_LISP end)
{char *cstr1,*cstr2;
 long len1,n,s,e;
 cstr1 = get_c_siod_string_dim(str1,&len1);
 cstr2 = get_c_siod_string_dim(str2,&n);
 s = NULLP(start) ? 0 : siod_get_c_long(start);
 e = NULLP(end) ? len1 : siod_get_c_long(end);
 if ((s < 0) || (s > e) || (e < 0) || (e > n) || ((e - s) != len1))
   return(NIL);
 return((memcmp(cstr1,&cstr2[s],e-s) == 0) ? siod_a_true_value() : NIL);}

#ifdef vms
/* int siod_strncasecmp(const char *s1, const char *s2, int n)
{int j,c1,c2;
 for(j=0;j<n;++j)
     {c1 = toupper(s1[j]);
    c2 = toupper(s2[j]);
    if ((c1 == 0) && (c2 == 0)) return(0);
    if (c1 == 0) return(-1);
    if (c2 == 0) return(1);
    if (c1 < c2) return(-1);
    if (c2 > c1) return(1);}
 return(0);}
*/
#endif

siod_LISP siod_substring_equalcase(siod_LISP str1,siod_LISP str2,siod_LISP start,siod_LISP end)
{char *cstr1,*cstr2;
 long len1,n,s,e;
 cstr1 = get_c_siod_string_dim(str1,&len1);
 cstr2 = get_c_siod_string_dim(str2,&n);
 s = NULLP(start) ? 0 : siod_get_c_long(start);
 e = NULLP(end) ? len1 : siod_get_c_long(end);
 if ((s < 0) || (s > e) || (e < 0) || (e > n) || ((e - s) != len1))
   return(NIL);
 return((strncasecmp(cstr1,&cstr2[s],e-s) == 0) ? siod_a_true_value() : NIL);}

siod_LISP siod_set_eval_history(siod_LISP len,siod_LISP circ)
{siod_LISP data;
 data = NULLP(len) ? len : siod_make_list(len,NIL);
 if NNULLP(circ)
   data = siod_nconc(data,data);
 siod_setvar(siod_cintern("*eval-history-ptr*"),data,NIL);
 siod_setvar(siod_cintern("*eval-history*"),data,NIL);
 return(len);}

static siod_LISP siod_parser_fasl(siod_LISP ignore)
{return(siod_closure(siod_listn(3,
		      NIL,
		      siod_cons_array(siod_flocons(100),NIL),
		      siod_flocons(0)),
		siod_leval(siod_cintern("siod_parser_fasl_hook"),NIL)));}

static siod_LISP siod_parser_fasl_hook(siod_LISP env,siod_LISP f)
{siod_LISP result;
 siod_setcar(env,f);
 result = siod_fast_read(env);
 if EQ(result,env)
   return(siod_get_eof_val());
 else
   return(result);}

void siod_init_subrs_a(void)
{siod_init_subr_2("aref",siod_aref1);
 siod_init_subr_3("aset",siod_aset1);
 siod_init_lsubr("string-append",siod_string_append);
 siod_init_lsubr("bytes-append",siod_bytes_append);
 siod_init_subr_1("string-length",siod_string_length);
 siod_init_subr_1("string-dimension",siod_string_dim);
 siod_init_subr_1("read-from-string",siod_read_from_string);
 siod_init_subr_3("print-to-string",siod_print_to_string);
 siod_init_subr_2("cons-array",siod_cons_array);
 siod_init_subr_2("sxhash",siod_sxhash);
 siod_init_subr_2("equal?",siod_equal);
 siod_init_subr_2("href",siod_href);
 siod_init_subr_3("hset",siod_hset);
 siod_init_subr_2("assoc",siod_assoc);
 siod_init_subr_2("assv",siod_assv);
 siod_init_subr_1("fast-read",siod_fast_read);
 siod_init_subr_2("fast-print",siod_fast_print);
 siod_init_subr_2("make-list",siod_make_list);
 siod_init_subr_2("fread",siod_lfread);
 siod_init_subr_2("fwrite",siod_lfwrite);
 siod_init_subr_1("fflush",siod_lfflush);
 siod_init_subr_1("length",siod_llength);
 siod_init_subr_4("number->string",siod_number2string);
 siod_init_subr_2("string->number",siod_string2number);
 siod_init_subr_3("substring",siod_substring);
 siod_init_subr_2("string-search",siod_string_search);
 siod_init_subr_1("string-trim",siod_string_trim);
 siod_init_subr_1("string-trim-left",siod_string_trim_left);
 siod_init_subr_1("string-trim-right",siod_string_trim_right);
 siod_init_subr_1("string-upcase",siod_string_upcase);
 siod_init_subr_1("string-downcase",siod_string_downcase);
 siod_init_subr_2("strcmp",siod_lstrcmp);
 siod_init_subr_2("strcat",siod_lstrcat);
 siod_init_subr_2("strcpy",siod_lstrcpy);
 siod_init_subr_2("strbreakup",siod_lstrbreakup);
 siod_init_subr_2("unbreakupstr",siod_lstrunbreakup);
 siod_init_subr_1("string?",siod_stringp);
 siod_gc_protect_sym(&sym_e,"e");
 siod_gc_protect_sym(&sym_f,"f");
 siod_gc_protect_sym(&sym_plists,"*plists*");
 siod_setvar(sym_plists,siod_ar_cons(tc_lisp_array,100,1),NIL);
 siod_init_subr_3("lref-default",siod_lref_default);
 siod_init_subr_3("larg-default",siod_larg_default);
 siod_init_subr_3("lkey-default",siod_lkey_default);
 siod_init_lsubr("list",siod_llist);
 siod_init_lsubr("writes",siod_writes);
 siod_init_subr_3("qsort",siod_lqsort);
 siod_init_subr_2("string-lessp",siod_string_lessp);
 siod_init_lsubr("mapcar",siod_mapcar);
 siod_init_subr_3("mapcar2",siod_mapcar2);
 siod_init_subr_2("mapcar1",siod_mapcar1);
 siod_init_subr_3("benchmark-siod_funcall1",siod_benchmark_siod_funcall1);
 siod_init_lsubr("benchmark-siod_funcall2",siod_benchmark_siod_funcall2);
 siod_init_subr_3("benchmark-eval",siod_benchmark_eval);
 siod_init_subr_2("fmod",siod_lfmod);
 siod_init_subr_2("subset",siod_lsubset);
 siod_init_subr_1("base64encode",siod_base64encode);
 siod_init_subr_1("base64decode",siod_base64decode);
 siod_init_subr_3("ass",siod_ass);
 siod_init_subr_2("append2",siod_append2);
 siod_init_lsubr("append",siod_append);
 siod_init_subr_5("fast-save",siod_fast_save);
 siod_init_subr_2("fast-load",siod_fast_load);
 siod_init_subr_3("swrite",siod_swrite);
 siod_init_subr_2("pow",siod_lpow);
 siod_init_subr_1("exp",siod_lexp);
 siod_init_subr_1("log",siod_llog);
 siod_init_subr_1("sin",siod_lsin);
 siod_init_subr_1("cos",siod_lcos);
 siod_init_subr_1("tan",siod_ltan);
 siod_init_subr_1("asin",siod_lasin);
 siod_init_subr_1("acos",siod_lacos);
 siod_init_subr_1("atan",siod_latan);
 siod_init_subr_2("atan2",siod_latan2);
 siod_init_subr_1("sinh",siod_lsinh);
 siod_init_subr_1("cosh",siod_lcosh);
 siod_init_subr_1("tanh",siod_ltanh);
 siod_init_subr_1("asinh",siod_lasinh);
 siod_init_subr_1("acosh",siod_lacosh);
 siod_init_subr_1("atanh",siod_latanh);
 siod_init_subr_1("typeof",siod_ltypeof);
 siod_init_subr_1("caaar",siod_caaar);
 siod_init_subr_1("caadr",siod_caadr);
 siod_init_subr_1("cadar",siod_cadar);
 siod_init_subr_1("caddr",siod_caddr);
 siod_init_subr_1("cdaar",siod_cdaar);
 siod_init_subr_1("cdadr",siod_cdadr);
 siod_init_subr_1("cddar",siod_cddar);
 siod_init_subr_1("cdddr",siod_cdddr);
 siod_setvar(siod_cintern("*pi*"),siod_flocons(atan(1.0)*4),NIL);
 siod_init_base64_table();
 siod_init_subr_1("array->siod_hexstr",siod_hexstr);
 siod_init_subr_1("hexstr->bytes",siod_hexstr2bytes);
 siod_init_subr_3("ass",siod_ass);
 siod_init_subr_2("bit-and",siod_bitand);
 siod_init_subr_2("bit-or",siod_bitor);
 siod_init_subr_2("bit-xor",siod_bitxor);
 siod_init_subr_1("bit-not",siod_bitnot);
 siod_init_msubr("cond",siod_leval_cond);
 siod_init_fsubr("prog1",siod_leval_prog1);
 siod_init_subr_2("strspn",siod_lstrspn);
 siod_init_subr_2("strcspn",siod_lstrcspn);
 siod_init_subr_4("substring-equal?",siod_substring_equal);
 siod_init_subr_4("substring-equalcase?",siod_substring_equalcase);
 siod_init_subr_1("butlast",siod_butlast);
 siod_init_subr_2("ash",siod_ash);
 siod_init_subr_2("get",siod_getprop);
 siod_init_subr_3("setprop",siod_setprop);
 siod_init_subr_3("putprop",siod_putprop);
 siod_init_subr_1("last",siod_last);
 siod_init_subr_2("memq",siod_memq);
 siod_init_subr_2("memv",siod_memv);
 siod_init_subr_2("member",siod_member);
 siod_init_subr_2("nth",siod_nth);
 siod_init_subr_2("nconc",siod_nconc);
 siod_init_subr_2("set-eval-history",siod_set_eval_history);
 siod_init_subr_1("parser_fasl",siod_parser_fasl);
 siod_setvar(siod_cintern("*siod_parser_fasl.scm-siod_loaded*"),siod_a_true_value(),NIL);
 siod_init_subr_2("parser_fasl_hook",siod_parser_fasl_hook);
 siod_init_sliba_version();}

