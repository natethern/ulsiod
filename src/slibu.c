/*-*-mode:c-*-*/

/* DEC-94 George Carrette. Additional lisp util subrs,
   many of them depending on operating system calls.
   Note that I have avoided more than one nesting of conditional compilation,
   and the use of the else clause, in hopes of preserving some readability.
   For better or worse I avoided gnu autoconfigure because it was complex siod_required
   scripts nearly as big as this source file. Meanwhile there is some ANSI POSIX
   convergence going on.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#define __USE_XOPEN 1
#include <time.h>
#undef __USE_XOPEN
#include <errno.h>
#include <stdarg.h>

#if defined(unix)
#include <crypt.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <grp.h>
#include <utime.h>
#include <sys/fcntl.h>
#endif

#if defined(__osf__) || defined(sun)
#include <sys/mode.h>
#endif

#if defined(__osf__) || defined(SUN5)
#include <fnmatch.h>
#endif

#if defined(__osf__)
#include <rld_interface.h>
#endif

#if defined(hpux)
#include <dl.h>
#endif

#if defined(__osf__) || defined(sun) || defined(linux) || defined(sgi)
#include <dlfcn.h>
#endif

#if defined(sun)
#include <crypt.h>
#include <limits.h>
#include <sys/mkdev.h>
#include <fcntl.h>
#endif

#if defined(linux) && defined(PPC)
/* I know, this should be defined(NEED_CRYPT_H) */
#include <crypt.h>
#endif

#if defined(sgi)
#include <limits.h>
#endif

#if defined(hpux)
#define PATH_MAX MAXPATHLEN
#endif

#if defined(VMS)
#include <unixlib.h>
#include <stat.h>
#include <ssdef.h>
#include <descrip.h>
#include <lib$routines.h>
#include <descrip.h>
#include <ssdef.h>
#include <iodef.h>
#include <lnmdef.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <direct.h>
#endif

#include "ulsiod.h"
#include "siodp.h"
#include "md5.h"

static void siod_init_slibu_version(void)
{siod_setvar(siod_cintern("*slibu-version*"),
	siod_cintern("$Id: slibu.c,v 1.5 2001/02/01 16:03:29 gjcarret Exp gjcarret $"),
	NIL);}


siod_LISP sym_channels = NIL;
long tc_opendir = 0;

char *ld_library_path_env = "LD_LIBRARY_PATH";

#ifdef VMS
char *strdup(char *in)
{char *r;
 r = (char *) malloc(strlen(in)+1);
 strcpy(r,in);
 return(r);}
#endif

siod_LISP siod_lsystem(siod_LISP args)
{int retval;
 long iflag;
 iflag = siod_no_interrupt(1);
 retval = system(get_c_string(siod_string_append(args)));
 siod_no_interrupt(iflag);
 if (retval < 0)
   return(siod_cons(siod_flocons(retval),siod_llast_c_errmsg(-1)));
 else
   return(siod_flocons(retval));}

#ifndef WIN32
siod_LISP siod_lgetuid(void)
{return(siod_flocons(getuid()));}

siod_LISP siod_lgetgid(void)
{return(siod_flocons(getgid()));}
#endif

#ifdef unix

siod_LISP siod_lcrypt(siod_LISP key,siod_LISP salt)
{char *result;
 if ((result = crypt(get_c_string(key),get_c_string(salt))))
   return(siod_strcons(strlen(result),result));
 else
   return(NIL);}

#endif

#if defined(unix) || defined(WIN32)

#if defined(WIN32)
#define getcwd _getcwd
#define PATH_MAX _MAX_PATH
#endif

siod_LISP siod_lgetcwd(void)
{char path[PATH_MAX+1];
 if (getcwd(path,sizeof(path)))
   return(siod_strcons(strlen(path),path));
 else
   return(err("getcwd",siod_llast_c_errmsg(-1)));}

#endif

#ifdef unix


siod_LISP siod_ldecode_pwent(struct passwd *p)
{return(siod_symalist(
		 "name",siod_strcons(strlen(p->pw_name),p->pw_name),
		 "passwd",siod_strcons(strlen(p->pw_passwd),p->pw_passwd),
		 "uid",siod_flocons(p->pw_uid),
		 "gid",siod_flocons(p->pw_gid),
		 "dir",siod_strcons(strlen(p->pw_dir),p->pw_dir),
		 "gecos",siod_strcons(strlen(p->pw_gecos),p->pw_gecos),
#if defined(__osf__) || defined(hpux) || defined(sun)
		 "comment",siod_strcons(strlen(p->pw_comment),p->pw_comment),
#endif
#if defined(hpux) || defined(sun)
		 "age",siod_strcons(strlen(p->pw_age),p->pw_age),
#endif
#if defined(__osf__)
		 "quota",siod_flocons(p->pw_quota),
#endif
		 "shell",siod_strcons(strlen(p->pw_shell),p->pw_shell),
		 NULL));}

static char *strfield(char *name,siod_LISP alist)
{siod_LISP value,key = siod_rintern(name);
 if NULLP(value = siod_assq(key,alist))
   return("");
 return(get_c_string(siod_cdr(value)));}

static long siod_longfield(char *name,siod_LISP alist)
{siod_LISP value,key = siod_rintern(name);
 if NULLP(value = siod_assq(key,alist))
   return(0);
 return(siod_get_c_long(siod_cdr(value)));}
 
void siod_lencode_pwent(siod_LISP alist,struct passwd *p)
{p->pw_name = strfield("name",alist);
 p->pw_passwd = strfield("passwd",alist);
 p->pw_uid = siod_longfield("uid",alist);
 p->pw_gid = siod_longfield("gid",alist);
 p->pw_dir = strfield("dir",alist);
 p->pw_gecos = strfield("gecos",alist);
#if defined(__osf__) || defined(hpux) || defined(sun)
 p->pw_comment = strfield("comment",alist);
#endif
#if defined(hpux) || defined(sun)
 p->pw_age = strfield("age",alist);
#endif
#if defined(__osf__)
 p->pw_quota = siod_longfield("quota",alist);
#endif
 p->pw_shell = strfield("shell",alist);}

siod_LISP siod_lgetpwuid(siod_LISP luid)
{int iflag;
 uid_t uid;
 struct passwd *p;
 siod_LISP result = NIL;
 uid = siod_get_c_long(luid);
 iflag = siod_no_interrupt(1);
 if ((p = getpwuid(uid)))
   result = siod_ldecode_pwent(p);
 siod_no_interrupt(iflag);
 return(result);}

siod_LISP siod_lgetpwnam(siod_LISP nam)
{int iflag;
 struct passwd *p;
 siod_LISP result = NIL;
 iflag = siod_no_interrupt(1);
 if ((p = getpwnam(get_c_string(nam))))
   result = siod_ldecode_pwent(p);
 siod_no_interrupt(iflag);
 return(result);}

siod_LISP siod_lgetpwent(void)
{int iflag;
 siod_LISP result = NIL;
 struct passwd *p;
 iflag = siod_no_interrupt(1);
 if ((p = getpwent()))
   result = siod_ldecode_pwent(p);
 siod_no_interrupt(iflag);
 return(result);}

