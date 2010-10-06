/* vim: set shiftwidth=2 softtabstop=2: */
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

DL_EXPORT(PyObject) * newPerlPkg_object(PyObject *base, PyObject *pkg);
staticforward void       PerlPkg_dealloc(PerlPkg_object *self);
staticforward PyObject * PerlPkg_repr(PerlPkg_object *self, PyObject *args);
staticforward PyObject * PerlPkg_getattr(PerlPkg_object *self, char *name);

DL_EXPORT(PyObject *) newPerlObj_object(SV *obj, PyObject *pkg);
staticforward void       PerlObj_dealloc(PerlObj_object *self);
staticforward PyObject * PerlObj_repr(PerlObj_object *self, PyObject *args);
staticforward PyObject * PerlObj_getattr(PerlObj_object *self, char *name);
staticforward PyObject * PerlObj_mp_subscript(PerlObj_object *self, PyObject *key);

DL_EXPORT(PyObject *) newPerlSub_object(PyObject *base,
					PyObject *pkg,
					SV *cv);
DL_EXPORT(PyObject *) newPerlMethod_object(PyObject *base,
					   PyObject *pkg,
					   SV *obj);
DL_EXPORT(PyObject *) newPerlCfun_object(PyObject* (*cfun)(PyObject *self, 
							   PyObject *args));
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
  PerlPkg_object *self = PyObject_NEW(PerlPkg_object, &PerlPkg_type);
  char *bs = PyString_AsString(base);
  char *pkg = PyString_AsString(package);
  char *str = (char*)malloc((strlen(bs) + strlen(pkg) + strlen("::") + 1)
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
  self->full = PyString_FromString(str);

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
  char *str;
  str = (char*)malloc((strlen("<perl package: ''>")
		       + PyObject_Length(self->full)
		       + 1) * sizeof(char));
  sprintf(str, "<perl package: '%s'>", PyString_AsString(self->full));
  s = PyString_FromString(str);
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
    PyObject *tmp = PyString_FromString(name);
    char *full_c = PyString_AsString(self->full);
    if (perl_pkg_exists(full_c, name)) {
      return newPerlPkg_object(self->full, tmp);
    }
    else {
      return newPerlSub_object(self->full, tmp, NULL);
    }
  }
}

static struct PyMethodDef PerlPkg_methods[] = {
  {NULL, NULL} /* sentinel */
};

/* doc string */
static char PerlPkg_type__doc__[] = 
"_perl_pkg -- Wrap a Perl package in a Python class"
;

/* type definition */
DL_EXPORT(PyTypeObject) PerlPkg_type = {
  PyObject_HEAD_INIT(NULL)
  0,                            /*ob_size*/
  "_perl_pkg",                  /*tp_name*/
  sizeof(PerlPkg_object),       /*tp_basicsize*/
  0,                            /*tp_itemsize*/
  /* methods */
  (destructor)PerlPkg_dealloc,  /*tp_dealloc*/
  (printfunc)0,                 /*tp_print*/
  (getattrfunc)PerlPkg_getattr, /*tp_getattr*/
  (setattrfunc)0,               /*tp_setattr*/
  (cmpfunc)0,                   /*tp_compare*/
  (reprfunc)PerlPkg_repr,       /*tp_repr*/
  0,                            /*tp_as_number*/
  0,                            /*tp_as_sequence*/
  0,                            /*tp_as_mapping*/
  (hashfunc)0,                  /*tp_hash*/
  (ternaryfunc)0,               /*tp_call*/
  (reprfunc)PerlPkg_repr,       /*tp_str*/

              /* Space for future expansion */
  0L,0L,0L,0L,
  PerlPkg_type__doc__, /* Documentation string */
};

/* methods of _perl_obj */
PyObject *
newPerlObj_object(SV *obj, PyObject *package) {
  PerlObj_object *self = PyObject_NEW(PerlObj_object, &PerlObj_type);

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

  if (self->obj) sv_2mortal(self->obj); // mortal instead of DECREF. Object might be return value

  PyObject_Del(self);
}

static PyObject *
PerlObj_repr(PerlObj_object *self, PyObject *args) {
  PyObject *s;
  char *str;
  str = (char*)malloc((strlen("<perl object: ''>")
		       + PyObject_Length(self->pkg)
		       + 1) * sizeof(char));
  sprintf(str, "<perl object: '%s'>", PyString_AsString(self->pkg));
  s = PyString_FromString(str);
  free(str);
  return s;
}

