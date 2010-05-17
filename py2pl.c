#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "Python.h"
#include "py2pl.h"
#include "util.h"

#ifdef EXPOSE_PERL
#include "perlmodule.h"
#endif

/****************************
 * SV* Py2Pl(PyObject *obj) 
 * 
 * Converts arbitrary Python data structures to Perl data structures
 * Note on references: does not Py_DECREF(obj).
 *
 * Modifications by Eric Wilhelm 2004-07-11 marked as elw
 *
 ****************************/
SV *Py2Pl(PyObject * obj) {
	/* elw: see what python says things are */
	PyObject *this_type = PyObject_Type(obj);
	PyObject *t_string = PyObject_Str(this_type);
	int is_string = PyString_Check(obj) || PyUnicode_Check(obj);
	char *type_str = PyString_AsString(t_string);
	Printf(("type is %s\n", type_str));
#ifdef I_PY_DEBUG
	printf("Py2Pl object:\n\t");
	PyObject_Print(obj, stdout, Py_PRINT_RAW);
	printf("\ntype:\n\t");
	PyObject_Print(this_type, stdout, Py_PRINT_RAW);
	printf("\n");
	Printf(("String check:   %i\n", is_string));
	Printf(("Number check:   %i\n", PyNumber_Check(obj)));
	Printf(("Int check:      %i\n", PyInt_Check(obj)));
	Printf(("Long check:     %i\n", PyLong_Check(obj)));
	Printf(("Float check:    %i\n", PyFloat_Check(obj)));
	Printf(("Type check:     %i\n", PyType_Check(obj)));
	Printf(("Instance check: %i\n", PyInstance_Check(obj)));
	Printf(("Dict check:     %i\n", PyDict_Check(obj)));
	Printf(("Mapping check:  %i\n", PyMapping_Check(obj)));
	Printf(("Sequence check: %i\n", PySequence_Check(obj)));
	Printf(("Iter check:     %i\n", PyIter_Check(obj)));
	Printf(("Module check:   %i\n", PyModule_Check(obj)));
	Printf(("Class check:    %i\n", PyClass_Check(obj)));
	Printf(("Method check:   %i\n", PyMethod_Check(obj)));
	if ((obj->ob_type->tp_flags & Py_TPFLAGS_HEAPTYPE))
		printf("heaptype true\n");
	if ((obj->ob_type->tp_flags & Py_TPFLAGS_HAVE_CLASS))
		printf("has class\n");
#endif
	/* elw: this needs to be early */
	/* None (like undef) */
	if (!obj || obj == Py_None) {
		Printf(("Py2Pl: Py_None\n"));
		return &PL_sv_undef;
	}
	else

#ifdef EXPOSE_PERL
	/* unwrap Perl objects */
	if (PerlObjObject_Check(obj)) {
		Printf(("Py2Pl: Obj_object\n"));
		return ((PerlObj_object *) obj)->obj;
	}

	/* unwrap Perl code refs */
	else if (PerlSubObject_Check(obj)) {
		Printf(("Py2Pl: Sub_object\n"));
		return ((PerlSub_object *) obj)->ref;
	}

	else
#endif

	/* wrap an instance of a Python class */
	/* elw: here we need to make these look like instances: */
	if ((obj->ob_type->tp_flags & Py_TPFLAGS_HEAPTYPE) || PyInstance_Check(obj)) {

		/* This is a Python class instance -- bless it into an
		 * Inline::Python::Object. If we're being called from an
		 * Inline::Python class, it will be re-blessed into whatever
		 * class that is.
		 */
		SV *inst_ptr = newSViv(0);
		SV *inst;
		MAGIC *mg;
		_inline_magic priv;

		inst = newSVrv(inst_ptr, "Inline::Python::Object");

		/* set up magic */
		priv.key = INLINE_MAGIC_KEY;
		sv_magic(inst, inst, '~', (char *) &priv, sizeof(priv));
		mg = mg_find(inst, '~');
		mg->mg_virtual = (MGVTBL *) malloc(sizeof(MGVTBL));
		mg->mg_virtual->svt_free = free_inline_py_obj;

		sv_setiv(inst, (IV) obj);
		/*SvREADONLY_on(inst); *//* to uncomment this means I can't
			re-bless it */
		Py_INCREF(obj);
		Printf(("Py2Pl: Instance. Obj: %p, inst_ptr: %p\n", obj, inst_ptr));

		sv_2mortal(inst_ptr);
		return inst_ptr;
	}

	/* a tuple or a list */
	else if (PySequence_Check(obj) && !is_string) {
		AV *retval = newAV();
		int i;
		int sz = PySequence_Length(obj);

		Printf(("sequence (%i)\n", sz));

		for (i = 0; i < sz; i++) {
			PyObject *tmp = PySequence_GetItem(obj, i);	/* new reference */
			SV *next = Py2Pl(tmp);
			av_push(retval, next);
			SvREFCNT_inc(next);
			Py_DECREF(tmp);
		}
		return newRV_noinc((SV *) retval);
	}

	/* a dictionary or fake Mapping object */
	/* elw: PyMapping_Check() now returns true for strings */
	else if (! is_string && PyMapping_Check(obj)) {
		HV *retval = newHV();
		int i;
		int sz = PyMapping_Length(obj);
		PyObject *keys = PyMapping_Keys(obj);   /* new reference */
		PyObject *vals = PyMapping_Values(obj); /* new reference */

		Printf(("Py2Pl: dict/map\n"));
		Printf(("mapping (%i)\n", sz));

		for (i = 0; i < sz; i++) {
			PyObject *key, *val;
			SV *sv_val;
			char *key_val;

			Printf(("working on map item  %i\n", i));
			key = PySequence_GetItem(keys, i); /* new reference */
			val = PySequence_GetItem(vals, i); /* new reference */
#ifdef I_PY_DEBUG
			printf("recursive call to get value for key:");
			PyObject_Print(key, stdout, Py_PRINT_RAW);
			printf("\n");
#endif

			sv_val = Py2Pl(val);

			if (PyUnicode_Check(key)) {
				PyObject *utf8_string = PyUnicode_AsUTF8String(key);
				key_val = PyString_AsString(utf8_string);
				SV *utf8_key = newSVpv(key_val, PyString_Size(utf8_string));
				SvUTF8_on(utf8_key);

				hv_store_ent(retval, utf8_key, sv_val, 0);
			}
			else {
				if (PyString_Check(key)) {
					key_val = PyString_AsString(key);
				}
				else {
					/* Warning -- encountered a non-string key value while converting a 
					 * Python dictionary into a Perl hash. Perl can only use strings as 
					 * key values. Using Python's string representation of the key as 
					 * Perl's key value.
					 */
					PyObject *s = PyObject_Str(key);
					key_val = PyString_AsString(s);
					Py_DECREF(s);
					if (PL_dowarn)
						warn("Stringifying non-string hash key value: '%s'",
							 key_val);
				}

				if (!key_val) {
					croak("Invalid key on key %i of mapping\n", i);
				}

				hv_store(retval, key_val, strlen(key_val), sv_val, 0);
			}
			SvREFCNT_inc(sv_val);
			Py_DECREF(key);
			Py_DECREF(val);
		}
		Py_DECREF(keys);
		Py_DECREF(vals);
		return newRV_noinc((SV *) retval);
	}

	/* an int */
	else if (PyInt_Check(obj)) {
		SV *sv = newSViv(PyInt_AsLong(obj));
		Printf(("Py2Pl: integer\n"));
		return sv;
	}

	/* a function or method */
	else if (PyFunction_Check(obj)) {
		SV *inst_ptr = newSViv(0);
		SV *inst;
		MAGIC *mg;
		_inline_magic priv;

		inst = newSVrv(inst_ptr, "Inline::Python::Function");

		/* set up magic */
		priv.key = INLINE_MAGIC_KEY;
		sv_magic(inst, inst, '~', (char *) &priv, sizeof(priv));
		mg = mg_find(inst, '~');
		mg->mg_virtual = (MGVTBL *) malloc(sizeof(MGVTBL));
		mg->mg_virtual->svt_free = free_inline_py_obj;

		sv_setiv(inst, (IV) obj);
		/*SvREADONLY_on(inst); *//* to uncomment this means I can't
			re-bless it */
		Py_INCREF(obj);
		Printf(("Py2Pl: Instance. Obj: %p, inst_ptr: %p\n", obj, inst_ptr));

		sv_2mortal(inst_ptr);
		return inst_ptr;
	}

	else if (PyUnicode_Check(obj)) {
		PyObject *string = PyUnicode_AsUTF8String(obj);	/* new reference */
		if (!string) {
			Printf(("Py2Pl: string is NULL!? -> Py_None\n"));
			return &PL_sv_undef;
		}
		char *str = PyString_AsString(string);
		SV *s2 = newSVpv(str, PyString_Size(string));
		SvUTF8_on(s2);
		Printf(("Py2Pl: utf8 string \n"));
		Py_DECREF(string);
		return s2;
	}

	/* a string (or number) */
	else {
		PyObject *string = PyObject_Str(obj);	/* new reference */
		if (!string) {
			Printf(("Py2Pl: string is NULL!? -> Py_None\n"));
			return &PL_sv_undef;
		}
		char *str = PyString_AsString(string);
		SV *s2 = newSVpv(str, PyString_Size(string));
		Printf(("Py2Pl: string / number\n"));
		Py_DECREF(string);
		return s2;
	}
}

