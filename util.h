#ifndef __INL_PY_UTILS__
#define __INL_PY_UTILS__
#ifdef __cplusplus
extern "C" {
#endif

/* Before Perl 5.6, this didn't exist */
#ifndef SvPV_nolen
#define SvPV_nolen(sv) SvPV(sv,PL_na)
#endif

#ifndef pTHX_
# define pTHX_
# define aTHX_
# define pTHX
# define aTHX
#endif

#ifdef I_PY_DEBUG
#define Printf(x) printf x
#else
#define Printf(x)
#endif

/* This structure is used to distinguish Python objects from regular
 * Perl objects. It could also be used to store additional information
 * about the objects, if necessary. It is a private area -- untouchable
 * from perl-space.
 */
typedef struct {
  I32 key; /* to make sure it came from Inline */
} _inline_magic;

#define INLINE_MAGIC_KEY 0x0DD515FD
#define Inline_Magic_Key(mg_ptr) (((_inline_magic*)mg_ptr)->key)
#define Inline_Magic_Check(mg_ptr) (Inline_Magic_Key(mg_ptr)==INLINE_MAGIC_KEY)

extern DL_IMPORT(PyObject *) get_perl_pkg_subs(PyObject *);
extern DL_IMPORT(int)	     perl_pkg_exists(char *, char *);
extern DL_IMPORT(PyObject *) perl_sub_exists(PyObject *, PyObject *);

/* This is called when Perl deallocates a PerlObj object */
extern int free_inline_py_obj(pTHX_ SV* obj, MAGIC *mg);

#ifdef __cplusplus
}
#endif
#endif
