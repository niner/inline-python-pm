/* vim: set shiftwidth=4 softtabstop=4 expandtab: */
#ifdef __cplusplus
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "Python.h"
#include "perlmodule.h"
#include "py2pl.h"
#include "util.h"
#ifdef __cplusplus
}
#endif

#ifdef CREATE_PERL
static PerlInterpreter *my_perl;
#endif

staticforward PyObject * special_perl_eval(PyObject *, PyObject *);
staticforward PyObject * special_perl_use(PyObject *, PyObject *);
staticforward PyObject * special_perl_require(PyObject *, PyObject *);

/***************************************
 *         METHOD DECLARATIONS         *
 ***************************************/

PyObject * newPerlPkg_object(PyObject *base, PyObject *pkg);
staticforward void       PerlPkg_dealloc(PerlPkg_object *self);
staticforward PyObject * PerlPkg_repr(PerlPkg_object *self, PyObject *args);
staticforward PyObject * PerlPkg_getattr(PerlPkg_object *self, char *name);

PyObject * newPerlObj_object(SV *obj, PyObject *pkg);
staticforward void       PerlObj_dealloc(PerlObj_object *self);
staticforward PyObject * PerlObj_repr(PerlObj_object *self);
staticforward PyObject * PerlObj_str(PerlObj_object *self);
staticforward PyObject * PerlObj_call(PerlObj_object *self, PyObject *args, PyObject *kw);
staticforward PyObject * PerlObj_getattr(PerlObj_object *self, char *name);
staticforward PyObject * PerlObj_mp_subscript(PerlObj_object *self, PyObject *key);

PyObject * newPerlSub_object(PyObject *base,
                             PyObject *pkg,
                             SV *cv);
PyObject * newPerlMethod_object(PyObject *base,
                                PyObject *pkg,
                                SV *obj);
PyObject * newPerlCfun_object(PyObject* (*cfun)(PyObject *self, PyObject *args));
staticforward void       PerlSub_dealloc(PerlSub_object *self);
staticforward PyObject * PerlSub_call(PerlSub_object *self, PyObject *args, PyObject *kw);
staticforward PyObject * PerlSub_repr(PerlSub_object *self, PyObject *args);
staticforward PyObject * PerlSub_getattr(PerlSub_object *self, char *name);
staticforward int PerlSub_setattr(PerlSub_object *self, 
                                  char *name, 
                                  PyObject *value);

/**************************************
 *         METHOD DEFINITIONS         *
 **************************************/

/* methods of _perl_pkg */
PyObject *
newPerlPkg_object(PyObject *base, PyObject *package) {
    PerlPkg_object * const self = PyObject_NEW(PerlPkg_object, &PerlPkg_type);

#if PY_MAJOR_VERSION >= 3
    char * const bs = PyBytes_AsString(base);
    char * const pkg = PyBytes_AsString(package);
#else
    char * const bs = PyString_AsString(base);
    char * const pkg = PyString_AsString(package);
#endif
    char * const str = (char*)malloc((strlen(bs) + strlen(pkg) + strlen("::") + 1)
            * sizeof(char));

    if(!self) {
        free(str); 
        PyErr_Format(PyExc_MemoryError, "Couldn't create Perl Package object.\n");
        return NULL; 
    }
    sprintf(str, "%s%s::", bs, pkg);

    Py_INCREF(base);
    Py_INCREF(package);
    self->base = base;
    self->pkg = package;
#if PY_MAJOR_VERSION >= 3
    self->full = PyBytes_FromString(str);
#else
    self->full = PyString_FromString(str);
#endif

    free(str);
    return (PyObject*)self;
}

static void
PerlPkg_dealloc(PerlPkg_object *self) {
    Py_XDECREF(self->pkg);
    Py_XDECREF(self->base);
    Py_XDECREF(self->full);
    PyObject_Del(self);
}

static PyObject *
PerlPkg_repr(PerlPkg_object *self, PyObject *args) {
    PyObject *s;
    char * const str = (char*)malloc((strlen("<perl package: ''>")
                + PyObject_Length(self->full)
                + 1) * sizeof(char));
#if PY_MAJOR_VERSION >= 3
    sprintf(str, "<perl package: '%s'>", PyBytes_AsString(self->full));
    s = PyUnicode_FromString(str);
#else
    sprintf(str, "<perl package: '%s'>", PyString_AsString(self->full));
    s = PyString_FromString(str);
#endif
    free(str);
    return s;
}

