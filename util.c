#ifdef __cplusplus
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "Python.h"
#include "util.h"
#ifdef __cplusplus
}
#endif

/*************************************
 *         UTILITY FUNCTIONS         *
 *************************************/
int free_inline_py_obj(pTHX_ SV* obj, MAGIC *mg)
{
  if (mg && mg->mg_type == PERL_MAGIC_ext && Inline_Magic_Check(mg->mg_ptr)) {
    IV const iv = SvIV(obj);
    /*Printf(("free_inline_py_obj: %p, iv: %p, ob_prev: %p, ob_next: %p, refcnt: %i\n", obj, iv, ((PyObject *)iv)->_ob_prev, ((PyObject *)iv)->_ob_next, ((PyObject *)iv)->ob_refcnt)); */ /* _ob_prev and _ob_next are only available if Python is compiled with reference debugging enabled */
    Printf(("free_inline_py_obj: %p, iv: %p, refcnt: %i\n", obj, iv, ((PyObject *)iv)->ob_refcnt));
    free(mg->mg_virtual); /* allocated in Py2Pl */
    Py_XDECREF((PyObject *)iv); /* just in case */
  }
  else {
    croak("ERROR: tried to free a non-Python object. Aborting.");
  }

  return 0;
}

PyObject * get_perl_pkg_subs(PyObject *package) {
  char * const pkg = PyString_AsString(package);
  PyObject * const retval = PyList_New(0);
  HV * const hash = perl_get_hv(pkg, 0);
  int const len = hv_iterinit(hash);
  int i;

  for (i=0; i<len; i++) {
    HE * const next = hv_iternext(hash);
    I32 n_a;
    char * const key = hv_iterkey(next,&n_a);
    char * const test = (char*)malloc((strlen(pkg) + strlen(key) + 1)*sizeof(char));
    sprintf(test,"%s%s",pkg,key);
    if (perl_get_cv(test,0)) {
      PyList_Append(retval, PyString_FromString(key));
    }
    free(test);
  }

  return retval;
}

int perl_pkg_exists(char *base, char *pkg) {
  int retval = 0;

  HV * const hash = perl_get_hv(base,0);
  char * const fpkg = (char*)malloc((strlen(pkg) + strlen("::") + 1)*sizeof(char));
  sprintf(fpkg,"%s::",pkg);

  Printf(("perl_pkg_exists: %s, %s --> %s\n", base, pkg, fpkg));
  Printf(("perl_pkg_exists: hash=%p\n", hash));

  if (hash && hv_exists(hash, fpkg, strlen(fpkg))) {
    /* here -- check if it's a package, not something else? */
    retval = 1;
  }

  free(fpkg);
  return retval;
}

PyObject * perl_sub_exists(PyObject *package, PyObject *usub) {
  char * const pkg = PyString_AsString(package);
  char * const sub = PyString_AsString(usub);
  PyObject * retval = Py_None;

  char * const qsub = (char*)malloc((strlen(pkg) + strlen(sub) + 1)*sizeof(char));
  sprintf(qsub,"%s%s",pkg,sub);
  
  if (perl_get_cv(qsub,0)) {
    retval = Py_True;
  }

  free(qsub);

  Py_INCREF(retval);
  return retval;
}

int py_is_tuple(SV *arr) {
  if (SvROK(arr) && SvTYPE(SvRV(arr)) == SVt_PVAV) {
    MAGIC * const mg = mg_find(SvRV(arr), PERL_MAGIC_ext);
    return (mg && Inline_Magic_Key(mg->mg_ptr) == TUPLE_MAGIC_KEY);
  }
  else
    return 0;
}