/****************************
 * SV* Pl2Py(PyObject *obj) 
 * 
 * Converts arbitrary Perl data structures to Python data structures
 ****************************/
PyObject *Pl2Py(SV * obj) {
	PyObject *o;

	/* an object */
	if (sv_isobject(obj)) {

		/* We know it's a blessed reference:
		 * Now it's time to check whether it's *really* a blessed Perl object,
		 * or whether it's a blessed Python object with '~' magic set.
		 * If '~' magic is set, we 'unwrap' it into its Python object. 
		 * If not, we wrap it up in a PerlObj_object. */

		SV *obj_deref = SvRV(obj);

		/* check for magic! */

		MAGIC *mg = mg_find(obj_deref, '~');
		if (mg && Inline_Magic_Check(mg->mg_ptr)) {
			IV ptr = SvIV(obj_deref);
			if (!ptr) {
				croak
					("Internal error: Pl2Py() caught NULL PyObject* at %s, line %i.\n",
					 __FILE__, __LINE__);
			}
			o = (PyObject *) ptr;
			Py_INCREF(o);
		}
		else {
			HV *stash = SvSTASH(obj_deref);
			char *pkg = HvNAME(stash);
			SV *full_pkg = newSVpvf("main::%s::", pkg);
			PyObject *pkg_py;

			Printf(("A Perl object (%s, refcnt: %i). Wrapping...\n",
					SvPV(full_pkg, PL_na), SvREFCNT(obj)));

			pkg_py = PyString_FromString(SvPV(full_pkg, PL_na));
			o = newPerlObj_object(obj, pkg_py);

			Py_DECREF(pkg_py);
			SvREFCNT_dec(full_pkg);
		}
	}

	/* An integer */
	else if (SvIOK(obj)) {
		Printf(("integer\n"));
		o = PyInt_FromLong((long) SvIV(obj));
	}
	/* A floating-point number */
	else if (SvNOK(obj)) {
		PyObject *tmp = PyString_FromString(SvPV_nolen(obj));
		Printf(("float\n"));
		if (tmp)
			o = PyNumber_Float(tmp);
		else {
			fprintf(stderr, "Internal Error --");
			fprintf(stderr, "your Perl string \"%s\" could not \n",
					SvPV_nolen(obj));
			fprintf(stderr, "be converted to a Python string\n");
			o = PyFloat_FromDouble((double) 0);
		}
		Py_DECREF(tmp);
	}
	/* A string */
	else if (SvPOKp(obj)) {
		STRLEN len;
		char *str = SvPV(obj, len);
		Printf(("string = "));
		Printf(("%s\n", str));
		if (SvUTF8(obj))
			o = PyUnicode_DecodeUTF8(str, len, "replace");
		else
			o = PyString_FromStringAndSize(str, len);
		Printf(("string ok\n"));
	}
	/* An array */
	else if (SvROK(obj) && SvTYPE(SvRV(obj)) == SVt_PVAV) {
		AV *av = (AV *) SvRV(obj);
		int i;
		int len = av_len(av) + 1;
		o = PyList_New(len);

		Printf(("array (%i)\n", len));

		for (i = 0; i < len; i++) {
			SV **tmp = av_fetch(av, i, 0);
			if (tmp) {
				PyObject *tmp_py = Pl2Py(*tmp);
				PyList_SetItem(o, i, tmp_py);
			}
			else {
				Printf(("Got a NULL from av_fetch for element %i. Might be a bug!", i));
				Py_INCREF(Py_None);
				PyList_SetItem(o, i, Py_None);
			}
		}
	}
	/* A hash */
	else if (SvROK(obj) && SvTYPE(SvRV(obj)) == SVt_PVHV) {
		HV *hv = (HV *) SvRV(obj);
		int len = hv_iterinit(hv);
		int i;

		o = PyDict_New();

		Printf(("hash (%i)\n", len));

		for (i = 0; i < len; i++) {
			HE *next = hv_iternext(hv);
			I32 n_a;
			char *key = hv_iterkey(next, &n_a);
			PyObject *val = Pl2Py(hv_iterval(hv, next));
			PyDict_SetItemString(o, key, val);
			Py_DECREF(val);
		}

		Printf(("returning from hash conversion.\n"));

	}
	/* A code ref */
	else if (SvROK(obj) && SvTYPE(SvRV(obj)) == SVt_PVCV) {
		/* wrap this into a PerlSub_object */

		o = (PyObject *) newPerlSub_object(NULL, NULL, obj);
	}

	else {
		Printf(("undef -> None\n"));
		o = Py_None;
		Py_INCREF(Py_None);
	}
	Printf(("returning from Pl2Py\n"));
	return o;
}