static PyObject *
PerlPkg_getattr(PerlPkg_object *self, char *name) {
    /*** Python Methods ***/
    if (strcmp(name,"__methods__") == 0) {
        return get_perl_pkg_subs(self->full);
    }
    else if (strcmp(name,"__members__") == 0) {
        PyObject *retval = PyList_New(0);
        return retval ? retval : NULL;
    }
    else if (strcmp(name,"__dict__") == 0) {
        PyObject *retval = PyDict_New();
        return retval ? retval : NULL;
    }

    /*** Special Names (but only for 'main' package) ***/
    else if (PKG_EQ(self, "main::") && strcmp(name,"eval")==0) {
        /* return a PerlSub_object which just does: eval(@_) */
        return newPerlCfun_object(&special_perl_eval);
    }
    else if (PKG_EQ(self, "main::") && strcmp(name,"use")==0) {
        /* return a PerlSub_object which just does: 
         * eval("use $_[0]; $_[0]->import") */
        return newPerlCfun_object(&special_perl_use);
    }
    else if (PKG_EQ(self, "main::") && strcmp(name,"require")==0) {
        /* return a PerlSub_object which just does:
         * eval("require $_[0];") */
        return newPerlCfun_object(&special_perl_require);
    }

    /*** A Perl Package, Sub, or Method ***/
    else {
#if PY_MAJOR_VERSION >= 3
        PyObject * const tmp = PyBytes_FromString(name);
        char * const full_c = PyBytes_AsString(self->full);
#else
        PyObject * const tmp = PyString_FromString(name);
        char * const full_c = PyString_AsString(self->full);
#endif

        PyObject * const res = perl_pkg_exists(full_c, name)
            ? newPerlPkg_object(self->full, tmp)
            : newPerlSub_object(self->full, tmp, NULL);

        Py_DECREF(tmp);

        return res;
    }
}

static PyObject * module_dir(PerlPkg_object *self, PyObject *args) {
    return get_perl_pkg_subs(self->full);
}

static struct PyMethodDef PerlPkg_methods[] = {
    {"__dir__", (PyCFunction)module_dir, METH_NOARGS, NULL},
    {NULL} /* sentinel */
};

/* doc string */
static char PerlPkg_type__doc__[] = 
"_perl_pkg -- Wrap a Perl package in a Python class"
;

/* type definition */
PyTypeObject PerlPkg_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_perl_pkg",                  /*tp_name*/
    sizeof(PerlPkg_object),       /*tp_basicsize*/
    0,                            /*tp_itemsize*/
    /* methods */
    (destructor)PerlPkg_dealloc,  /*tp_dealloc*/
    (printfunc)0,                 /*tp_print*/
    (getattrfunc)PerlPkg_getattr, /*tp_getattr*/
    (setattrfunc)0,               /*tp_setattr*/
#if PY_MAJOR_VERSION < 3
    (cmpfunc)0,                   /*tp_compare*/
#else
    0,                            /*reserved*/
#endif
    (reprfunc)PerlPkg_repr,       /*tp_repr*/
    0,                            /*tp_as_number*/
    0,                            /*tp_as_sequence*/
    0,                            /*tp_as_mapping*/
    (hashfunc)0,                  /*tp_hash*/
    (ternaryfunc)0,               /*tp_call*/
    (reprfunc)PerlPkg_repr,       /*tp_str*/
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    PerlPkg_type__doc__, /* Documentation string */
    (traverseproc)0,           /* tp_traverse */
    (inquiry)0,                /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    PerlPkg_methods,           /* tp_methods */
};

/* methods of _perl_obj */
PyObject *
newPerlObj_object(SV *obj, PyObject *package) {
    PerlObj_object * const self = PyObject_NEW(PerlObj_object, &PerlObj_type);

    if(!self) {
        PyErr_Format(PyExc_MemoryError, "Couldn't create Perl Obj object.\n");
        return NULL; 
    }

    Py_INCREF(package);
    SvREFCNT_inc(obj);
    self->pkg = package;
    self->obj = obj;

    return (PyObject*)self;
}

static void
PerlObj_dealloc(PerlObj_object *self) {
    Py_XDECREF(self->pkg);

    if (self->obj) sv_2mortal(self->obj); /* mortal instead of DECREF. Object might be return value */

    PyObject_Del(self);
}

static PyObject *
PerlObj_repr(PerlObj_object *self) {
    PyObject *s;
    char * const str = (char*)malloc((strlen("<perl object: ''>")
                + PyObject_Length(self->pkg)
                + 1) * sizeof(char));
#if PY_MAJOR_VERSION >= 3
    sprintf(str, "<perl object: '%s'>", PyBytes_AsString(self->pkg));
    s = PyUnicode_FromString(str);
#else
    sprintf(str, "<perl object: '%s'>", PyString_AsString(self->pkg));
    s = PyString_FromString(str);
#endif
    free(str);
    return s;
}

static PyObject *
PerlObj_str(PerlObj_object *self) {
    STRLEN len;
    SV* const sv = ((SvTHINKFIRST(self->obj) && !SvIsCOW(self->obj)) || isGV_with_GP(self->obj))
        ? sv_mortalcopy(self->obj)
        : self->obj;

    char * const str = SvPVutf8(sv, len);
    return PyUnicode_DecodeUTF8(str, len, "replace");
}

