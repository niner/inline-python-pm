/* vim: set expandtab shiftwidth=4 softtabstop=4 cinoptions='\:2=2': */
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

extern PyTypeObject PerlPkg_type, PerlObj_type, PerlSub_type;

#ifndef PyVarObject_HEAD_INIT  /* Python 2.5 does not define this*/
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

#ifndef Py_TYPE /* Python 2.5 does not define this*/
#define Py_TYPE(ob) 			(((PyObject*)(ob))->ob_type)
#endif

#define PerlPkgObject_Check(v) (Py_TYPE(v) == &PerlPkg_type)
#define PerlObjObject_Check(v) (Py_TYPE(v) == &PerlObj_type)
#define PerlSubObject_Check(v) (Py_TYPE(v) == &PerlSub_type)

#define PKG_EQ(obj,pkg) (strcmp(PyString_AsString((obj)->full), (pkg))==0)

/***************************************
 *         METHOD DECLARATIONS         *
 ***************************************/

/* methods of _perl_pkg */
extern PyObject * newPerlPkg_object(PyObject *, PyObject *);

/* methods of _perl_obj */
extern PyObject * newPerlObj_object(SV *, PyObject *);

/* methods of _perl_sub */
extern PyObject * newPerlSub_object(PyObject *, PyObject *, SV *);
extern PyObject * newPerlMethod_object(PyObject*,PyObject*,SV*);
extern PyObject * newPerlCfun_object(PyObject* (*)(PyObject *,
							      PyObject *));

#ifdef __cplusplus
}
#endif
#endif