siod_LISP siod_lsetpwent(void)
{int iflag = siod_no_interrupt(1);
 setpwent();
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_lendpwent(void)
{int iflag = siod_no_interrupt(1);
 endpwent();
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_lsetuid(siod_LISP n)
{uid_t uid;
 uid = (uid_t) siod_get_c_long(n);
 if (setuid(uid))
   return(err("setuid",siod_llast_c_errmsg(-1)));
 else
   return(NIL);}

siod_LISP siod_lseteuid(siod_LISP n)
{uid_t uid;
 uid = (uid_t) siod_get_c_long(n);
 if (seteuid(uid))
   return(err("seteuid",siod_llast_c_errmsg(-1)));
 else
   return(NIL);}

siod_LISP siod_lgeteuid(void)
{return(siod_flocons(geteuid()));}

#if defined(__osf__)
siod_LISP siod_lsetpwfile(siod_LISP fname)
{int iflag = siod_no_interrupt(1);
 setpwfile(get_c_string(fname));
 siod_no_interrupt(iflag);
 return(NIL);}
#endif

siod_LISP siod_lputpwent(siod_LISP alist,siod_LISP file)
{int iflag = siod_no_interrupt(1);
 int status;
 struct passwd p;
 siod_lencode_pwent(alist,&p);
 status = putpwent(&p,siod_get_c_file(file,NULL));
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_laccess_problem(siod_LISP lfname,siod_LISP lacc)
{char *fname = get_c_string(lfname);
 char *acc = get_c_string(lacc),*p;
 int amode = 0,iflag = siod_no_interrupt(1),retval;
 for(p=acc;*p;++p)
   switch(*p)
     {case 'r':
	amode |= R_OK;
	break;
      case 'w':
	amode |= W_OK;
	break;
      case 'x':
	amode |= X_OK;
	break;
      case 'f':
	amode |= F_OK;
	break;
      default:
	err("bad access mode",lacc);}
 retval = access(fname,amode);
 siod_no_interrupt(iflag);
 if (retval < 0)
   return(siod_llast_c_errmsg(-1));
 else
   return(NIL);}

siod_LISP siod_lsymlink(siod_LISP p1,siod_LISP p2)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (symlink(get_c_string(p1),get_c_string(p2)))
   return(err("symlink",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_llink(siod_LISP p1,siod_LISP p2)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (link(get_c_string(p1),get_c_string(p2)))
   return(err("link",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_lunlink(siod_LISP p)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (unlink(get_c_string(p)))
   return(err("unlink",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_lrmdir(siod_LISP p)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (rmdir(get_c_string(p)))
   return(err("rmdir",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_lmkdir(siod_LISP p,siod_LISP m)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (mkdir(get_c_string(p),siod_get_c_long(m)))
   return(err("mkdir",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_lreadlink(siod_LISP p)
{long iflag;
 char buff[PATH_MAX+1];
 int size;
 iflag = siod_no_interrupt(1);
 if ((size = readlink(get_c_string(p),buff,sizeof(buff))) < 0)
   return(err("readlink",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(siod_strcons(size,buff));}

siod_LISP siod_lrename(siod_LISP p1,siod_LISP p2)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (rename(get_c_string(p1),get_c_string(p2)))
   return(err("rename",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}

#endif

siod_LISP siod_lrandom(siod_LISP n)
{int res;
#if defined(hpux) || defined(vms) || defined(sun) || defined(sgi) || defined(WIN32)
 res = rand();
#endif
#if defined(__osf__) || defined(linux)
 res = random();
#endif
 return(siod_flocons(NNULLP(n) ? res % siod_get_c_long(n) : res));}

siod_LISP siod_lsrandom(siod_LISP n)
{long seed;
 seed = siod_get_c_long(n);
#if defined(hpux) || defined(vms) || defined(sun) || defined(sgi) || defined(WIN32)
 srand(seed);
#endif
#if defined(__osf__) || defined(linux)
 srandom(seed);
#endif
 return(NIL);}

#ifdef unix

siod_LISP siod_lfork(void)
{int iflag;
 pid_t pid;
 iflag = siod_no_interrupt(1);
 pid = fork();
 if (pid == 0)
   {siod_no_interrupt(iflag);
    return(NIL);}
 if (pid == -1)
   return(err("fork",siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(siod_flocons(pid));}

#endif

char **list2char(siod_LISP *safe,siod_LISP v)
{char **x,*tmp;
 long j,n;
 siod_LISP l;
 n = siod_get_c_long(siod_llength(v));
 *safe = siod_cons(siod_mallocl(&x,sizeof(char *) * (n + 1)),*safe);
 for(l=v,j=0;j<n;l=siod_cdr(l),++j)
   {tmp = get_c_string(siod_car(l));
    *safe = siod_cons(siod_mallocl(&x[j],strlen(tmp)+1),*safe);
    strcpy(x[j],tmp);}
 x[n] = NULL;
 return(x);}

#ifdef unix

siod_LISP siod_lexec(siod_LISP path,siod_LISP args,siod_LISP env)
{int iflag;
 char **argv = NULL, **envp = NULL;
 siod_LISP gcsafe=NIL;
 iflag = siod_no_interrupt(1);
 argv = list2char(&gcsafe,args);
 if NNULLP(env)
   envp = list2char(&gcsafe,env);
 if (envp)
   execve(get_c_string(path),argv,envp);
 else
   execv(get_c_string(path),argv);
 siod_no_interrupt(iflag);
 return(err("exec",siod_llast_c_errmsg(-1)));}

siod_LISP siod_lnice(siod_LISP val)
{int iflag,n;
 n = siod_get_c_long(val);
 iflag = siod_no_interrupt(1);
 n = nice(n);
 if (n == -1)
   err("nice",siod_llast_c_errmsg(-1));
 siod_no_interrupt(iflag);
 return(siod_flocons(n));}

#endif

int siod_assemble_options(siod_LISP l, ...)
{int result = 0,val,noptions,nmask = 0;
 siod_LISP lsym,lp = NIL;
 char *sym;
 va_list syms;
 if NULLP(l) return(0);
 noptions = CONSP(l) ? siod_get_c_long(siod_llength(l)) : -1;
 va_start(syms,l);
 while((sym = va_arg(syms,char *)))
   {val = va_arg(syms,int);
    lsym = siod_cintern(sym);
    if (EQ(l,lsym) || (CONSP(l) && NNULLP(lp = siod_memq(lsym,l))))
      {result |= val;
       if (noptions > 0)
	 nmask = nmask | (1 << (noptions - siod_get_c_long(siod_llength(lp))));
       else
	 noptions = -2;}}
 va_end(syms);
 if ((noptions == -1) ||
     ((noptions > 0) && (nmask != ((1 << noptions) - 1))))
   err("contains undefined options",l);
 return(result);}

#ifdef unix

siod_LISP siod_lwait(siod_LISP lpid,siod_LISP loptions)
{pid_t pid,ret;
 int iflag,status = 0,options;
 pid = NULLP(lpid) ? -1 : siod_get_c_long(lpid);
 options = siod_assemble_options(loptions,
#ifdef WCONTINUED
			    "WCONTINUED",WCONTINUED,
#endif
#ifdef WNOWAIT
			    "WNOWAIT",WNOWAIT,
#endif
			    "WNOHANG",WNOHANG,
			    "WUNTRACED",WUNTRACED,
			    NULL);
 iflag = siod_no_interrupt(1); 
 ret = waitpid(pid,&status,options);
 siod_no_interrupt(iflag);
 if (ret == 0)
   return(NIL);
 else if (ret == -1)
   return(err("wait",siod_llast_c_errmsg(-1)));
 else
   /* should do more decoding on the status */
   return(siod_cons(siod_flocons(ret),siod_cons(siod_flocons(status),NIL)));}

siod_LISP siod_lkill(siod_LISP pid,siod_LISP sig)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (kill(siod_get_c_long(pid),
	  NULLP(sig) ? SIGKILL : siod_get_c_long(sig)))
   err("kill",siod_llast_c_errmsg(-1));
 else
   siod_no_interrupt(iflag);
 return(NIL);}

#endif

siod_LISP siod_lgetpid(void)
{return(siod_flocons(getpid()));}

#ifdef unix
siod_LISP siod_lgetpgrp(void)
{return(siod_flocons(getpgrp()));}

siod_LISP siod_lsetpgid(siod_LISP pid,siod_LISP pgid)
{if (setpgid(siod_get_c_long(pid),siod_get_c_long(pgid)))
  return(err("setpgid",siod_llast_c_errmsg(-1)));
 else
  return(NIL);}

siod_LISP siod_lgetgrgid(siod_LISP n)
{gid_t gid;
 struct group *gr;
 long iflag,j;
 siod_LISP result = NIL;
 gid = siod_get_c_long(n);
 iflag = siod_no_interrupt(1);
 if ((gr = getgrgid(gid)))
   {result = siod_cons(siod_strcons(strlen(gr->gr_name),gr->gr_name),result);
    for(j=0;gr->gr_mem[j];++j)
      result = siod_cons(siod_strcons(strlen(gr->gr_mem[j]),gr->gr_mem[j]),result);
    result = siod_nreverse(result);}
 siod_no_interrupt(iflag);
 return(result);}

#endif

#ifndef WIN32
siod_LISP siod_lgetppid(void)
{return(siod_flocons(getppid()));}
#endif

siod_LISP siod_lmemref_byte(siod_LISP addr)
{unsigned char *ptr = (unsigned char *) siod_get_c_long(addr);
 return(siod_flocons(*ptr));}

siod_LISP siod_lexit(siod_LISP val)
{int iflag = siod_no_interrupt(1);
 exit(siod_get_c_long(val));
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_ltrunc(siod_LISP x)
{long i;
 if NFLONUMP(x) err("wta to trunc",x);
 i = (long) FLONM(x);
 return(siod_flocons((double) i));}

#ifdef unix
siod_LISP siod_lputenv(siod_LISP lstr)
{char *orig,*cpy;
 orig = get_c_string(lstr);
 /* unix putenv keeps a pointer to the string we pass,
    therefore we must make a fresh copy, which is memory leaky. */
 cpy = (char *) must_malloc(strlen(orig)+1);
 strcpy(cpy,orig);
  if (putenv(cpy))
   return(err("putenv",siod_llast_c_errmsg(-1)));
 else
   return(NIL);}
#endif

MD5_CTX * siod_get_md5_ctx(siod_LISP a)
{if (TYPEP(a,tc_byte_array) &&
     (a->storage_as.string.dim == sizeof(MD5_CTX)))
   return((MD5_CTX *)a->storage_as.string.data);
 else
   {err("not an MD5_CTX array",a);
    return(NULL);}}

siod_LISP siod_md5_init(void)
{siod_LISP a = siod_ar_cons(tc_byte_array,sizeof(MD5_CTX),1);
 MD5Init(siod_get_md5_ctx(a));
 return(a);}

void siod_md5_update_from_file(MD5_CTX *ctx,FILE *f,unsigned char *buff,long dim)
{size_t len;
 while((len = fread(buff,sizeof(buff[0]),dim,f)))
   MD5Update(ctx,buff,len);}

siod_LISP siod_md5_update(siod_LISP ctx,siod_LISP str,siod_LISP len)
{char *buffer; long dim,n;
 buffer = get_c_siod_string_dim(str,&dim);
 if TYPEP(len,tc_c_file)
   {siod_md5_update_from_file(siod_get_md5_ctx(ctx), siod_get_c_file(len,NULL),
			 (unsigned char *)buffer,dim);
    return(NIL);}
 else if NULLP(len)
   n = dim;
 else
   {n = siod_get_c_long(len);
    if ((n < 0) || (n > dim)) err("invalid length for string",len);}
 MD5Update(siod_get_md5_ctx(ctx),(unsigned char *)buffer,n);
 return(NIL);}

siod_LISP siod_md5_final(siod_LISP ctx)
{siod_LISP result = siod_ar_cons(tc_byte_array,16,0);
 MD5Final((unsigned char *) result->storage_as.string.data,
	  siod_get_md5_ctx(ctx));
 return(result);}

#if defined(__osf__) || defined(sun)

void siod_handle_sigxcpu(int sig)
{struct rlimit x;
 if (getrlimit(RLIMIT_CPU,&x))
   {errjmp_ok = 0;
    err("getrlimit",siod_llast_c_errmsg(-1));}
 if (x.rlim_cur >= x.rlim_max)
   {errjmp_ok = 0;
    err("hard cpu limit exceded",NIL);}
 if (nointerrupt == 1)
   interrupt_differed = 1;
 else
   err("cpu limit exceded",NIL);}

siod_LISP siod_cpu_usage_limits(siod_LISP soft,siod_LISP hard)
{struct rlimit x;
 if (NULLP(soft) && NULLP(hard))
   {if (getrlimit(RLIMIT_CPU,&x))
      return(err("getrlimit",siod_llast_c_errmsg(-1)));
    else
      return(siod_listn(2,siod_flocons(x.rlim_cur),siod_flocons(x.rlim_max)));}
 else
   {x.rlim_cur = siod_get_c_long(soft);
    x.rlim_max = siod_get_c_long(hard);
    signal(SIGXCPU,siod_handle_sigxcpu);
    if (setrlimit(RLIMIT_CPU,&x))
      return(err("setrlimit",siod_llast_c_errmsg(-1)));
    else
      return(NIL);}}

#endif

#if defined(unix)

static int siod_handle_sigalrm_flag = 0;

void siod_handle_sigalrm(int sig)
{if (nointerrupt == 1)
  {if (siod_handle_sigalrm_flag)
    /* If we were inside a system call then it would be
       interrupted even if we take no action here.
       But sometimes we want to be really sure of signalling
       an error, hence the flag. */
    interrupt_differed = 1;}
 else
   err("alarm signal",NIL);}

siod_LISP siod_lalarm(siod_LISP seconds,siod_LISP flag)
{long iflag;
 int retval;
 iflag = siod_no_interrupt(1);
 signal(SIGALRM,siod_handle_sigalrm);
 siod_handle_sigalrm_flag = NULLP(flag) ? 0 : 1;
 retval = alarm(siod_get_c_long(seconds));
 siod_no_interrupt(iflag);
 return(siod_flocons(retval));}

#endif


#if defined(__osf__) || defined(SUN5)

#define TV_FRAC(x) (((double)x.tv_usec) * 1.0e-6)

#ifdef SUN5
int getrusage(int,struct rusage *);
#endif

siod_LISP siod_current_resource_usage(siod_LISP kind)
{struct rusage u;
 int code;
 if (NULLP(kind) || EQ(siod_cintern("SELF"),kind))
   code = RUSAGE_SELF;
 else if EQ(siod_cintern("CHILDREN"),kind)
   code = RUSAGE_CHILDREN;
 else
   return(err("unknown rusage",kind));
 if (getrusage(code,&u))
   return(err("getrusage",siod_llast_c_errmsg(-1)));
 return(siod_symalist("utime",siod_flocons(((double)u.ru_utime.tv_sec) +
				 TV_FRAC(u.ru_utime)),
		 "stime",siod_flocons(((double)u.ru_stime.tv_sec) +
				 TV_FRAC(u.ru_stime)),
		 "maxrss",siod_flocons(u.ru_maxrss),
		 "ixrss",siod_flocons(u.ru_ixrss),
		 "idrss",siod_flocons(u.ru_idrss),
		 "isrss",siod_flocons(u.ru_isrss),
		 "minflt",siod_flocons(u.ru_minflt),
		 "majflt",siod_flocons(u.ru_majflt),
		 "nswap",siod_flocons(u.ru_nswap),
		 "inblock",siod_flocons(u.ru_inblock),
		 "oublock",siod_flocons(u.ru_oublock),
		 "msgsnd",siod_flocons(u.ru_msgsnd),
		 "msgrcv",siod_flocons(u.ru_msgrcv),
		 "nsignals",siod_flocons(u.ru_nsignals),
		 "nvcsw",siod_flocons(u.ru_nvcsw),
		 "nivcsw",siod_flocons(u.ru_nivcsw),
		 NULL));}

#endif

#ifdef unix

siod_LISP siod_l_opendir(siod_LISP name)
{long iflag;
 siod_LISP value;
 DIR *d;
 iflag = siod_no_interrupt(1);
 value = siod_cons(NIL,NIL);
 if (!(d = opendir(get_c_string(name))))
   return(err("opendir",siod_llast_c_errmsg(-1)));
 value->type = tc_opendir;
 CAR(value) = (siod_LISP) d;
 siod_no_interrupt(iflag);
 return(value);}

DIR *get_opendir(siod_LISP v,long oflag)
{if NTYPEP(v,tc_opendir) err("not an opendir",v);
 if NULLP(CAR(v))
   {if (oflag) err("opendir not open",v);
    return(NULL);}
 return((DIR *)CAR(v));}

siod_LISP siod_l_closedir(siod_LISP v)
{long iflag,old_errno;
 DIR *d;
 iflag = siod_no_interrupt(1);
 d = get_opendir(v,1);
 old_errno = errno;
 CAR(v) = NIL;
 if (closedir(d))
   return(err("closedir",siod_llast_c_errmsg(old_errno)));
 siod_no_interrupt(iflag);
 return(NIL);}

void  siod_opendir_gc_free(siod_LISP v)
{DIR *d;
 if ((d = get_opendir(v,0)))
   closedir(d);}

siod_LISP siod_l_readdir(siod_LISP v)
{long iflag,namlen;
 DIR *d;
 struct dirent *r;
 d = get_opendir(v,1);
 iflag = siod_no_interrupt(1);
 r = readdir(d);
 siod_no_interrupt(iflag);
 if (!r) return(NIL);
#if defined(sun) || defined(sgi) || defined(linux)
 namlen = siod_safe_strlen(r->d_name,r->d_reclen);
#else
 namlen = r->d_namlen;
#endif
 return(siod_strcons(namlen,r->d_name));}

void siod_opendir_prin1(siod_LISP ptr,struct gen_printio *f)
{char buffer[256];
 sprintf(buffer,"#<OPENDIR %p>",get_opendir(ptr,0));
 siod_gput_st(f,buffer);}

#endif

#ifdef WIN32

typedef struct
{long count;
 HANDLE h;
 WIN32_FIND_DATA s;} DIR;

siod_LISP siod_llast_win32_errmsg(DWORD status)
{DWORD len,msgcode;
 char buffer[256];
 msgcode = (status == 0) ? GetLastError() : status;
 len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
	                 FORMAT_MESSAGE_IGNORE_INSERTS |
					 FORMAT_MESSAGE_MAX_WIDTH_MASK,
	                 0,
					 msgcode,
					 0, /* what language? */
					 buffer,
					 sizeof(buffer),
					 NULL);
 if (len)
   return(siod_strcons(len,buffer));
 else
   return(siod_flocons(msgcode));}

siod_LISP siod_l_opendir(siod_LISP name)
{long iflag;
 siod_LISP value;
 DIR *d;
 iflag = siod_no_interrupt(1);
 value = siod_cons(NIL,NIL);
 d = (DIR *) must_malloc(sizeof(DIR));
 d->h = INVALID_HANDLE_VALUE;
 value->type = (short) tc_opendir;
 d->count = 0;
 CAR(value) = (siod_LISP) d;
 if ((d->h = FindFirstFile(get_c_string(name),&d->s)) == INVALID_HANDLE_VALUE)
   return(err("FindFirstFile",siod_llast_win32_errmsg(0)));
 siod_no_interrupt(iflag);
 return(value);}

DIR *get_opendir(siod_LISP v,long oflag)
{if NTYPEP(v,tc_opendir) err("not an opendir",v);
 if NULLP(CAR(v))
   {if (oflag) err("opendir not open",v);
    return(NULL);}
 return((DIR *)CAR(v));}

siod_LISP siod_l_closedir(siod_LISP v)
{long iflag;
 DIR *d;
 HANDLE h;
 iflag = siod_no_interrupt(1);
 d = get_opendir(v,1);
 CAR(v) = NIL;
 h = d->h;
 free(d);
 if ((h != INVALID_HANDLE_VALUE) && !FindClose(h))
   return(err("closedir",siod_llast_win32_errmsg(0)));
 siod_no_interrupt(iflag);
 return(NIL);}

void  siod_opendir_gc_free(siod_LISP v)
{DIR *d;
 if ((d = get_opendir(v,0)))
   {FindClose(d->h);
    free(d);
	CAR(v) = NIL;}}

siod_LISP siod_l_readdir(siod_LISP v)
{long iflag;
 DIR *d;
 d = get_opendir(v,1);
 iflag = siod_no_interrupt(1);
 if (d->count > 0)
   if (!FindNextFile(d->h,&d->s))
	 if (GetLastError() == ERROR_NO_MORE_FILES)
	   {siod_no_interrupt(1);
        return(NIL);}
 ++d->count;
 siod_no_interrupt(iflag);
 return(siod_strcons(-1,d->s.cFileName));}

void siod_opendir_prin1(siod_LISP ptr,struct gen_printio *f)
{char buffer[256];
 sprintf(buffer,"#<OPENDIR %p>",get_opendir(ptr,0));
 siod_gput_st(f,buffer);}

#endif

siod_LISP siod_file_times(siod_LISP fname)
{struct stat st;
 int iflag,ret;
 iflag = siod_no_interrupt(1);
 ret = stat(get_c_string(fname),&st);
 siod_no_interrupt(iflag);
 if (ret)
   return(NIL);
 else
   return(siod_cons(siod_flocons(st.st_ctime),
	       siod_cons(siod_flocons(st.st_mtime),NIL)));}

#if defined(unix) || defined(WIN32)

#if defined(unix)
siod_LISP siod_decode_st_moden(mode_t mode)
{siod_LISP ret = NIL;
 if (mode & S_ISUID) ret = siod_cons(siod_cintern("SUID"),ret);
 if (mode & S_ISGID) ret = siod_cons(siod_cintern("SGID"),ret);
 if (mode & S_IRUSR) ret = siod_cons(siod_cintern("RUSR"),ret);
 if (mode & S_IWUSR) ret = siod_cons(siod_cintern("WUSR"),ret);
 if (mode & S_IXUSR) ret = siod_cons(siod_cintern("XUSR"),ret);
 if (mode & S_IRGRP) ret = siod_cons(siod_cintern("RGRP"),ret);
 if (mode & S_IWGRP) ret = siod_cons(siod_cintern("WGRP"),ret);
 if (mode & S_IXGRP) ret = siod_cons(siod_cintern("XGRP"),ret);
 if (mode & S_IROTH) ret = siod_cons(siod_cintern("ROTH"),ret);
 if (mode & S_IWOTH) ret = siod_cons(siod_cintern("WOTH"),ret);
 if (mode & S_IXOTH) ret = siod_cons(siod_cintern("XOTH"),ret);
 if (S_ISFIFO(mode)) ret = siod_cons(siod_cintern("FIFO"),ret);
 if (S_ISDIR(mode)) ret = siod_cons(siod_cintern("DIR"),ret);
 if (S_ISCHR(mode)) ret = siod_cons(siod_cintern("CHR"),ret);
 if (S_ISBLK(mode)) ret = siod_cons(siod_cintern("BLK"),ret);
 if (S_ISREG(mode)) ret = siod_cons(siod_cintern("REG"),ret);
 if (S_ISLNK(mode)) ret = siod_cons(siod_cintern("LNK"),ret);
 if (S_ISSOCK(mode)) ret = siod_cons(siod_cintern("SOCK"),ret);
 return(ret);}

siod_LISP siod_encode_st_mode(siod_LISP l)
{return(siod_flocons(siod_assemble_options(l,
				 "SUID",S_ISUID,
				 "SGID",S_ISGID,
				 "RUSR",S_IRUSR,
				 "WUSR",S_IWUSR,
				 "XUSR",S_IXUSR,
				 "RGRP",S_IRGRP,
				 "WGRP",S_IWGRP,
				 "XGRP",S_IXGRP,
				 "ROTH",S_IROTH,
				 "WOTH",S_IWOTH,
				 "XOTH",S_IXOTH,
				 NULL)));}
#endif

#ifdef WIN32

siod_LISP siod_decode_st_moden(int mode)
{siod_LISP ret = NIL;
 if (mode & _S_IREAD) ret = siod_cons(siod_cintern("RUSR"),ret);
 if (mode & _S_IWRITE) ret = siod_cons(siod_cintern("WUSR"),ret);
 if (mode & _S_IEXEC) ret = siod_cons(siod_cintern("XUSR"),ret);
 if (mode & _S_IFDIR) ret = siod_cons(siod_cintern("DIR"),ret);
 if (mode & _S_IFCHR) ret = siod_cons(siod_cintern("CHR"),ret);
 if (mode & _S_IFREG) ret = siod_cons(siod_cintern("REG"),ret);
 return(ret);}

siod_LISP siod_encode_st_mode(siod_LISP l)
{return(siod_flocons(siod_assemble_options(l,
				 "RUSR",_S_IREAD,
				 "WUSR",_S_IWRITE,
				 "XUSR",_S_IEXEC,
				 NULL)));}
#endif

siod_LISP siod_decode_st_mode(siod_LISP value)
{return(siod_decode_st_moden(siod_get_c_long(value)));}

siod_LISP siod_decode_stat(struct stat *s)
{return(siod_symalist("dev",siod_flocons(s->st_dev),
		 "ino",siod_flocons(s->st_ino),
		 "mode",siod_decode_st_moden(s->st_mode),
		 "nlink",siod_flocons(s->st_nlink),
		 "uid",siod_flocons(s->st_uid),
		 "gid",siod_flocons(s->st_gid),
		 "rdev",siod_flocons(s->st_rdev),
		 "size",siod_flocons(s->st_size),
		 "atime",siod_flocons(s->st_atime),
		 "mtime",siod_flocons(s->st_mtime),
		 "ctime",siod_flocons(s->st_ctime),
#if defined(unix)
		 "blksize",siod_flocons(s->st_blksize),
		 "blocks",siod_flocons(s->st_blocks),
#endif
#if defined(__osf__)
		 "flags",siod_flocons(s->st_flags),
		 "gen",siod_flocons(s->st_gen),
#endif
		 NULL));}
	      

siod_LISP siod_g_stat(siod_LISP fname,int (*fcn)(const char *,struct stat *))
{struct stat st;
 int iflag,ret;
 iflag = siod_no_interrupt(1);
 memset(&st,0,sizeof(struct stat));
 ret = (*fcn)(get_c_string(fname),&st);
 siod_no_interrupt(iflag);
 if (ret)
   return(NIL);
 else
   return(siod_decode_stat(&st));}

siod_LISP siod_l_stat(siod_LISP fname)
{return(siod_g_stat(fname,stat));}

siod_LISP siod_l_fstat(siod_LISP f)
{struct stat st;
 int iflag,ret;
 iflag = siod_no_interrupt(1);
 ret = fstat(fileno(siod_get_c_file(f,NULL)),&st);
 siod_no_interrupt(iflag);
 if (ret)
   return(NIL);
 else
   return(siod_decode_stat(&st));}

#ifdef unix
siod_LISP siod_l_lstat(siod_LISP fname)
{return(siod_g_stat(fname,lstat));}
#endif

#if defined(__osf__) || defined(SUN5)

siod_LISP siod_l_fnmatch(siod_LISP pat,siod_LISP str,siod_LISP flgs)
{if (fnmatch(get_c_string(pat),
	     get_c_string(str),
	     0))
   return(NIL);
 else
   return(siod_a_true_value());}

#endif

#if defined(unix) || defined(WIN32)

siod_LISP siod_l_chmod(siod_LISP path,siod_LISP mode)
{if (chmod(get_c_string(path),siod_get_c_long(mode)))
   return(err("chmod",siod_llast_c_errmsg(-1)));
 else
   return(NIL);}

#endif


#ifdef unix

siod_LISP siod_lutime(siod_LISP fname,siod_LISP mod,siod_LISP ac)
{struct utimbuf x;
 x.modtime = siod_get_c_long(mod);
 x.actime = NNULLP(ac) ? siod_get_c_long(ac) : time(NULL);
 if (utime(get_c_string(fname), &x))
   return(err("utime",siod_llast_c_errmsg(-1)));
 else
   return(NIL);}


siod_LISP siod_lfchmod(siod_LISP file,siod_LISP mode)
{if (fchmod(fileno(siod_get_c_file(file,NULL)),siod_get_c_long(mode)))
   return(err("fchmod",siod_llast_c_errmsg(-1)));
 else
   return(NIL);}

siod_LISP siod_encode_open_flags(siod_LISP l)
{return(siod_flocons(siod_assemble_options(l,
				 "NONBLOCK",O_NONBLOCK,
				 "APPEND",O_APPEND,
				 "RDONLY",O_RDONLY,
				 "WRONLY",O_WRONLY,
				 "RDWR",O_RDWR,
				 "CREAT",O_CREAT,
				 "TRUNC",O_TRUNC,
				 "EXCL",O_EXCL,
				 NULL)));}

int siod_get_fd(siod_LISP ptr)
{if TYPEP(ptr,tc_c_file)
   return(fileno(siod_get_c_file(ptr,NULL)));
 else
   return(siod_get_c_long(ptr));}

siod_LISP siod_gsetlk(int op,siod_LISP lfd,siod_LISP ltype,siod_LISP whence,siod_LISP start,siod_LISP len)
{struct flock f;
 int fd = siod_get_fd(lfd);
 f.l_type = siod_get_c_long(ltype);
 f.l_whence = NNULLP(whence) ? siod_get_c_long(whence) : SEEK_SET;
 f.l_start = NNULLP(start) ? siod_get_c_long(start) : 0;
 f.l_len = NNULLP(len) ? siod_get_c_long(len) : 0;
 f.l_pid = 0;
 if (fcntl(fd,op,&f) == -1)
   return(siod_llast_c_errmsg(-1));
 else if (op != F_GETLK)
   return(NIL);
 else if (f.l_type == F_UNLCK)
   return(NIL);
 else
   return(siod_listn(2,siod_flocons(f.l_type),siod_flocons(f.l_pid)));}

siod_LISP siod_lF_SETLK(siod_LISP fd,siod_LISP ltype,siod_LISP whence,siod_LISP start,siod_LISP len)
{return(siod_gsetlk(F_SETLK,fd,ltype,whence,start,len));}

siod_LISP siod_lF_SETLKW(siod_LISP fd,siod_LISP ltype,siod_LISP whence,siod_LISP start,siod_LISP len)
{return(siod_gsetlk(F_SETLKW,fd,ltype,whence,start,len));}

siod_LISP siod_lF_GETLK(siod_LISP fd,siod_LISP ltype,siod_LISP whence,siod_LISP start,siod_LISP len)
{return(siod_gsetlk(F_GETLK,fd,ltype,whence,start,len));}

#endif

#endif

siod_LISP siod_delete_file(siod_LISP fname)
{int iflag,ret;
 iflag = siod_no_interrupt(1);
#ifdef VMS
 ret = delete(get_c_string(fname));
#else
 ret = unlink(get_c_string(fname));
#endif
 siod_no_interrupt(iflag);
 if (ret)
   return(siod_strcons(-1,last_c_errmsg(-1)));
 else
   return(NIL);}

siod_LISP siod_utime2str(siod_LISP u)
{time_t bt;
 struct tm *btm;
 char sbuff[100];
 bt = siod_get_c_long(u);
 if ((btm = localtime(&bt)))
   {sprintf(sbuff,"%04d%02d%02d%02d%02d%02d%02d",
	    btm->tm_year+1900,btm->tm_mon + 1,btm->tm_mday,
	    btm->tm_hour,btm->tm_min,btm->tm_sec,0);
    return(siod_strcons(strlen(sbuff),sbuff));}
 else
   return(NIL);}

#ifdef WIN32
siod_LISP siod_win32_debug(void)
{DebugBreak();
 return(NIL);}
#endif

#ifdef VMS

siod_LISP siod_vms_debug(arg)
     siod_LISP arg;
{unsigned char arg1[257];
 char *data;
 if NULLP(arg)
   lib$signal(SS$_DEBUG,0);
 else
   {data = get_c_string(arg);
    arg1[0] = strlen(data);
    memcpy(&arg1[1],data,arg1[0]);
    lib$signal(SS$_DEBUG,1,arg1);}
 return(NIL);}

struct dsc$descriptor *set_dsc_cst(struct dsc$descriptor *d,char *s)
{d->dsc$w_length = strlen(s);
 d->dsc$b_dtype = DSC$K_DTYPE_T;
 d->dsc$b_class = DSC$K_CLASS_S;
 d->dsc$a_pointer = s;
 return(d);}


void siod_err_vms(long retval)
{char *errmsg,buff[100];
 if (errmsg = strerror(EVMSERR,retval))
   err(errmsg,NIL);
 else
   {sprintf(buff,"VMS ERROR %d",retval);
    err(buff,NIL);}}

siod_LISP siod_lcrembx(siod_LISP l)
{siod_LISP tmp;
 short chan;
 int prmflg,maxmsg,bufquo,promsk,acmode,iflag,retval;
 struct dsc$descriptor lognam;
 set_dsc_cst(&lognam,get_c_string(siod_car(l)));
 tmp = siod_cadr(siod_assq(siod_cintern("prmflg"),l));
 prmflg = NNULLP(tmp) ? 1 : 0;
 tmp = siod_cadr(siod_assq(siod_cintern("maxmsg"),l));
 maxmsg = NNULLP(tmp) ? siod_get_c_long(tmp) : 0;
 tmp = siod_cadr(siod_assq(siod_cintern("bufquo"),l));
 bufquo = NNULLP(tmp) ? siod_get_c_long(tmp) : 0;
 tmp = siod_cadr(siod_assq(siod_cintern("promsk"),l));
 promsk = NNULLP(tmp) ? siod_get_c_long(tmp) : 0;
 tmp = siod_cadr(siod_assq(siod_cintern("acmode"),l));
 acmode = NNULLP(tmp) ? siod_get_c_long(tmp) : 0;
 tmp = siod_cons(siod_flocons(-1),siod_leval(sym_channels,NIL));
 iflag = siod_no_interrupt(1);
 retval = sys$crembx(prmflg,&chan,maxmsg,bufquo,promsk,acmode,&lognam);
 if (retval != SS$_NORMAL)
   {siod_no_interrupt(iflag);
    siod_err_vms(retval);}
 siod_setvar(sym_channels,tmp,NIL);
 tmp = siod_car(tmp);
 tmp->storage_as.flonum.data = chan;
 siod_no_interrupt(iflag);
 return(tmp);}

siod_LISP siod_lset_logical(siod_LISP name,siod_LISP value,siod_LISP table,siod_LISP attributes)
{struct dsc$descriptor dname,dvalue,dtable;
 long status,iflag;
 iflag = siod_no_interrupt(1);
 status = lib$set_logical(set_dsc_cst(&dname,get_c_string(name)),
			  NULLP(value) ? 0 : set_dsc_cst(&dvalue,
							 get_c_string(value)),
			  NULLP(table) ? 0 : set_dsc_cst(&dtable,
							 get_c_string(table)),
			  siod_assemble_options(attributes,
					   "NO_ALIAS",LNM$M_NO_ALIAS,
					   "CONFINE",LNM$M_CONFINE,
					   "CRELOG",LNM$M_CRELOG,
					   "TABLE",LNM$M_TABLE,
					   "CONCEALED",LNM$M_CONCEALED,
					   "TERMINAL",LNM$M_TERMINAL,
					   "EXISTS",LNM$M_EXISTS,
					   "SHAREABLE",LNM$M_SHAREABLE,
					   "CREATE_IF",LNM$M_CREATE_IF,
					   "CASE_BLIND",LNM$M_CASE_BLIND,
					   NULL),
			  0);
 if (status != SS$_NORMAL)
   siod_err_vms(status);
 siod_no_interrupt(iflag);
 return(NIL);}

#endif

siod_LISP siod_lgetenv(siod_LISP var)
{char *str;
 if ((str = getenv(get_c_string(var))))
   return(siod_strcons(strlen(str),str));
 else
   return(NIL);}

siod_LISP siod_unix_time(void)
{return(siod_flocons(time(NULL)));}

siod_LISP siod_unix_ctime(siod_LISP value)
{time_t b;
 char *buff,*p;
 if NNULLP(value)
   b = siod_get_c_long(value);
 else
   time(&b);
 if ((buff = ctime(&b)))
   {if ((p = strchr(buff,'\n'))) *p = 0;
    return(siod_strcons(strlen(buff),buff));}
 else
   return(NIL);}

siod_LISP siod_http_date(siod_LISP value)
     /* returns the siod_internet standard RFC 1123 format */
{time_t b;
 char buff[256];
 struct tm *t;
 if NNULLP(value)
   b = siod_get_c_long(value);
 else
   time(&b);
 if (!(t = gmtime(&b))) return(NIL);
 (sprintf
  (buff,"%s, %02d %s %04d %02d:%02d:%02d GMT",
   &"Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat"[t->tm_wday*4],
   t->tm_mday,
   &"Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec"[t->tm_mon*4],
   t->tm_year+1900,
   t->tm_hour,
   t->tm_min,
   t->tm_sec));
 return(siod_strcons(strlen(buff),buff));}

#if defined(__osf__)

siod_LISP siod_http_date_parse(siod_LISP input)
     /* handle RFC 822, RFC 850, RFC 1123 and the ANSI C ascitime() format */
{struct tm tm,*lc;
 time_t t;
 int gmtoff;
 char *str = get_c_string(input),*format;
 t = time(NULL);
 if (lc = localtime(&t))
   gmtoff = lc->tm_gmtoff;
 if (strchr(str,',') && strchr(str,'-')) 
   /* rfc-850: Sunday, 06-Nov-94 08:49:37 GMT */
   format = "%a, %d-%b-%y %H:%M:%S GMT";
 else if (strchr(str,','))
   /* rfc-1123: Sun, 06 Nov 1994 08:49:37 GMT */
   format = "%a, %d %b %Y %H:%M:%S GMT";
 else
   /* ascitime: Sun Nov  6 08:49:37 1994 */
   {format = "%c";
    gmtoff = 0;}
 if (strptime(str,format,&tm))
   {t = mktime(&tm);
    /* unfortunately there is no documented way to tell mktime
       to assume GMT. Except for saving the value of the current
       timezone, setting TZ to GMT, doing a tzset() then doing
       our mktime() followed by setting the time zone back to the way
       it was before. That is fairly horrible, so instead we work around
       this by adding the gmtoff we computed above, which of course may
       have changed since we computed it (if the system manager switched
       daylight savings time modes, for example).
       There is an executable /usr/lib/mh/dp which is presumably
       doing the same sort of thing, although perhaps it uses tzset */
    return(siod_flocons(t + gmtoff));}
 else
   return(NIL);}

#endif


#ifdef hpux
long siod_usleep(unsigned int winks)	/* added, dcd */
{
  struct timeval sleepytime;
  sleepytime.tv_sec = winks / 1000000;
  sleepytime.tv_usec = winks % 1000000;
  return select(0,0,0,0,&sleepytime);
}
#endif

/* #if defined(sun_old) || defined(sgi) */
/* long siod_usleep(unsigned int winks) */
/* {struct timespec x; */
/*  x.tv_sec = winks / 1000000; */
/*  x.tv_nsec = (winks % 1000000) * 1000; */
/*  return(nanosleep(&x,NULL));} */
/* #endif */

siod_LISP siod_lsleep(siod_LISP ns)
{double val = siod_get_c_double(ns);
#ifdef unix
  usleep((unsigned int)(val * 1.0e6));
#else
#ifdef WIN32
 Sleep((DWORD)(val * 1000));
#else
 sleep((unsigned int)val);
#endif
#endif
 return(NIL);}

siod_LISP siod_url_encode(siod_LISP in)
{int spaces=0,specials=0,regulars=0,c;
 char *str = get_c_string(in),*p,*r;
 siod_LISP out;
 for(p=str,spaces=0,specials=0,regulars=0;(c = *p);++p)
   if (c == ' ') ++spaces;
   else if (!(isalnum(c) || strchr("*-._@",c))) ++specials;
   else ++regulars;
 if ((spaces == 0) && (specials == 0))
   return(in);
 out = siod_strcons(spaces + regulars + specials * 3,NULL);
 for(p=str,r=get_c_string(out);(c = *p);++p)
   if (c == ' ')
     *r++ = '+';
   else if (!(isalnum(c) || strchr("*-._@",c)))
     {sprintf(r,"%%%02X",c & 0xFF);
      r += 3;}
   else
     *r++ = c;
 *r = 0;
 return(out);}

siod_LISP siod_url_decode(siod_LISP in)
{int siod_pluses=0,specials=0,regulars=0,c,j;
 char *str = get_c_string(in),*p,*r;
 siod_LISP out;
 for(p=str,siod_pluses=0,specials=0,regulars=0;(c = *p);++p)
   if (c == '+') ++siod_pluses;
   else if (c == '%')
     {if (isxdigit(p[1]) && isxdigit(p[2]))
	++specials;
      else
	return(NIL);}
   else
     ++regulars;
 if ((siod_pluses == 0) && (specials == 0))
   return(in);
 out = siod_strcons(regulars + siod_pluses + specials,NULL);
 for(p=str,r=get_c_string(out);(c = *p);++p)
   if (c == '+')
     *r++ = ' ';
   else if (c == '%')
     {for(*r = 0,j=1;j<3;++j)
	*r = *r * 16 + ((isdigit(p[j]))
			? (p[j] - '0')
			: (toupper(p[j]) - 'A' + 10));
      p += 2;
      ++r;}
   else
     *r++ = c;
 *r = 0;
 return(out);}

siod_LISP siod_html_encode(siod_LISP in)
{long j,n,m;
 char *str,*ptr;
 siod_LISP out;
 switch(TYPE(in))
   {case tc_string:
    case tc_symbol:
      break;
    default:
      return(in);}
 str = get_c_string(in);
 n = strlen(str);
 for(j=0,m=0;j < n; ++j)
   switch(str[j])
     {case '>':
      case '<':
	m += 4;
	break;
      case '&':
	m += 5;
	break;
      case '"':
	m += 6;
        break;
      default:
	++m;}
 if (n == m) return(in);
 out = siod_strcons(m,NULL);
 for(j=0,ptr=get_c_string(out);j < n; ++j)
   switch(str[j])
     {case '>':
	strcpy(ptr,"&gt;");
	ptr += strlen(ptr);
	break;
      case '<':
	strcpy(ptr,"&lt;");
	ptr += strlen(ptr);
	break;
      case '&':
	strcpy(ptr,"&amp;");
	ptr += strlen(ptr);
	break;
      case '"':
        strcpy(ptr,"&quot;");
	ptr += strlen(ptr);
	break;
      default:
	*ptr++ = str[j];}
 return(out);}

siod_LISP siod_html_decode(siod_LISP in)
{return(in);}

siod_LISP siod_lgets(siod_LISP file,siod_LISP buffn)
{FILE *f;
 int iflag;
 long n;
 char buffer[2048],*ptr;
 f = siod_get_c_file(file,stdin);
 if NULLP(buffn)
   n = sizeof(buffer);
 else if ((n = siod_get_c_long(buffn)) < 0)
   err("size must be >= 0",buffn);
 else if (n > sizeof(buffer))
   err("not handling buffer of size",siod_listn(2,buffn,siod_flocons(sizeof(buffer))));
 iflag = siod_no_interrupt(1);
 if ((ptr = fgets(buffer,n,f)))
   {siod_no_interrupt(iflag);
    return(siod_strcons(strlen(buffer),buffer));}
 siod_no_interrupt(iflag);
 return(NIL);}

siod_LISP siod_readline(siod_LISP file)
{siod_LISP result;
 char *start,*ptr;
 result = siod_lgets(file,NIL);
 if NULLP(result) return(NIL);
 start = get_c_string(result);
 if ((ptr = strchr(start,'\n')))
   {*ptr = 0;
    /* we also change the dim, because otherwise our siod_equal? function
       is confused. What we really need are arrays with fill pointers. */
    result->storage_as.string.dim = ptr - start;
    return(result);}
 else
   /* we should be doing siod_lgets until we  get a string with a newline or NIL,
      and then siod_append the results */
   return(result);}

#ifndef WIN32

siod_LISP siod_l_chown(siod_LISP path,siod_LISP uid,siod_LISP gid)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (chown(get_c_string(path),siod_get_c_long(uid),siod_get_c_long(gid)))
   err("chown",siod_cons(path,siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}

#endif

#if defined(unix) && !defined(linux)
siod_LISP siod_l_lchown(siod_LISP path,siod_LISP uid,siod_LISP gid)
{long iflag;
 iflag = siod_no_interrupt(1);
 if (lchown(get_c_string(path),siod_get_c_long(uid),siod_get_c_long(gid)))
   err("lchown",siod_cons(path,siod_llast_c_errmsg(-1)));
 siod_no_interrupt(iflag);
 return(NIL);}
#endif


#ifdef unix

siod_LISP siod_popen_l(siod_LISP name,siod_LISP how)
{return(siod_fopen_cg(popen,
		 get_c_string(name),
		 NULLP(how) ? "r" : get_c_string(how)));}

/* note: if the user fails to call pclose then the gc is going
         to utilize fclose, which can result in a <defunct>
	 process laying around. However, we don't want to
	 modify siod_file_gc_free nor add a new datatype.
	 So beware.
	 */
siod_LISP siod_pclose_l(siod_LISP ptr)
{FILE *f = siod_get_c_file(ptr,NULL);
 long iflag = siod_no_interrupt(1);
 int retval,xerrno;
 retval = pclose(f);
 xerrno = errno;
 ptr->storage_as.c_file.f = (FILE *) NULL;
 free(ptr->storage_as.c_file.name);
 ptr->storage_as.c_file.name = NULL;
 siod_no_interrupt(iflag);
 if (retval < 0)
   err("pclose",siod_llast_c_errmsg(xerrno));
 return(siod_flocons(retval));}

#endif

siod_LISP siod_so_init_name(siod_LISP fname,siod_LISP iname)
{siod_LISP init_name;
 if NNULLP(iname)
   init_name = iname;
 else
   {init_name = siod_car(siod_last(siod_lstrbreakup(fname,siod_cintern("/"))));
#if !defined(VMS)
    init_name = siod_lstrunbreakup(siod_butlast(siod_lstrbreakup(init_name,siod_cintern("."))),
			      siod_cintern("."));
#endif
    init_name = siod_string_append(siod_listn(2,siod_cintern("init_"),init_name));}
 return(siod_intern(init_name));}

siod_LISP siod_so_ext(siod_LISP fname)
{char *ext = ".so";
 siod_LISP lext;
#if defined(hpux)
 ext = ".sl";
#endif
#if defined(vms)
 ext = "";
#endif
#if defined(WIN32)
 ext = ".dll";
#endif
 lext = siod_strcons(strlen(ext),ext);
 if NULLP(fname)
   return(lext);
 else
   return(siod_string_append(siod_listn(2,fname,lext)));}

siod_LISP siod_load_so(siod_LISP fname,siod_LISP iname)
     /* note: error cases can leak memory in this procedure. */
{siod_LISP init_name;
 void (*fcn)(void) = NULL;
#if defined(__osf__) || defined(sun) || defined(linux) || defined(sgi)
 void *handle;
#endif
#if defined(hpux)
 shl_t handle;
#endif
#if defined(VMS)
 struct dsc$descriptor filename,symbol,defaultd;
 long status;
 siod_LISP dsym;
#endif
#ifdef WIN32
 HINSTANCE handle;
#endif
 long iflag;
 init_name = siod_so_init_name(fname,iname);
 iflag = siod_no_interrupt(1);
 if (siod_verbose_check(3))
   {siod_put_st("so-siod_loading ");
    siod_put_st(get_c_string(fname));
    siod_put_st("\n");}
#if defined(__osf__) || defined(sun) || defined(linux) || defined(sgi)
#if !defined(__osf__)
 /* Observed bug: values of LD_LIBRARY_PATH established with putenv
    -after- a process has started are ignored. Work around follows. */
 if (access(get_c_string(fname),F_OK))
   fname = siod_string_append(siod_listn(3,
			       siod_strcons(-1,siod_lib),
			       siod_strcons(-1,"/"),
			       fname));
#endif
 if (!(handle = dlopen(get_c_string(fname),RTLD_LAZY)))
   err(dlerror(),fname);
 if (!(fcn = dlsym(handle,get_c_string(init_name))))
   err(dlerror(),init_name);
#endif
#if defined(hpux)
 if (access(get_c_string(fname),F_OK))
   fname = siod_string_append(siod_listn(3,
			       siod_strcons(-1,siod_lib),
			       siod_strcons(-1,"/"),
			       fname));
 if (!(handle = shl_siod_load(get_c_string(fname),BIND_DEFERRED,0L)))
   err("shl_siod_load",siod_llast_c_errmsg(errno));
 if (shl_findsym(&handle,get_c_string(init_name),TYPE_PROCEDURE,&fcn))
   err("shl_findsym",siod_llast_c_errmsg(errno));
#endif
#if defined(VMS)
 dsym = siod_cintern("*siod_require-so-dir*");
 if (NNULLP(siod_symbol_boundp(dsym,NIL)) && NNULLP(siod_symbol_value(dsym,NIL)))
   set_dsc_cst(&defaultd,get_c_string(siod_symbol_value(dsym,NIL)));
 else
   dsym = NIL;
 status = lib$find_image_symbol(set_dsc_cst(&filename,
					    get_c_string(fname)),
				set_dsc_cst(&symbol,
					    get_c_string(init_name)),
				&fcn,
				NULLP(dsym) ? 0 : &defaultd);
 if (status != SS$_NORMAL)
   siod_err_vms(status);
#endif
#ifdef WIN32
 if (!(handle = LoadLibrary(get_c_string(fname))))
   err("LoadLibrary",fname);
 if (!(fcn = (LPVOID)GetProcAddress(handle,get_c_string(init_name))))
   err("GetProcAddress",init_name);
#endif
 if (fcn)
   (*fcn)();
 else
   err("did not siod_load function",init_name);
 siod_no_interrupt(iflag);
 if (siod_verbose_check(3))
   siod_put_st("done.\n");
 return(init_name);}

siod_LISP siod_require_so(siod_LISP fname)
{siod_LISP init_name;
 init_name = siod_so_init_name(fname,NIL);
 if (NULLP(siod_symbol_boundp(init_name,NIL)) ||
     NULLP(siod_symbol_value(init_name,NIL)))
   {siod_load_so(fname,NIL);
    return(siod_setvar(init_name,siod_a_true_value(),NIL));}
 else
   return(NIL);}

siod_LISP siod_lib_l(void)
{return(siod_rintern(siod_lib));}


siod_LISP siod_ccall_catch_1(siod_LISP (*fcn)(void *),void *arg)
{siod_LISP val;
 val = (*fcn)(arg);
 catch_framep = catch_framep->next;
 return(val);}

siod_LISP siod_ccall_catch(siod_LISP tag,siod_LISP (*fcn)(void *),void *arg)
{struct catch_frame frame;
 int k;
 frame.tag = tag;
 frame.next = catch_framep;
 k = setjmp(frame.cframe);
 catch_framep = &frame;
 if (k == 2)
   {catch_framep = frame.next;
    return(frame.retval);}
 return(siod_ccall_catch_1(fcn,arg));}

siod_LISP siod_decode_tm(struct tm *t)
{return(siod_symalist("sec",siod_flocons(t->tm_sec),
		 "min",siod_flocons(t->tm_min),
		 "hour",siod_flocons(t->tm_hour),
		 "mday",siod_flocons(t->tm_mday),
		 "mon",siod_flocons(t->tm_mon),
		 "year",siod_flocons(t->tm_year),
		 "wday",siod_flocons(t->tm_wday),
		 "yday",siod_flocons(t->tm_yday),
		 "isdst",siod_flocons(t->tm_isdst),
#if defined(__osf__)
		 "gmtoff",siod_flocons(t->__tm_gmtoff),
		 "tm_zone",(t->__tm_zone) ? siod_rintern(t->__tm_zone) : NIL,
#endif
		 NULL));}

siod_LISP siod_symalist(char *arg,...)
{va_list args;
 siod_LISP result,l,val;
 char *key;
 if (!arg) return(NIL);
 va_start(args,arg);
 val = va_arg(args,siod_LISP);
 result = siod_cons(siod_cons(siod_cintern(arg),val),NIL);
 l = result;
 while((key = va_arg(args,char *)))
   {val = va_arg(args,siod_LISP);
    CDR(l) = siod_cons(siod_cons(siod_cintern(key),val),NIL);
    l = CDR(l);}
 va_end(args);
 return(result);}

void siod_encode_tm(siod_LISP alist,struct tm *t)
{siod_LISP val;
 val = siod_cdr(siod_assq(siod_cintern("sec"),alist));
 t->tm_sec = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("min"),alist));
 t->tm_min = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("hour"),alist));
 t->tm_hour = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("mday"),alist));
 t->tm_mday = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("mon"),alist));
 t->tm_mon = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("year"),alist));
 t->tm_year = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("wday"),alist));
 t->tm_wday = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("yday"),alist));
 t->tm_yday = NULLP(val) ? 0 : siod_get_c_long(val);
 val = siod_cdr(siod_assq(siod_cintern("isdst"),alist));
 t->tm_isdst = NULLP(val) ? -1 : siod_get_c_long(val);
#if defined(__osf__)
 val = siod_cdr(siod_assq(siod_cintern("gmtoff"),alist));
 t->__tm_gmtoff = NULLP(val) ? 0 : siod_get_c_long(val);
#endif
}

siod_LISP siod_llocaltime(siod_LISP value)
{time_t b;
 struct tm *t;
 if NNULLP(value)
   b = siod_get_c_long(value);
 else
   time(&b);
 if ((t = localtime(&b)))
   return(siod_decode_tm(t));
 else
   return(err("localtime",siod_llast_c_errmsg(-1)));}

siod_LISP siod_lgmtime(siod_LISP value)
{time_t b;
 struct tm *t;
 if NNULLP(value)
   b = siod_get_c_long(value);
 else
   time(&b);
 if ((t = gmtime(&b)))
   return(siod_decode_tm(t));
 else
   return(err("gmtime",siod_llast_c_errmsg(-1)));}

#if defined(unix) || defined(WIN32)
siod_LISP siod_ltzset(void)
{tzset();
 return(NIL);}
#endif

siod_LISP siod_lmktime(siod_LISP alist)
{struct tm tm;
 time_t t;
 siod_encode_tm(alist,&tm);
 t = mktime(&tm);
 return(siod_flocons(t));}

#if defined(__osf__) || defined(SUN5) || defined(linux)

siod_LISP siod_lstrptime(siod_LISP str,siod_LISP fmt,siod_LISP in)
{struct tm tm;
 siod_encode_tm(in,&tm);
 if (strptime(get_c_string(str),get_c_string(fmt),&tm))
   {
#if defined(SUN5)
     /* SUN software incorrectly sets this to 0, but until further
	analysis (such as by mktime) it is too early to conclude */
     tm.tm_isdst = -1;
#endif
     return(siod_decode_tm(&tm));
   }
 else
   return(NIL);}

#endif

#ifdef unix

siod_LISP siod_lstrftime(siod_LISP fmt,siod_LISP in)
{struct tm tm;
 time_t b;
 struct tm *t;
 size_t ret;
 char buff[1024];
 if NNULLP(in)
   {siod_encode_tm(in,&tm);
    t = &tm;}
 else
   {time(&b);
    if (!(t = gmtime(&b)))
      return(NIL);}
 if ((ret = strftime(buff,sizeof(buff),get_c_string(fmt),t)))
   return(siod_strcons(ret,buff));
 else
   return(NIL);}

#endif

siod_LISP siod_lchdir(siod_LISP dir)
{long iflag;
#ifdef unix
 FILE *f;
 int fd;
#endif
 char *path;
 switch(TYPE(dir))
   {case tc_c_file:
#ifdef unix
      f = siod_get_c_file(dir,NULL);
      fd = fileno(f);
      iflag = siod_no_interrupt(1);
      if (fchdir(fd))
	return(err("fchdir",siod_llast_c_errmsg(-1)));
      siod_no_interrupt(iflag);
#else
      err("fchdir not supported in os",NIL);
#endif
      return(NIL);
    default:
      path = get_c_string(dir);
      iflag = siod_no_interrupt(1);
      if (chdir(path))
	return(err("chdir",siod_llast_c_errmsg(-1)));
      siod_no_interrupt(iflag);
      return(NIL);}}

#if defined(__osf__)
siod_LISP siod_rld_pathnames(void)
     /* this is a quick diagnostic to know what images we are running */
{char *path;
 siod_LISP result = NIL;
 for(path=_rld_first_pathname();path;path=_rld_next_pathname())
   result = siod_cons(siod_strcons(strlen(path),path),result);
 return(siod_nreverse(result));}
#endif

#ifdef unix
siod_LISP siod_lgetpass(siod_LISP lprompt)
{long iflag;
 char *result;
 iflag = siod_no_interrupt(1);
 result = getpass(NULLP(lprompt) ? "" : get_c_string(lprompt));
 siod_no_interrupt(iflag);
 if (result)
   return(siod_strcons(strlen(result),result));
 else
   return(NIL);}
#endif

#ifdef unix
siod_LISP siod_lpipe(void)
{int filedes[2];
 long iflag;
 siod_LISP f1,f2;
 f1 = siod_cons(NIL,NIL);
 f2 = siod_cons(NIL,NIL);
 iflag = siod_no_interrupt(1);
 if (pipe(filedes) == 0)
   {f1->type = tc_c_file;
    f1->storage_as.c_file.f = fdopen(filedes[0],"r");
    f2->type = tc_c_file;
    f2->storage_as.c_file.f = fdopen(filedes[1],"w");
    siod_no_interrupt(iflag);
    return(siod_listn(2,f1,f2));}
 else
   return(err("pipe",siod_llast_c_errmsg(-1)));}
#endif

#define CTYPE_FLOAT   1
#define CTYPE_DOUBLE  2
#define CTYPE_CHAR    3
#define CTYPE_UCHAR   4
#define CTYPE_SHORT   5
#define CTYPE_USHORT  6
#define CTYPE_INT     7
#define CTYPE_UINT    8
#define CTYPE_LONG    9
#define CTYPE_ULONG  10

siod_LISP siod_err_large_index(siod_LISP ind)
{return(err("index too large",ind));}

siod_LISP siod_datref(siod_LISP dat,siod_LISP ctype,siod_LISP ind)
{char *data;
 long size,i;
 data = get_c_siod_string_dim(dat,&size);
 i = siod_get_c_long(ind);
 if (i < 0) err("negative index",ind);
 switch(siod_get_c_long(ctype))
   {case CTYPE_FLOAT:
      if (((i+1) * (int) sizeof(float)) > size) siod_err_large_index(ind);
      return(siod_flocons(((float *)data)[i]));
    case CTYPE_DOUBLE:
      if (((i+1) * (int) sizeof(double)) > size) siod_err_large_index(ind);
      return(siod_flocons(((double *)data)[i]));
    case CTYPE_LONG:
      if (((i+1) * (int) sizeof(long)) > size) siod_err_large_index(ind);
      return(siod_flocons(((long *)data)[i]));
    case CTYPE_SHORT:
      if (((i+1) * (int) sizeof(short)) > size) siod_err_large_index(ind);
      return(siod_flocons(((short *)data)[i]));
    case CTYPE_CHAR:
      if (((i+1) * (int) sizeof(char)) > size) siod_err_large_index(ind);
      return(siod_flocons(((char *)data)[i]));
    case CTYPE_INT:
      if (((i+1) * (int) sizeof(int)) > size) siod_err_large_index(ind);
      return(siod_flocons(((int *)data)[i]));
    case CTYPE_ULONG:
      if (((i+1) * (int) sizeof(unsigned long)) > size) siod_err_large_index(ind);
      return(siod_flocons(((unsigned long *)data)[i]));
    case CTYPE_USHORT:
      if (((i+1) * (int) sizeof(unsigned short)) > size) siod_err_large_index(ind);
      return(siod_flocons(((unsigned short *)data)[i]));
    case CTYPE_UCHAR:
      if (((i+1) * (int) sizeof(unsigned char)) > size) siod_err_large_index(ind);
      return(siod_flocons(((unsigned char *)data)[i]));
    case CTYPE_UINT:
      if (((i+1) * (int) sizeof(unsigned int)) > size) siod_err_large_index(ind);
      return(siod_flocons(((unsigned int *)data)[i]));
    default:
      return(err("unknown CTYPE",ctype));}}

siod_LISP siod_ssiod_datref(siod_LISP spec,siod_LISP dat)
{return(siod_datref(dat,siod_car(spec),siod_cdr(spec)));}

siod_LISP siod_mksiod_datref(siod_LISP ctype,siod_LISP ind)
{return(siod_closure(siod_cons(ctype,ind),
		siod_leval(siod_cintern("siod_ssiod_datref"),NIL)));}

siod_LISP siod_datlength(siod_LISP dat,siod_LISP ctype)
{char *data;
 long size;
 data = get_c_siod_string_dim(dat,&size);
 switch(siod_get_c_long(ctype))
   {case CTYPE_FLOAT:
      return(siod_flocons(size / sizeof(float)));
    case CTYPE_DOUBLE:
      return(siod_flocons(size / sizeof(double)));
    case CTYPE_LONG:
      return(siod_flocons(size / sizeof(long)));
    case CTYPE_SHORT:
      return(siod_flocons(size / sizeof(short)));
    case CTYPE_CHAR:
      return(siod_flocons(size / sizeof(char)));
    case CTYPE_INT:
      return(siod_flocons(size / sizeof(int)));
    case CTYPE_ULONG:
      return(siod_flocons(size / sizeof(unsigned long)));
    case CTYPE_USHORT:
      return(siod_flocons(size / sizeof(unsigned short)));
    case CTYPE_UCHAR:
      return(siod_flocons(size / sizeof(unsigned char)));
    case CTYPE_UINT:
      return(siod_flocons(size / sizeof(unsigned int)));
    default:
      return(err("unknown CTYPE",ctype));}}

static siod_LISP siod_cgi_main(siod_LISP flag,siod_LISP result)
{int code;
 code = FLONUMP(flag) ? ((long)FLONM(flag)) : 0;
 
 if (CONSP(result) && TYPEP(siod_car(result),tc_string))
   {if ((code != 4) && (code != 6))
     siod_put_st("Status: 500 Server Error (Application)\n");
    if ((code != 5) && (code != 6))
      siod_put_st("Content-type: text/html\n\n");
    else
      siod_put_st("Content-type: text/xml\n\n");
    siod_put_st("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n");
    siod_put_st("<HTML><HEAD><TITLE>Server Error (Application)</TITLE></HEAD>\n");
    siod_put_st("<BODY><H1>Server Error (Application)</H1>\n");
    siod_put_st("An application on this server has encountered an error\n");
    siod_put_st("which prevents it from fulfilling your request.");
    siod_put_st("<P><PRE><B>Error Message:</B> ");
    siod_lprint(siod_car(result),NIL);
    if NNULLP(siod_cdr(result))
      {siod_put_st("\n");
       siod_lprint(siod_cdr(result),NIL);}
    siod_put_st("</PRE></P></BODY></HTML>\n");
    err("cgi-main",NIL);}
 return(NIL);}


static int siod_htqs_arg(char *value)
{char tmpbuff[1024],*p1,*p2;
 if ((strcmp(value,"(siod_repl)") == 0) ||
     (strcmp(value,"siod_repl") == 0))
   return(siod_repl_driver(1,1,NULL));
 else if (!strchr(value,'('))
   {strcpy(tmpbuff,"(siod_require \"");
    for(p1 = &tmpbuff[strlen(tmpbuff)],p2 = value;*p2;++p2)
	{if (strchr("\\\"",*p2)) *p1++ = '\\';
	 *p1++ = *p2;}
	*p1 = 0;
	strcat(tmpbuff,"\")");
    return(siod_repl_c_string(tmpbuff,0,0,0));}
 else
   return(siod_repl_c_string(value,0,0,0));}


int __stdcall ulsiod_main(int argc,char **argv, char **env)
{int j,retval = 0,iargc,mainflag = 0,text_plain_flag = 0;
 char *iargv[2],*start,*end,siod_htqs_arg_tmp[100];
 siod_LISP l;
 iargv[0] = "";
 for(iargc=0,j=1;j<argc; ++j)
   if (*(start = argv[j]) == '-')
     {while(*start)
	{if (!(end = strstr(start,",-"))) end = &start[strlen(start)];
	 iargv[1] = (char *) malloc(end-start+1);
	 memcpy(iargv[1],start,end-start);
	 iargv[1][end-start] = 0;
	 if ((strncmp(iargv[1],"-v",2) == 0) &&
	     (atol(&iargv[1][2]) > 0) &&
	     (iargv[1][2] != '0'))
	   {printf("Content-type: text/plain\r\n\r\n");
	    text_plain_flag = 1;}
	 if ((strncmp(iargv[1],"-m",2) == 0))
	   mainflag = atol(&iargv[1][2]);
	 else
	   ulsiod_process_cla(2,iargv,1);
	 /* Note: Not doing free(iargv[1]); */
	 start = (*end) ? end+1 : end;}}
 else
   ++iargc;
 ulsiod_print_welcome();
 ulsiod_print_hs_1();
 siod_init_storage();
 for(l=NIL,j=0;j<argc;++j)
   l = siod_cons(siod_strcons(strlen(argv[j]),argv[j]),l);
 siod_setvar(siod_cintern("*args*"),siod_nreverse(l),NIL);
 l = NIL;
 for(l=NIL,j=0;env && env[j];++j)
   l = siod_cons(siod_strcons(strlen(env[j]),env[j]),l);
 siod_setvar(siod_cintern("*env*"),siod_nreverse(l),NIL);
 l = NIL;
 siod_init_subrs();
 siod_init_trace();
 siod_init_slibu();
 siod_init_subr_2("__cgi-main",siod_cgi_main);
 if (iargc == 0)
   retval = siod_repl_driver(1,1,NULL);
 else
   {for(j=1;j<(((mainflag >= 2) && (argc > 3)) ? 3 : argc);++j)
      if (argv[j][0] != '-')
	{retval = siod_htqs_arg(argv[j]);
	 if (retval != 0) break;}
     if (mainflag) {
      if ((mainflag > 2) && !text_plain_flag)
	{sprintf(siod_htqs_arg_tmp,"(__cgi-main %d (*catch 'errobj (main))))",
		 mainflag);
	 retval = siod_htqs_arg(siod_htqs_arg_tmp);}
      else
	retval = siod_htqs_arg("(main)");}}
 if (siod_verbose_check(2))
   printf("EXIT\n");
#ifdef VMS
 if (retval == 0) retval = 1;
#endif
 return(retval);}

long siod_position_script(FILE *f,char *buff,size_t bufflen)
/* This recognizes #!/ sequence. Exersize: compute the probability
   of the sequence showing up in a file of N random bytes. */
{int c,s = 0;
 long pos = -1,offset;
 size_t j;
 buff[0] = 0;
 for(offset=0;offset<250000;++offset)
  {c = getc(f);
   switch(c) 
    {case EOF: 
	  return(-1);
     case '#':
      s = '#';
      pos = offset;
      break;
	 case '!':
      s = (s == '#') ? '!' : 0;
      break;
     case '/':
      if (s == '!')
       {while((c = getc(f)) != EOF) if (c == ' ') break;  
        for(j=0;((c = getc(f)) != '\n') && (c != EOF) && (j+1 <= bufflen);++j)
         {buff[j] = c; buff[j+1] = 0;}
        if (strspn(buff," \t\r") == strlen(buff)) buff[0] = 0;
        return(pos);}
      s = 0;
      break;
     default:
      s = 0;
      break;}}
return(-1);}

#ifdef WIN32
char *find_exe_self(char *cmd)
 /* This is for the benefit of WINDOWS NT, which is in fact
    unix compatible in what it passes in as argv[0]. There
    are other ways of getting a handle to the current executable. */
{DWORD retsize;
 char exe_self[512];
 retsize = SearchPath(NULL,cmd,".EXE",sizeof(exe_self),exe_self,NULL);
 if (retsize > 0)
  return(strdup(exe_self));
 else
  return(cmd);}
#endif
  
void __stdcall ulsiod_shuffle_args(int *pargc,char ***pargv)
 /* shuffle arguments in the same way that the unix exec siod_loader
    would do for a #!/xxx script execution. */
{FILE *f;
 char flagbuff[100],**argv,**nargv,offbuff[10];
 long pos;
 int argc,nargc,j,k;
 argc = *pargc;
 argv = *pargv;
#ifdef WIN32
 argv[0] = find_exe_self(argv[0]);
 siod_process_cla(1,argv,1);
#endif
 if (!(f = fopen(argv[0],"rb")))
  {/* perror(argv[0]); */
   return;}
 pos = siod_position_script(f,flagbuff,sizeof(flagbuff));
 fclose(f);
 if (pos < 0) return;
 nargc = argc + ((*flagbuff) ? 2 : 1);
 nargv = (char **) malloc(sizeof(char *) * nargc);
 j = 0;
 nargv[j++] = "siod.exe";
 if (*flagbuff) nargv[j++] = strdup(flagbuff);
 sprintf(offbuff,"%ld",pos);
 nargv[j] = (char *) malloc(strlen(offbuff)+strlen(argv[0])+2);
 sprintf(nargv[j],"%s%c%s",offbuff,VLOAD_OFFSET_HACK_CHAR,argv[0]);
 j++;
 for(k=1;k<argc;++k) nargv[j++] = argv[k];
 *pargc = nargc;
 *pargv = nargv;
 }

siod_LISP siod_lsiod_position_script(siod_LISP lfile)
{FILE *f;
 long iflag,pos;
 char flbuff[100];
 f = siod_get_c_file(lfile,stdin);
 iflag = siod_no_interrupt(1);
 pos = siod_position_script(f,flbuff,sizeof(flbuff));
 siod_no_interrupt(iflag);
 if (pos < 0) return(NIL);
 return(siod_cons(siod_flocons(pos),siod_strcons(-1,flbuff)));}

void __stdcall ulsiod_init(int argc,char **argv)
{ulsiod_process_cla(argc,argv,0);
 siod_init_storage();
 siod_init_subrs();
 siod_init_trace();
 siod_init_slibu();}
 
void __stdcall siod_init_slibu(void)
{long j;
#if defined(unix)
 char *tmp1,*tmp2;
#endif
#if defined(unix) || defined(WIN32)
 tc_opendir = siod_allocate_user_tc();
 siod_set_gc_hooks(tc_opendir,
	      NULL,
	      NULL,
	      NULL,
	      siod_opendir_gc_free,
	      &j);
 siod_set_print_hooks(tc_opendir,siod_opendir_prin1);
 siod_init_subr_2("chmod",siod_l_chmod);
#endif

 siod_gc_protect_sym(&sym_channels,"*channels*");
 siod_setvar(sym_channels,NIL,NIL);
#ifdef WIN32
 siod_init_subr_0("win32-debug",siod_win32_debug);
#endif
#ifdef VMS
 siod_init_subr_1("vms-debug",siod_vms_debug);
 siod_init_lsubr("sys$crembx",siod_lcrembx);
 siod_init_subr_4("lib$set_logical",siod_lset_logical);
#endif
 siod_init_lsubr("system",siod_lsystem);
#ifndef WIN32
 siod_init_subr_0("getgid",siod_lgetgid);
 siod_init_subr_0("getuid",siod_lgetuid);
#endif
#if defined(unix) || defined(WIN32)
 siod_init_subr_0("getcwd",siod_lgetcwd);
#endif
#ifdef unix
 siod_init_subr_2("crypt",siod_lcrypt);
 siod_init_subr_1("getpwuid",siod_lgetpwuid);
 siod_init_subr_1("getpwnam",siod_lgetpwnam);
 siod_init_subr_0("getpwent",siod_lgetpwent);
 siod_init_subr_0("setpwent",siod_lsetpwent);
 siod_init_subr_0("endpwent",siod_lendpwent);
 siod_init_subr_1("setuid",siod_lsetuid);
 siod_init_subr_1("seteuid",siod_lseteuid);
 siod_init_subr_0("geteuid",siod_lgeteuid);
#if defined(__osf__)
 siod_init_subr_1("setpwfile",siod_lsetpwfile);
#endif
 siod_init_subr_2("putpwent",siod_lputpwent);
 siod_init_subr_2("access-problem?",siod_laccess_problem);
 siod_init_subr_3("utime",siod_lutime);
 siod_init_subr_2("fchmod",siod_lfchmod);
#endif
 siod_init_subr_1("random",siod_lrandom);
 siod_init_subr_1("srandom",siod_lsrandom);
 siod_init_subr_1("first",siod_car);
 siod_init_subr_1("rest",siod_cdr);
#ifdef unix
 siod_init_subr_0("fork",siod_lfork);
 siod_init_subr_3("exec",siod_lexec);
 siod_init_subr_1("nice",siod_lnice);
 siod_init_subr_2("wait",siod_lwait);
 siod_init_subr_0("getpgrp",siod_lgetpgrp);
 siod_init_subr_1("getgrgid",siod_lgetgrgid);
 siod_init_subr_2("setpgid",siod_lsetpgid);
 siod_init_subr_2("kill",siod_lkill);
#endif
 siod_init_subr_1("%%%memref",siod_lmemref_byte);
 siod_init_subr_0("getpid",siod_lgetpid);
#ifndef WIN32
 siod_init_subr_0("getppid",siod_lgetppid);
#endif
 siod_init_subr_1("exit",siod_lexit);
 siod_init_subr_1("trunc",siod_ltrunc);
#ifdef unix
 siod_init_subr_1("putenv",siod_lputenv);
#endif
 siod_init_subr_0("md5-init",siod_md5_init);
 siod_init_subr_3("md5-update",siod_md5_update);
 siod_init_subr_1("md5-final",siod_md5_final);
#if defined(__osf__) || defined(sun)
 siod_init_subr_2("cpu-usage-limits",siod_cpu_usage_limits);
#endif
#if defined(__osf__) || defined(SUN5)
 siod_init_subr_1("current-resource-usage",siod_current_resource_usage);
#endif
#if  defined(unix) || defined(WIN32)
 siod_init_subr_1("opendir",siod_l_opendir);
 siod_init_subr_1("closedir",siod_l_closedir);
 siod_init_subr_1("readdir",siod_l_readdir);
#endif
 siod_init_subr_1("delete-file",siod_delete_file);
 siod_init_subr_1("file-times",siod_file_times);
 siod_init_subr_1("unix-time->strtime",siod_utime2str);
 siod_init_subr_0("unix-time",siod_unix_time);
 siod_init_subr_1("unix-ctime",siod_unix_ctime);
 siod_init_subr_1("getenv",siod_lgetenv);
 siod_init_subr_1("sleep",siod_lsleep);
 siod_init_subr_1("url-encode",siod_url_encode);
 siod_init_subr_1("url-decode",siod_url_decode);
 siod_init_subr_2("gets",siod_lgets);
 siod_init_subr_1("siod_readline",siod_readline);
 siod_init_subr_1("html-encode",siod_html_encode);
 siod_init_subr_1("html-decode",siod_html_decode);
#if defined(unix) || defined(WIN32)
 siod_init_subr_1("decode-file-mode",siod_decode_st_mode);
 siod_init_subr_1("encode-file-mode",siod_encode_st_mode);
 siod_init_subr_1("stat",siod_l_stat);
 siod_init_subr_1("fstat",siod_l_fstat);
#endif
#ifdef unix
 siod_init_subr_1("encode-open-flags",siod_encode_open_flags);
 siod_init_subr_1("lstat",siod_l_lstat);
#endif
#if defined(__osf__) || defined(SUN5)
 siod_init_subr_3("fnmatch",siod_l_fnmatch);
#endif
#ifdef unix
 siod_init_subr_2("symlink",siod_lsymlink);
 siod_init_subr_2("link",siod_llink);
 siod_init_subr_1("unlink",siod_lunlink);
 siod_init_subr_1("rmdir",siod_lrmdir);
 siod_init_subr_2("mkdir",siod_lmkdir);
 siod_init_subr_2("rename",siod_lrename);
 siod_init_subr_1("readlink",siod_lreadlink);
#endif
#ifndef WIN32
 siod_init_subr_3("chown",siod_l_chown);
#endif
#if defined(unix) && !defined(linux)
 siod_init_subr_3("lchown",siod_l_lchown);
#endif
 siod_init_subr_1("http-date",siod_http_date);
#if defined(__osf__)
 siod_init_subr_1("http-date-parse",siod_http_date_parse);
#endif
#ifdef unix
 siod_init_subr_2("popen",siod_popen_l);
 siod_init_subr_1("pclose",siod_pclose_l);
#endif
 siod_init_subr_2("siod_load-so",siod_load_so);
 siod_init_subr_1("siod_require-so",siod_require_so);
 siod_init_subr_1("so-ext",siod_so_ext);
#ifdef unix
 siod_setvar(siod_cintern("SEEK_SET"),siod_flocons(SEEK_SET),NIL);
 siod_setvar(siod_cintern("SEEK_CUR"),siod_flocons(SEEK_CUR),NIL);
 siod_setvar(siod_cintern("SEEK_END"),siod_flocons(SEEK_END),NIL);
 siod_setvar(siod_cintern("F_RDLCK"),siod_flocons(F_RDLCK),NIL);
 siod_setvar(siod_cintern("F_WRLCK"),siod_flocons(F_WRLCK),NIL);
 siod_setvar(siod_cintern("F_UNLCK"),siod_flocons(F_UNLCK),NIL);
 siod_init_subr_5("F_SETLK",siod_lF_SETLK);
 siod_init_subr_5("F_SETLKW",siod_lF_SETLKW);
 siod_init_subr_5("F_GETLK",siod_lF_GETLK);

#endif
 siod_init_subr_0("siod-lib",siod_lib_l);

#ifdef unix
 if ((!(tmp1 = getenv(ld_library_path_env))) ||
     (!strstr(tmp1,siod_lib)))
   {tmp2 = (char *) must_malloc(strlen(ld_library_path_env) + 1 +
				((tmp1) ? strlen(tmp1) + 1 : 0) +
				strlen(siod_lib) + 1);
    sprintf(tmp2,"%s=%s%s%s",
	    ld_library_path_env,
	    (tmp1) ? tmp1 : "",
	    (tmp1) ? ":" : "",
	    siod_lib);
    /* note that we cannot free the string afterwards. */
    putenv(tmp2);}
#endif
#ifdef vms
 siod_setvar(siod_cintern("*siod_require-so-dir*"),
	siod_string_append(siod_listn(2,
			    siod_strcons(-1,siod_lib),
			    siod_strcons(-1,".EXE"))),
	NIL);
#endif
 siod_init_subr_1("localtime",siod_llocaltime);
 siod_init_subr_1("gmtime",siod_lgmtime);
#if defined(unix) || defined(WIN32)
 siod_init_subr_0("tzset",siod_ltzset);
#endif
 siod_init_subr_1("mktime",siod_lmktime);
 siod_init_subr_1("chdir",siod_lchdir);
#if defined(__osf__)
 siod_init_subr_0("rld-pathnames",siod_rld_pathnames);
#endif
#if defined(__osf__) || defined(SUN5) || defined(linux)
 siod_init_subr_3("strptime",siod_lstrptime);
#endif
#ifdef unix
 siod_init_subr_2("strftime",siod_lstrftime);
 siod_init_subr_1("getpass",siod_lgetpass);
 siod_init_subr_0("pipe",siod_lpipe);
 siod_init_subr_2("alarm",siod_lalarm);
#endif

 siod_setvar(siod_cintern("CTYPE_FLOAT"),siod_flocons(CTYPE_FLOAT),NIL);
 siod_setvar(siod_cintern("CTYPE_DOUBLE"),siod_flocons(CTYPE_DOUBLE),NIL);
 siod_setvar(siod_cintern("CTYPE_LONG"),siod_flocons(CTYPE_LONG),NIL);
 siod_setvar(siod_cintern("CTYPE_SHORT"),siod_flocons(CTYPE_SHORT),NIL);
 siod_setvar(siod_cintern("CTYPE_CHAR"),siod_flocons(CTYPE_CHAR),NIL);
 siod_setvar(siod_cintern("CTYPE_INT"),siod_flocons(CTYPE_INT),NIL);
 siod_setvar(siod_cintern("CTYPE_ULONG"),siod_flocons(CTYPE_ULONG),NIL);
 siod_setvar(siod_cintern("CTYPE_USHORT"),siod_flocons(CTYPE_USHORT),NIL);
 siod_setvar(siod_cintern("CTYPE_UCHAR"),siod_flocons(CTYPE_UCHAR),NIL);
 siod_setvar(siod_cintern("CTYPE_UINT"),siod_flocons(CTYPE_UINT),NIL);
 siod_init_subr_3("siod_datref",siod_datref);
 siod_init_subr_2("siod_ssiod_datref",siod_ssiod_datref);
 siod_init_subr_2("siod_mksiod_datref",siod_mksiod_datref);
 siod_init_subr_2("siod_datlength",siod_datlength);
 siod_init_subr_1("position-script",siod_lsiod_position_script);

 siod_init_slibu_version();}