static PyObject *
PerlObj_getattr(PerlObj_object *self, char *name) {
    PyObject *retval = NULL;
    if (strcmp(name,"__methods__") == 0) {
        return get_perl_pkg_subs(self->pkg);
    }
    else if (strcmp(name,"__members__") == 0) {
        retval = PyList_New(0);
        return retval ? retval : NULL;
    }
    else if (strcmp(name,"__dict__") == 0) {
        retval = PyDict_New();
        return retval ? retval : NULL;
    }
    else {
        SV * const obj = (SV*)SvRV(self->obj);
        HV * const pkg = SvSTASH(obj);
        /* probably a request for a method */
        GV * const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, name, TRUE);
        if (gv && isGV(gv)) {
#if PY_MAJOR_VERSION >= 3
            PyObject * const py_name = PyBytes_FromString(name);
#else
            PyObject * const py_name = PyString_FromString(name);
#endif
            retval = newPerlMethod_object(self->pkg, py_name, self->obj);
            Py_DECREF(py_name);
        }
        else {
            /* search for an attribute */
            /* check if the object supports the __getattr__ protocol */
            GV* const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, "__getattr__", FALSE);
            if (gv && isGV(gv)) { /* __getattr__ supported! Let's see if an attribute is found. */
                dSP;

                ENTER;
                SAVETMPS;

                SV * const rv = sv_2mortal(newRV((SV*)GvCV(gv)));

                PUSHMARK(SP);
                XPUSHs(self->obj);
                XPUSHs(sv_2mortal(newSVpv(name, 0)));
                PUTBACK;

                /* array context needed, so it's possible to return nothing (not even undef)
                   if the attribute does not exist */
                int const count = call_sv(rv, G_ARRAY);

                SPAGAIN;

                if (count > 1)
                    croak("__getattr__ may only return a single scalar or an empty list!\n");

                if (count == 1) { /* attribute exists! Now give the value back to Python */
                    retval = Pl2Py(POPs);
                }

                PUTBACK;
                FREETMPS;
                LEAVE;
            }
            if (! retval) { /* give up and raise a AttributeError */
                char attribute_error[strlen(name) + 21];
                sprintf(attribute_error, "attribute %s not found", name);
                PyErr_SetString(PyExc_AttributeError, attribute_error);
            }
        }
        return retval;
    }
}

static PyObject*
PerlObj_mp_subscript(PerlObj_object *self, PyObject *key) {
    /* check if the object supports the __getitem__ protocol */
    PyObject *item = NULL;
    PyObject *key_str = PyObject_Str(key);  /* new reference */
#if PY_MAJOR_VERSION >= 3
    PyObject* string_as_bytes = PyUnicode_AsUTF8String(key_str);/* new reference */
    char * const name = PyBytes_AsString(string_as_bytes);
#else
    char * const name = PyString_AsString(key_str);
#endif
    SV * const obj = (SV*)SvRV(self->obj);
    HV * const pkg = SvSTASH(obj);
    GV* const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, "__getitem__", FALSE);
    if (gv && isGV(gv)) { /* __getitem__ supported! Let's see if the key is found. */
        dSP;

        ENTER;
        SAVETMPS;

        SV * const rv = sv_2mortal(newRV((SV*)GvCV(gv)));

        PUSHMARK(SP);
        XPUSHs(self->obj);
        XPUSHs(sv_2mortal(newSVpv(name, 0)));
        PUTBACK;

        /* array context needed, so it's possible to return nothing (not even undef)
           if the attribute does not exist */
        int const count = call_sv(rv, G_ARRAY);

        SPAGAIN;

        if (count > 1)
            croak("__getitem__ may only return a single scalar or an empty list!\n");

        if (count == 1) { /* item exists! Now give the value back to Python */
            item = Pl2Py(POPs);
        }

        PUTBACK;
        FREETMPS;
        LEAVE;

        if (count == 0) {
            char attribute_error[strlen(name) + 21];
            sprintf(attribute_error, "attribute %s not found", name);
            PyErr_SetString(PyExc_KeyError, attribute_error);
        }
    }
    else {
        PyErr_Format(PyExc_TypeError, "'%.200s' object is unsubscriptable", Py_TYPE(self)->tp_name);
    }
#if PY_MAJOR_VERSION >= 3
    Py_DECREF(string_as_bytes);
#endif
    Py_DECREF(key_str);
    return item;
}

