#ifndef Py_PERLMODULE_H
#define Py_PERLMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

/* _perl_pkg: a class which wraps Perl packages */
typedef struct {
  PyObject_HEAD
  PyObject *base; /* the name of the "parent" package */
  PyObject *pkg;  /* the name of the package */
  PyObject *full; /* the fully-qualified name (base::pkg) */
} PerlPkg_object;

/* _perl_obj: a class which wraps Perl objects */
typedef struct {
  PyObject_HEAD
  PyObject *pkg;  /* the name of the package */
  SV       *obj;  /* the blessed Perl object */
} PerlObj_object;

/* _perl_sub: a class which wraps Perl subs and methods */
typedef struct {
  PyObject_HEAD
  PyObject *pkg;  /* the (fully-qualified) name of the package */
  PyObject *sub;  /* the (unqualified) name of the sub */
  PyObject *full; /* the (fully-qualified) name of the sub */
  SV       *ref;  /* reference to the Perl subroutine (if found) */
  SV       *obj;  /* reference to a Perl object (if a method) */
  int       conf; /* flag: is this sub/method confirmed to exist? */
  I32       flgs; /* flags to pass to perl_call_sv() */
  PyObject* (*cfun)(PyObject *self, PyObject *args); /* a regular Python function */
} PerlSub_object;

extern DL_IMPORT(PyTypeObject) PerlPkg_type, PerlObj_type, PerlSub_type;

#define PerlPkgObject_Check(v) ((v)->ob_type == &PerlPkg_type)
#define PerlObjObject_Check(v) ((v)->ob_type == &PerlObj_type)
#define PerlSubObject_Check(v) ((v)->ob_type == &PerlSub_type)

#define PKG_EQ(obj,pkg) (strcmp(PyString_AsString((obj)->full), (pkg))==0)

/***************************************
 *         METHOD DECLARATIONS         *
 ***************************************/

/* methods of _perl_pkg */
extern DL_IMPORT(PyObject *) newPerlPkg_object(PyObject *, PyObject *);

/* methods of _perl_obj */
extern DL_IMPORT(PyObject *) newPerlObj_object(SV *, PyObject *);

/* methods of _perl_sub */
extern DL_IMPORT(PyObject *) newPerlSub_object(PyObject *, PyObject *, SV *);
extern DL_IMPORT(PyObject *) newPerlMethod_object(PyObject*,PyObject*,SV*);
extern DL_IMPORT(PyObject *) newPerlCfun_object(PyObject* (*)(PyObject *,
							      PyObject *));

#ifdef __cplusplus
}
#endif
#endif