static PyObject *
PerlObj_getattr(PerlObj_object *self, char *name) {
  PyObject *retval = NULL;
  if (strcmp(name,"__methods__") == 0) {
    return get_perl_pkg_subs(self->pkg);
  }
  else if (strcmp(name,"__members__") == 0) {
    PyObject *retval = PyList_New(0);
    return retval ? retval : NULL;
  }
  else if (strcmp(name,"__dict__") == 0) {
    PyObject *retval = PyDict_New();
    return retval ? retval : NULL;
  }
  else {
    SV *obj = (SV*)SvRV(self->obj);
    HV* pkg = SvSTASH(obj);
    /* probably a request for a method */
    GV* const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, name, TRUE);
    if (gv && isGV(gv)) {
      PyObject *py_name = PyString_FromString(name);
      retval = newPerlMethod_object(self->pkg, py_name, self->obj);
    }
    else {
      /* search for an attribute */
      // check if the object supports the __getattr__ protocol
      GV* const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, "__getattr__", FALSE);
      if (gv && isGV(gv)) { // __getattr__ supported! Let's see if an attribute is found.
	dSP;

	ENTER;
	SAVETMPS;

	SV* rv = sv_2mortal(newRV((SV*)GvCV(gv)));

	PUSHMARK(SP);
	XPUSHs(self->obj);
	XPUSHs(sv_2mortal(newSVpv(name, 0)));
	PUTBACK;

	/* array context needed, so it's possible to return nothing (not even undef)
	   if the attribute does not exist */
	int count = call_sv(rv, G_ARRAY);

	SPAGAIN;

	if (count > 1)
	  croak("__getattr__ may only return a single scalar or an empty list!\n");

	if (count == 1) { // attribute exists! Now give the value back to Python
	  retval = Pl2Py(POPs);
	}

	FREETMPS;
	LEAVE;
      }
      if (! retval) { // give up and raise a KeyError
        char attribute_error[strlen(name) + 21];
        sprintf(attribute_error, "attribute %s not found", name);
        PyErr_SetString(PyExc_KeyError, attribute_error);
      }
    }
    return retval;
  }
}

static PyObject*
PerlObj_mp_subscript(PerlObj_object *self, PyObject *key) {
  // check if the object supports the __getitem__ protocol
  PyObject *item = NULL;
  char *name = PyString_AsString(PyObject_Str(key));
  SV *obj = (SV*)SvRV(self->obj);
  HV* pkg = SvSTASH(obj);
  GV* const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, "__getitem__", FALSE);
  if (gv && isGV(gv)) { // __getitem__ supported! Let's see if the key is found.
    dSP;

    ENTER;
    SAVETMPS;

    SV* rv = sv_2mortal(newRV((SV*)GvCV(gv)));

    PUSHMARK(SP);
    XPUSHs(self->obj);
    XPUSHs(sv_2mortal(newSVpv(name, 0)));
    PUTBACK;

    /* array context needed, so it's possible to return nothing (not even undef)
       if the attribute does not exist */
    int count = call_sv(rv, G_ARRAY);

    SPAGAIN;

    if (count > 1)
      croak("__getitem__ may only return a single scalar or an empty list!\n");

    if (count == 1) { // item exists! Now give the value back to Python
      item = Pl2Py(POPs);
    }

    FREETMPS;
    LEAVE;

    if (count == 0) {
      char attribute_error[strlen(name) + 21];
      sprintf(attribute_error, "attribute %s not found", name);
      PyErr_SetString(PyExc_KeyError, attribute_error);
    }
  }
  else {
    PyErr_Format(PyExc_TypeError, "'%.200s' object is unsubscriptable", self->ob_type->tp_name);
  }
  return item;
}

static int
PerlObj_compare(PerlObj_object *o1, PerlObj_object *o2) {
  if (SvRV(o1->obj) == SvRV(o2->obj)) // just compare the dereferenced object pointers
    return 0;
  return 1;
}