static PyObject *
PerlObj_call(PerlObj_object *self, PyObject *args, PyObject *kw) {
    dSP;
    int i;
    int const len = PyObject_Length(args);
    int count;
    PyObject *retval;

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);

    if (self->obj) XPUSHs(self->obj);

    if (kw) { /* if keyword arguments are present, positional arguments get pushed as into an arrayref */
        AV * const positional = newAV();
        for (i=0; i<len; i++) {
            SV * const arg = Py2Pl(PyTuple_GetItem(args, i));
            av_push(positional, sv_isobject(arg) ? SvREFCNT_inc(arg) : arg);
        }
        XPUSHs((SV *) sv_2mortal((SV *) newRV_inc((SV *) positional)));

        SV * const kw_hash = Py2Pl(kw);
        XPUSHs(kw_hash);
        sv_2mortal(kw_hash);
        sv_2mortal((SV *)positional);
    }
    else {
        for (i=0; i<len; i++) {
            SV * const arg = Py2Pl(PyTuple_GetItem(args, i));
            XPUSHs(arg);
            if (! sv_isobject(arg))
                sv_2mortal(arg);
        }
    }

    PUTBACK;

    /* call the function */
    /* because the Perl sub *could* be arbitrary Python code,
     * I probably should temporarily hold a reference here */
    Py_INCREF(self);

    count = perl_call_sv(self->obj, G_EVAL);
    SPAGAIN;

    Py_DECREF(self); /* release*/


    if (SvTRUE(ERRSV)) {
        PyObject *exc = Pl2Py(ERRSV);
        PyErr_SetObject(PyExc_Perl, exc);
        ERRSV = NULL;
        return NULL;
    }

    /* what to return? */
    if (count == 0) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (count == 1) {
        retval = Pl2Py(POPs);
    }
    else {
        AV * const lst = newAV();
        av_extend(lst, count);
        for (i = count - 1; i >= 0; i--) {
            av_store(lst, i, SvREFCNT_inc(POPs));
        }
        SV * const rv_lst = newRV_inc((SV*)lst);
        retval = Pl2Py(rv_lst);
        SvREFCNT_dec(rv_lst);
        sv_2mortal((SV*)lst); /* this will get killed shortly */
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return retval;
}

#if PY_MAJOR_VERSION >= 3 // Python 3 rich compare
static PyObject*
PerlObj_richcompare(PerlObj_object *o1, PerlObj_object *o2, int op) {
    /* Unable to compare different a Perl object with something else */
    if (!PerlObjObject_Check(o1) || !PerlObjObject_Check(o2)) {
        Py_RETURN_FALSE;
    }

    /* check if the object supports the __cmp__ protocol */
    SV * const obj = (SV*)SvRV(o1->obj);
    HV * const pkg = SvSTASH(obj);

    const char* method_name = NULL;
    switch (op) {
    case Py_LT: method_name = "__lt__"; break;
    case Py_LE: method_name = "__le__"; break;
    case Py_EQ: method_name = "__eq__"; break;
    case Py_NE: method_name = "__ne__"; break;
    case Py_GT: method_name = "__gt__"; break;
    case Py_GE: method_name = "__ge__"; break;
    }

    GV* const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, method_name, FALSE);
    if (gv && isGV(gv)) {
        int retval = 1;
        dSP;

        ENTER;
        SAVETMPS;

        SV * const rv = sv_2mortal(newRV((SV*)GvCV(gv)));

        PUSHMARK(SP);
        XPUSHs(o1->obj);
        XPUSHs(o2->obj);
        PUTBACK;

        int const count = call_sv(rv, G_SCALAR);

        SPAGAIN;

        if (count > 1)
            croak("%s may only return a single scalar!\n", method_name);

        if (count == 1) { /* attribute exists! Now give the value back to Python */
            SV * const result = POPs;
            if(!SvIOK(result))
                croak("%s must return an integer!\n", method_name);
            retval = SvIV(result);
        }

        PUTBACK;
        FREETMPS;
        LEAVE;
        if(retval == 0) {Py_RETURN_TRUE;}
        Py_RETURN_FALSE;
    }
    if (SvRV(o1->obj) == SvRV(o2->obj)) {/* just compare the dereferenced object pointers */
        if(op == Py_EQ) {Py_RETURN_TRUE;}
        Py_RETURN_FALSE;
    }
    if (SvRV(o1->obj) != SvRV(o2->obj)) {
        if(op == Py_NE) {Py_RETURN_TRUE;}
        Py_RETURN_FALSE;
    }
    Py_RETURN_NOTIMPLEMENTED;
}
#else // Python 2 __cmp__ method
static int
PerlObj_compare(PerlObj_object *o1, PerlObj_object *o2) {
    /* check if the object supports the __cmp__ protocol */
    SV * const obj = (SV*)SvRV(o1->obj);
    HV * const pkg = SvSTASH(obj);
    GV* const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, "__cmp__", FALSE);
    if (gv && isGV(gv)) {
        int retval = 1;
        dSP;

        ENTER;
        SAVETMPS;

        SV * const rv = sv_2mortal(newRV((SV*)GvCV(gv)));

        PUSHMARK(SP);
        XPUSHs(o1->obj);
        XPUSHs(o2->obj);
        PUTBACK;

        int const count = call_sv(rv, G_SCALAR);

        SPAGAIN;

        if (count > 1)
            croak("__cmp__ may only return a single scalar!\n");

        if (count == 1) { /* attribute exists! Now give the value back to Python */
            SV * const result = POPs;
            if(!SvIOK(result))
                croak("__cmp__ must return an integer!\n");
            retval = SvIV(result);
        }

        PUTBACK;
        FREETMPS;
        LEAVE;
        return retval;
    }
    if (SvRV(o1->obj) == SvRV(o2->obj)) /* just compare the dereferenced object pointers */
        return 0;
    return 1;
}
#endif

