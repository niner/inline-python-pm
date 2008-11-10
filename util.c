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
  if (mg && mg->mg_type == '~' && Inline_Magic_Check(mg->mg_ptr)) {
    IV iv = SvIV(obj);
    //Printf(("free_inline_py_obj: %p, iv: %p, ob_prev: %p, ob_next: %p, refcnt: %i\n", obj, iv, ((PyObject *)iv)->_ob_prev, ((PyObject *)iv)->_ob_next, ((PyObject *)iv)->ob_refcnt)); // _ob_prev and _ob_next are only available if Python is compiled with reference debugging enabled
    Printf(("free_inline_py_obj: %p, iv: %p, refcnt: %i\n", obj, iv, ((PyObject *)iv)->ob_refcnt));
    Py_XDECREF((PyObject *)iv); /* just in case */
  }
  else {
    croak("ERROR: tried to free a non-Python object. Aborting.");
  }
}

PyObject * get_perl_pkg_subs(PyObject *package) {
  char *pkg = PyString_AsString(package);
  PyObject *retval = PyList_New(0);
  HV *hash = perl_get_hv(pkg, 0);
  int len = hv_iterinit(hash);
  int i;

  for (i=0; i<len; i++) {
    HE *next = hv_iternext(hash);
    I32 n_a;
    char *key = hv_iterkey(next,&n_a);
    char *test = (char*)malloc((strlen(pkg) + strlen(key) + 1)*sizeof(char));
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

  HV* hash = perl_get_hv(base,0);
  char *fpkg = (char*)malloc((strlen(pkg) + strlen("::") + 1)*sizeof(char));
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
  char *pkg = PyString_AsString(package);
  char *sub = PyString_AsString(usub);
  PyObject *retval = Py_None;

  char *qsub = (char*)malloc((strlen(pkg) + strlen(sub) + 1)*sizeof(char));
  sprintf(qsub,"%s%s",pkg,sub);
  
  if (perl_get_cv(qsub,0)) {
    retval = Py_True;
  }

  free(qsub);

  Py_INCREF(retval);
  return retval;
}