static struct PyMethodDef PerlObj_methods[] = {
  {NULL, NULL} /* sentinel */
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
DL_EXPORT(PyTypeObject) PerlObj_type = {
  PyObject_HEAD_INIT(NULL)
  0,                            /*ob_size*/
  "_perl_obj",                  /*tp_name*/
  sizeof(PerlObj_object),       /*tp_basicsize*/
  0,                            /*tp_itemsize*/
  /* methods */
  (destructor)PerlObj_dealloc,  /*tp_dealloc*/
  (printfunc)0,                 /*tp_print*/
  (getattrfunc)PerlObj_getattr, /*tp_getattr*/
  (setattrfunc)0,               /*tp_setattr*/
  (cmpfunc)PerlObj_compare,     /*tp_compare*/
  (reprfunc)PerlObj_repr,       /*tp_repr*/
  0,                            /*tp_as_number*/
  0,                            /*tp_as_sequence*/
  &mp_methods,                  /*tp_as_mapping*/
  (hashfunc)0,                  /*tp_hash*/
  (ternaryfunc)0,               /*tp_call*/
  (reprfunc)PerlObj_repr,       /*tp_str*/

              /* Space for future expansion */
  0L,0L,0L,0L,
  PerlObj_type__doc__, /* Documentation string */
};

/* methods of _perl_sub */
PyObject *
newPerlSub_object(PyObject *package, PyObject *sub, SV *cv) {
  PerlSub_object *self = PyObject_NEW(PerlSub_object, &PerlSub_type);
  char *str = NULL;

  if(!self) {
    PyErr_Format(PyExc_MemoryError, "Couldn't create Perl Sub object.\n");
    return NULL;
  }

  /* initialize the name of the sub or method */
  if (package && sub) {
    str = malloc((PyObject_Length(package) + PyObject_Length(sub) + 1)
		 *sizeof(char));
    
    sprintf(str, "%s%s", PyString_AsString(package),
	    PyString_AsString(sub));
    
    Py_INCREF(sub);
    Py_INCREF(package);
    self->sub = sub;
    self->pkg = package;
    self->full = PyString_FromString(str);
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
  PerlSub_object *self = (PerlSub_object*)newPerlSub_object(package, 
							    sub, NULL);
  self->obj = obj;
  SvREFCNT_inc(obj);
  return (PyObject*)self;
}

PyObject * newPerlCfun_object(PyObject* (*cfun)(PyObject *self, 
						PyObject *args)) 
{
  PerlSub_object *self = PyObject_NEW(PerlSub_object, &PerlSub_type);
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
  int len = PyObject_Length(args);
  int count;
  PyObject *retval;

  /* if this wraps a C function, execute that */
  if (self->cfun) return self->cfun((PyObject*)self, args);

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);

  if (self->obj) XPUSHs(self->obj);

  if (kw) { /* if keyword arguments are present, positional arguments get pushed as into an arrayref */
    AV *positional = newAV();
    for (i=0; i<len; i++) {
      SV *arg = Py2Pl(PyTuple_GetItem(args, i));
      av_push(positional, SvREFCNT_inc(arg));
    }
    XPUSHs((SV *) sv_2mortal((SV *) newRV_inc((SV *) positional)));

    SV *kw_hash = Py2Pl(kw);
    XPUSHs(kw_hash);
    sv_2mortal(kw_hash);
    sv_2mortal((SV *)positional);
  }
  else {
    for (i=0; i<len; i++) {
      SV *arg = Py2Pl(PyTuple_GetItem(args, i));
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
    count = perl_call_sv(self->ref, self->flgs);
  else if (self->sub && self->obj)
    count = perl_call_method(PyString_AsString(self->sub), self->flgs);
  else {
    croak("Error: PerlSub called, but no C function, sub, or name found!\n");
  }
  
  Py_DECREF(self); /* release*/
  
  SPAGAIN;

  if (SvTRUE(ERRSV)) {
    warn("%s\n", SvPV_nolen(ERRSV));
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
    AV *lst = newAV();
    av_extend(lst, count);
    for (i = count - 1; i >= 0; i--) {
      av_store(lst, i, SvREFCNT_inc(POPs));
    }
    SV *rv_lst = newRV_inc((SV*)lst);
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
  char *str;
  str = (char*)malloc((strlen("<perl sub: ''>")
		       + (self->full 
			  ? PyObject_Length(self->full) 
			  : strlen("anonymous"))
		       + 1) * sizeof(char));
  sprintf(str, "<perl sub: '%s'>", (self->full 
				    ? PyString_AsString(self->full)
				    : "anonymous"));
  s = PyString_FromString(str);
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
		 (self->full
		 	? PyString_AsString(self->full)
			: (self->pkg ? PyString_AsString(self->pkg) : ""))
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
		 (self->pkg ? PyString_AsString(self->pkg) : ""));
    return -1;  /* failure */
  }
  else {
    PyErr_Format(PyExc_AttributeError,
		 "Attribute '%s' not found for Perl sub '%s'", name,
		 (self->full
		 	? PyString_AsString(self->full)
			: (self->pkg ? PyString_AsString(self->pkg) : ""))
		);
    return -1;  /* failure */
  }
}

static struct PyMethodDef PerlSub_methods[] = {
  {NULL, NULL} /* sentinel */
};

/* doc string */
static char PerlSub_type__doc__[] = 
"_perl_sub -- Wrap a Perl sub in a Python class"
;

/* type definition */
DL_EXPORT(PyTypeObject) PerlSub_type = {
  PyObject_HEAD_INIT(NULL)
  0,                            /*ob_size*/
  "_perl_sub",                  /*tp_name*/
  sizeof(PerlSub_object),       /*tp_basicsize*/
  0,                            /*tp_itemsize*/
  /* methods */
  (destructor)PerlSub_dealloc,  /*tp_dealloc*/
  (printfunc)0,                 /*tp_print*/
  (getattrfunc)PerlSub_getattr, /*tp_getattr*/
  (setattrfunc)PerlSub_setattr, /*tp_setattr*/
  (cmpfunc)0,                   /*tp_compare*/
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
  PyObject *s = PyTuple_GetItem(args, 0);

  if (!PyString_Check(s)) {
    return NULL;
  }

  ENTER;
  SAVETMPS;

  /* not necessary -- but why not? */
  PUSHMARK(SP);
  PUTBACK;

  /* run the anonymous subroutine under G_EVAL mode */
  code = newSVpv(PyString_AsString(s),0);
  count = perl_eval_sv(code, G_EVAL);

  SPAGAIN;

  if (SvTRUE(ERRSV)) {
    warn("%s\n", SvPV_nolen(ERRSV));
  }

  if (count == 0) {
    retval = Py_None;
    Py_INCREF(retval);
  }
  else if (count == 1) {
    SV* s = POPs;
    retval = Pl2Py(s);
  }
  else {
    AV *lst = newAV();
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
  PyObject *s = PyTuple_GetItem(args, 0);
  char *str;

  if(!PyString_Check(s)) {
    return NULL;
  }

  Printf(("calling use...'%s'\n", PyString_AsString(s)));

  str = malloc((strlen("use ")
		+ PyObject_Length(s)) * sizeof(char));
  sprintf(str, "use %s", PyString_AsString(s));

  Printf(("eval-ing now!\n"));
  perl_eval_pv(str, TRUE);
  Printf(("'twas called!\n"));

  free(str);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * special_perl_require(PyObject *ignored, PyObject *args) {
  PyObject *s = PyTuple_GetItem(args, 0);

  if (!PyString_Check(s)) 
    return NULL;

  perl_require_pv(PyString_AsString(s));

  Py_INCREF(Py_None);
  return Py_None;
}

#ifdef CREATE_PERL
static void
create_perl()
{
  int argc = 1;
  char *argv[] = {
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

DL_EXPORT(void)
initperl(void)
{
  PyObject *m, *d, *p;
  PyObject *dummy1 = PyString_FromString(""), 
           *dummy2 = PyString_FromString("main");

  /* Initialize the type of the new type objects here; doing it here
   * is required for portability to Windows without requiring C++. */
  PerlPkg_type.ob_type = &PyType_Type;
  PerlObj_type.ob_type = &PyType_Type;
  PerlSub_type.ob_type = &PyType_Type;

  /* Create the module and add the functions */
  m = Py_InitModule4("perl", 
		     perl_functions, 
		     "perl -- Access a Perl interpreter transparently", 
		     (PyObject*)NULL, 
		     PYTHON_API_VERSION);

  /* Now replace the package 'perl' with the 'perl' object */
  m = PyImport_AddModule("sys");
  d = PyModule_GetDict(m);
  d = PyDict_GetItemString(d, "modules");
  p = newPerlPkg_object(dummy1, dummy2);
  PyDict_SetItemString(d, "perl", p);

#ifdef CREATE_PERL
  create_perl();
#endif

  Py_DECREF(dummy1);
  Py_DECREF(dummy2);
}