static PyObject * object_dir(PerlObj_object *self, PyObject *args) {
    return get_perl_pkg_subs(self->pkg);
}

static struct PyMethodDef PerlObj_methods[] = {
    {"__dir__", (PyCFunction)object_dir, METH_NOARGS, NULL},
    {NULL} /* sentinel */
};

/* doc string */
static char PerlObj_type__doc__[] = 
"_perl_obj -- Wrap a Perl object in a Python class"
;

PyMappingMethods mp_methods = {
    (lenfunc) 0,                       /*mp_length*/
    (binaryfunc) PerlObj_mp_subscript, /*mp_subscript*/
    (objobjargproc) 0,                 /*mp_ass_subscript*/
};

/* type definition */
PyTypeObject PerlObj_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_perl_obj",                  /*tp_name*/
    sizeof(PerlObj_object),       /*tp_basicsize*/
    0,                            /*tp_itemsize*/
    /* methods */
    (destructor)PerlObj_dealloc,  /*tp_dealloc*/
    (printfunc)0,                 /*tp_print*/
    (getattrfunc)PerlObj_getattr, /*tp_getattr*/
    (setattrfunc)0,               /*tp_setattr*/
#if PY_MAJOR_VERSION < 3
    (cmpfunc)PerlObj_compare,     /*tp_compare*/
#else
    0,                            /*reserved*/
#endif
    (reprfunc)PerlObj_repr,       /*tp_repr*/
    0,                            /*tp_as_number*/
    0,                            /*tp_as_sequence*/
    &mp_methods,                  /*tp_as_mapping*/
    (hashfunc)0,                  /*tp_hash*/
    (ternaryfunc)PerlObj_call,    /*tp_call*/
    (reprfunc)PerlObj_str,        /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    PerlObj_type__doc__, /* Documentation string */
    (traverseproc)0,           /* tp_traverse */
    (inquiry)0,                /* tp_clear */
#if PY_MAJOR_VERSION < 3
    0,                          /* unused */
#else
    (richcmpfunc)PerlObj_richcompare, /* tp_richcompare */
#endif
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    PerlObj_methods,           /* tp_methods */
};

/* methods of _perl_sub */
PyObject *
newPerlSub_object(PyObject *package, PyObject *sub, SV *cv) {
    PerlSub_object * const self = PyObject_NEW(PerlSub_object, &PerlSub_type);
    char *str = NULL;

    if(!self) {
        PyErr_Format(PyExc_MemoryError, "Couldn't create Perl Sub object.\n");
        return NULL;
    }

    /* initialize the name of the sub or method */
    if (package && sub) {
        str = malloc((PyObject_Length(package) + PyObject_Length(sub) + 1)
                *sizeof(char));

#if PY_MAJOR_VERSION >= 3
        sprintf(str, "%s%s", PyBytes_AsString(package),
                PyBytes_AsString(sub));
#else
        sprintf(str, "%s%s", PyString_AsString(package),
                PyString_AsString(sub));
#endif

        Py_INCREF(sub);
        Py_INCREF(package);
        self->sub = sub;
        self->pkg = package;
#if PY_MAJOR_VERSION >= 3
        self->full = PyBytes_FromString(str);
#else
        self->full = PyString_FromString(str);
#endif
    }
    else {
        self->sub = NULL;
        self->pkg = NULL;
        self->full = NULL;
    }

    /* we don't have to check for errors because we shouldn't have been
     * created unless perl_get_cv worked once. 
     */
    if (cv) {
        self->ref = cv;
        self->conf = 1;
    }
    else if (str) {
        self->ref = (SV*)perl_get_cv(str,0); /* can return NULL if not found */
        self->conf = self->ref ? 1 : 0;
    }
    else {
        croak("Can't call newPerlSub_object() with all NULL arguments!\n");
    }

    SvREFCNT_inc(self->ref); /* quite important -- otherwise we lose it */
    self->obj = NULL;
    self->flgs = G_ARRAY;
    self->cfun = 0;

    if (str) free(str);

    return (PyObject*)self;
}

PyObject *
newPerlMethod_object(PyObject *package, PyObject *sub, SV *obj) {
    PerlSub_object * const self = (PerlSub_object*)newPerlSub_object(package,
            sub, NULL);
    self->obj = obj;
    SvREFCNT_inc(obj);
    return (PyObject*)self;
}

PyObject * newPerlCfun_object(PyObject* (*cfun)(PyObject *self, 
            PyObject *args)) 
{
    PerlSub_object * const self = PyObject_NEW(PerlSub_object, &PerlSub_type);
    self->pkg = NULL;
    self->sub = NULL;
    self->full = NULL;
    self->ref = NULL;
    self->obj = NULL;
    self->flgs = 0;
    self->cfun = cfun;
    return (PyObject *)self;
}

static void
PerlSub_dealloc(PerlSub_object *self) {
    Py_XDECREF(self->sub);
    Py_XDECREF(self->pkg);
    Py_XDECREF(self->full);

    if (self->obj) SvREFCNT_dec(self->obj);
    if (self->ref) SvREFCNT_dec(self->ref);

    PyObject_Del(self);
}

static PyObject *
PerlSub_call(PerlSub_object *self, PyObject *args, PyObject *kw) {
    dSP;
    int i;
    int const len = PyObject_Length(args);
    int count;
    PyObject *retval;

    /* if this wraps a C function, execute that */
    if (self->cfun) return self->cfun((PyObject*)self, args);

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);

    if (self->obj) XPUSHs(self->obj);

    if (kw) { /* if keyword arguments are present, positional arguments get pushed as into an arrayref */
        AV * const positional = newAV();
        for (i=0; i<len; i++) {
            SV * const arg = Py2Pl(PyTuple_GetItem(args, i));
            av_push(positional, sv_isobject(arg) ? SvREFCNT_inc(arg) : arg);
        }
        XPUSHs((SV *) sv_2mortal((SV *) newRV_inc((SV *) positional)));

        SV * const kw_hash = Py2Pl(kw);
        XPUSHs(kw_hash);
        sv_2mortal(kw_hash);
        sv_2mortal((SV *)positional);
    }
    else {
        for (i=0; i<len; i++) {
            SV * const arg = Py2Pl(PyTuple_GetItem(args, i));
            XPUSHs(arg);
            if (! sv_isobject(arg))
                sv_2mortal(arg);
        }
    }

    PUTBACK;

    /* call the function */
    /* because the Perl sub *could* be arbitrary Python code,
     * I probably should temporarily hold a reference here */
    Py_INCREF(self);

    if (self->ref)
        count = perl_call_sv(self->ref, self->flgs | G_EVAL);
    else if (self->sub && self->obj)
#if PY_MAJOR_VERSION >= 3
        count = perl_call_method(PyBytes_AsString(self->sub), self->flgs | G_EVAL);
#else
        count = perl_call_method(PyString_AsString(self->sub), self->flgs | G_EVAL);
#endif
    else {
        croak("Error: PerlSub called, but no C function, sub, or name found!\n");
    }
    SPAGAIN;

    Py_DECREF(self); /* release*/


    if (SvTRUE(ERRSV)) {
        PyObject *exc = Pl2Py(ERRSV);
        PyErr_SetObject(PyExc_Perl, exc);
        ERRSV = NULL;
        return NULL;
    }

    /* what to return? */
    if (count == 0) {
        Py_INCREF(Py_None);
        retval = Py_None;
    }
    else if (count == 1) {
        retval = Pl2Py(POPs);
    }
    else {
        AV * const lst = newAV();
        av_extend(lst, count);
        for (i = count - 1; i >= 0; i--) {
            av_store(lst, i, SvREFCNT_inc(POPs));
        }
        SV * const rv_lst = newRV_inc((SV*)lst);
        retval = Pl2Py(rv_lst);
        SvREFCNT_dec(rv_lst);
        sv_2mortal((SV*)lst); /* this will get killed shortly */
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return retval;
}

static PyObject *
PerlSub_repr(PerlSub_object *self, PyObject *args) {
    PyObject *s;
    char * const str = (char*)malloc((strlen("<perl sub: ''>")
                + (self->full
                    ? PyObject_Length(self->full)
                    : strlen("anonymous"))
                + 1) * sizeof(char));
#if PY_MAJOR_VERSION >= 3
    sprintf(str, "<perl sub: '%s'>", (self->full
                ? PyBytes_AsString(self->full)
                : "anonymous"));
    s = PyUnicode_FromString(str);
#else
    sprintf(str, "<perl sub: '%s'>", (self->full
                ? PyString_AsString(self->full)
                : "anonymous"));
    s = PyString_FromString(str);
#endif
    free(str);
    return s;
}

static PyObject *
PerlSub_getattr(PerlSub_object *self, char *name) {
    PyObject *retval = NULL;
    if (strcmp(name,"flags")==0) {
        retval = PyInt_FromLong((long)self->flgs);
    }
    else if (strcmp(name,"G_VOID")==0) {
        retval = PyInt_FromLong((long)G_VOID);
    }
    else if (strcmp(name,"G_SCALAR")==0) {
        retval = PyInt_FromLong((long)G_SCALAR);
    }
    else if (strcmp(name,"G_ARRAY")==0) {
        retval = PyInt_FromLong((long)G_ARRAY);
    }
    else if (strcmp(name,"G_DISCARD")==0) {
        retval = PyInt_FromLong((long)G_DISCARD);
    }
    else if (strcmp(name,"G_NOARGS")==0) {
        retval = PyInt_FromLong((long)G_NOARGS);
    }
    else if (strcmp(name,"G_EVAL")==0) {
        retval = PyInt_FromLong((long)G_EVAL);
    }
    else if (strcmp(name,"G_KEEPERR")==0) {
        retval = PyInt_FromLong((long)G_KEEPERR);
    }
    else {
        PyErr_Format(PyExc_AttributeError,
                "Attribute '%s' not found for Perl sub '%s'", name,
#if PY_MAJOR_VERSION < 3
                (self->full
                 ? PyString_AsString(self->full)
                 : (self->pkg ? PyString_AsString(self->pkg) : ""))
#else
                (self->full
                 ? PyBytes_AsString(self->full)
                 : (self->pkg ? PyBytes_AsString(self->pkg) : ""))
#endif
                );
        retval = NULL;
    }
    return retval;
}

static int
PerlSub_setattr(PerlSub_object *self, char *name, PyObject *v) {
    if (strcmp(name, "flags")==0 && PyInt_Check(v)) {
        self->flgs = (int)PyInt_AsLong(v);
        return 0;  /* success */
    }
    else if (strcmp(name,"flags")==0) {
        PyErr_Format(PyExc_TypeError,
                "'flags' can only be set from an integer. '%s'",
#if PY_MAJOR_VERSION < 3
                (self->pkg ? PyString_AsString(self->pkg) : ""));
#else
                (self->pkg ? PyBytes_AsString(self->pkg) : ""));
#endif

        return -1;  /* failure */
    }
    else {
        PyErr_Format(PyExc_AttributeError,
                "Attribute '%s' not found for Perl sub '%s'", name,
#if PY_MAJOR_VERSION < 3
                (self->full
                 ? PyString_AsString(self->full)
                 : (self->pkg ? PyString_AsString(self->pkg) : ""))
#else
                (self->full
                 ? PyBytes_AsString(self->full)
                 : (self->pkg ? PyBytes_AsString(self->pkg) : ""))
#endif
                );
        return -1;  /* failure */
    }
}

/* doc string */
static char PerlSub_type__doc__[] = 
"_perl_sub -- Wrap a Perl sub in a Python class"
;

/* type definition */
PyTypeObject PerlSub_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_perl_sub",                  /*tp_name*/
    sizeof(PerlSub_object),       /*tp_basicsize*/
    0,                            /*tp_itemsize*/
    /* methods */
    (destructor)PerlSub_dealloc,  /*tp_dealloc*/
    (printfunc)0,                 /*tp_print*/
    (getattrfunc)PerlSub_getattr, /*tp_getattr*/
    (setattrfunc)PerlSub_setattr, /*tp_setattr*/
#if PY_MAJOR_VERSION < 3
    (cmpfunc)0,                   /*tp_compare*/
#else
    0,                            /*reserved*/
#endif
    (reprfunc)PerlSub_repr,       /*tp_repr*/
    0,                            /*tp_as_number*/
    0,                            /*tp_as_sequence*/
    0,                            /*tp_as_mapping*/
    (hashfunc)0,                  /*tp_hash*/
    (ternaryfunc)PerlSub_call,    /*tp_call*/
    (reprfunc)PerlSub_repr,       /*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    PerlSub_type__doc__, /* Documentation string */
};

/* no module-public functions */
static PyMethodDef perl_functions[] = {
    {NULL,              NULL}                /* sentinel */
};

static PyObject * special_perl_eval(PyObject *ignored, PyObject *args) {
    dSP;
    SV *code;
    int i;
    int count;
    PyObject *retval;
    PyObject * const s = PyTuple_GetItem(args, 0);

#if PY_MAJOR_VERSION >= 3
    int is_string = PyBytes_Check(s) || PyUnicode_Check(s);
#else
    int is_string = PyString_Check(s);
#endif
    if(!is_string) {
        return NULL;
    }

    ENTER;
    SAVETMPS;

    /* not necessary -- but why not? */
    PUSHMARK(SP);
    PUTBACK;

    /* run the anonymous subroutine under G_EVAL mode */
#if PY_MAJOR_VERSION >= 3
    PyObject* s_bytes = 0;
    char* s_c_bytes = 0;
    if(PyUnicode_Check(s)) {
        s_bytes = PyUnicode_AsUTF8String(s);
        s_c_bytes = PyBytes_AsString(s_bytes);
    }
    else s_c_bytes = PyBytes_AsString(s);
#else
    char* s_c_bytes = PyString_AsString(s);
#endif

    code = newSVpv(s_c_bytes,0);
    count = perl_eval_sv(code, G_EVAL);

#if PY_MAJOR_VERSION >= 3
    Py_XDECREF(s_bytes);
#endif

    SPAGAIN;

    if (SvTRUE(ERRSV)) {
        warn("%s\n", SvPV_nolen(ERRSV));
    }

    if (count == 0) {
        retval = Py_None;
        Py_INCREF(retval);
    }
    else if (count == 1) {
        SV * const s = POPs;
        retval = Pl2Py(s);
    }
    else {
        AV * const lst = newAV();
        for (i=0; i<count; i++) {
            av_push(lst, POPs);
        }
        retval = Pl2Py((SV*)lst);
        sv_2mortal((SV*)lst);
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return retval;
}

static PyObject * special_perl_use(PyObject *ignored, PyObject *args) {
    PyObject * s = PyTuple_GetItem(args, 0);
    char *str;

#if PY_MAJOR_VERSION >= 3
    int is_string = PyBytes_Check(s) || PyUnicode_Check(s);
#else
    int is_string = PyString_Check(s);
#endif
    if(!is_string) {
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    PyObject* s_bytes = 0;
    char* s_c_bytes = 0;
    if(PyUnicode_Check(s)) {
        s_bytes = PyUnicode_AsUTF8String(s);
        s_c_bytes = PyBytes_AsString(s_bytes);
    }
    else s_c_bytes = PyBytes_AsString(s);
#else
    char* s_c_bytes = PyString_AsString(s);
#endif

    Printf(("calling use...'%s'\n", s_c_bytes));

    str = malloc((strlen("use ")
                + PyObject_Length(s) + 1) * sizeof(char));
    sprintf(str, "use %s", s_c_bytes);

    Printf(("eval-ing now!\n"));
    perl_eval_pv(str, TRUE);
    Printf(("'twas called!\n"));

    free(str);

#if PY_MAJOR_VERSION >= 3
    Py_XDECREF(s_bytes);
#endif

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * special_perl_require(PyObject *ignored, PyObject *args) {
    PyObject * const s = PyTuple_GetItem(args, 0);

#if PY_MAJOR_VERSION >= 3
    int is_string = PyBytes_Check(s) || PyUnicode_Check(s);
#else
    int is_string = PyString_Check(s);
#endif
    if(!is_string) {
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    PyObject* s_bytes = 0;
    char* s_c_bytes = 0;
    if(PyUnicode_Check(s)) {
        s_bytes = PyUnicode_AsUTF8String(s);
        s_c_bytes = PyBytes_AsString(s_bytes);
    }
    else s_c_bytes = PyBytes_AsString(s);
#else
    char* s_c_bytes = PyString_AsString(s);
#endif

    perl_require_pv(s_c_bytes);

#if PY_MAJOR_VERSION >= 3
    Py_XDECREF(s_bytes);
#endif

    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef CREATE_PERL
static void
create_perl()
{
    int argc = 1;
    char * const argv[] = {
        "perl"
    };

    /* When we create a Perl interpreter from Python, we don't get to 
     * dynamically load Perl modules unless that Python is patched, since
     * Python doesn't expose the LDGLOBAL flag, which is required. This
     * problem doesn't exist the other way because Perl exposes this 
     * interface.
     *
     * For this reason I haven't bothered provided an xs_init function.
     */

    my_perl = perl_alloc();
    perl_construct(my_perl);
    perl_parse(my_perl, NULL, argc, argv, NULL);
    perl_run(my_perl);
}
#endif

PyObject *PyExc_Perl;

void
initperl(void){
    PyObject *m, *d, *p;
#if PY_MAJOR_VERSION >= 3
    PyObject *dummy1 = PyBytes_FromString(""),
             *dummy2 = PyBytes_FromString("main");
#else
    PyObject *dummy1 = PyString_FromString(""),
             *dummy2 = PyString_FromString("main");
#endif


    /* Initialize the type of the new type objects here; doing it here
     * is required for portability to Windows without requiring C++. */
#if PY_MAJOR_VERSION >= 3
    PerlPkg_type.ob_base.ob_base.ob_type = &PyType_Type;
    PyType_Ready(&PerlPkg_type);
    PerlObj_type.ob_base.ob_base.ob_type = &PyType_Type;
    PyType_Ready(&PerlObj_type);
    PerlSub_type.ob_base.ob_base.ob_type = &PyType_Type;
    PyType_Ready(&PerlSub_type);
#else
    PerlPkg_type.ob_type = &PyType_Type;
    PerlObj_type.ob_type = &PyType_Type;
    PerlSub_type.ob_type = &PyType_Type;
#endif

    /* Create the module and add the functions */
#if PY_MAJOR_VERSION >= 3
    static struct PyModuleDef perl_module = {
        PyModuleDef_HEAD_INIT,
        "perl",
        "perl -- Access a Perl interpreter transparently",
        -1, /* m_size */
        perl_functions, /* m_methods */
        0, /* m_reload */
        0, /* m_traverse */
        0, /* m_clear */
        0 /* m_free */
    };
    m = PyModule_Create(&perl_module);
#else
    m = Py_InitModule4("perl",
            perl_functions,
            "perl -- Access a Perl interpreter transparently",
            (PyObject*)NULL,
            PYTHON_API_VERSION);
#endif

    /* Now replace the package 'perl' with the 'perl' object */
    m = PyImport_AddModule("sys");
    d = PyModule_GetDict(m);
    d = PyDict_GetItemString(d, "modules");
    p = newPerlPkg_object(dummy1, dummy2);
    PyDict_SetItemString(d, "perl", p);
    Py_DECREF(p);

#ifdef CREATE_PERL
    create_perl();
#endif
    PyExc_Perl = PyErr_NewException("perl.Exception", NULL, NULL);

    Py_DECREF(dummy1);
    Py_DECREF(dummy2);
}
