#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "Python.h"
#include "py2pl.h"
#include "util.h"

#ifdef EXPOSE_PERL
#include "perlmodule.h"
#endif

SV* py_true;
SV* py_false;

/****************************
 * SV* Py2Pl(PyObject *obj)
 *
 * Converts arbitrary Python data structures to Perl data structures
 * Note on references: does not Py_DECREF(obj).
 *
 * Modifications by Eric Wilhelm 2004-07-11 marked as elw
 *
 ****************************/
SV *Py2Pl(PyObject * const obj) {
    /* elw: see what python says things are */
    int const is_string = PY_IS_STRING(obj);
#ifdef I_PY_DEBUG
    PyObject *this_type = PyObject_Type(obj); /* new reference */
    PyObject *t_string = PyObject_Str(this_type); /* new reference */
#if PY_MAJOR_VERSION >= 3
    PyObject *type_str_bytes = PyUnicode_AsUTF8String(t_string); /* new reference */
    char *type_str = PyBytes_AsString(type_str_bytes);
#else
    char *type_str = PyString_AsString(t_string);
#endif
    Printf(("type is %s\n", type_str));
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
#if PY_MAJOR_VERSION < 3
    Printf(("Class check:    %i\n", PyClass_Check(obj)));
    Printf(("Instance check: %i\n", PyInstance_Check(obj)));
#endif
    Printf(("Dict check:     %i\n", PyDict_Check(obj)));
    Printf(("Mapping check:  %i\n", PyMapping_Check(obj)));
    Printf(("Sequence check: %i\n", PySequence_Check(obj)));
    Printf(("Iter check:     %i\n", PyIter_Check(obj)));
    Printf(("Function check: %i\n", PyFunction_Check(obj)));
    Printf(("Module check:   %i\n", PyModule_Check(obj)));
    Printf(("Method check:   %i\n", PyMethod_Check(obj)));
#if PY_MAJOR_VERSION < 3
    if ((obj->ob_type->tp_flags & Py_TPFLAGS_HEAPTYPE))
        printf("heaptype true\n");
    if ((obj->ob_type->tp_flags & Py_TPFLAGS_HAVE_CLASS))
        printf("has class\n");
#else
    Py_DECREF(type_str_bytes);
#endif
    Py_DECREF(t_string);
    Py_DECREF(this_type);
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
        SV * ref = ((PerlSub_object *) obj)->ref;
        if (! ref) { /* probably an inherited method */
            if (! ((PerlSub_object *) obj)->obj)
                croak("Error: could not find a code reference or object method for PerlSub");
            SV * const sub_obj = (SV*)SvRV(((PerlSub_object *) obj)->obj);
            HV * const pkg = SvSTASH(sub_obj);
#if PY_MAJOR_VERSION >= 3
            char * const sub = PyBytes_AsString(((PerlSub_object *) obj)->sub);
#else
            PyObject *obj_sub_str = PyObject_Str(((PerlSub_object *) obj)->sub); /* new ref. */
            char * const sub = PyString_AsString(obj_sub_str);
#endif
            GV * const gv = Perl_gv_fetchmethod_autoload(aTHX_ pkg, sub, TRUE);
            if (gv && isGV(gv)) {
                ref = (SV *)GvCV(gv);
            }
#if PY_MAJOR_VERSION < 3
            Py_DECREF(obj_sub_str);
#endif
        }
        return newRV_inc((SV *) ref);
    }

    else
#endif

    /* wrap an instance of a Python class */
    /* elw: here we need to make these look like instances: */
    if (PY_IS_OBJECT(obj)) {
        /* This is a Python class instance -- bless it into an
         * Inline::Python::Object. If we're being called from an
         * Inline::Python class, it will be re-blessed into whatever
         * class that is.
         */
        SV * const inst_ptr = newSViv(0);
        SV * const inst = newSVrv(inst_ptr, "Inline::Python::Object");;
        _inline_magic priv;

        /* set up magic */
        priv.key = INLINE_MAGIC_KEY;
        sv_magic(inst, inst, PERL_MAGIC_ext, (char *) &priv, sizeof(priv));
        MAGIC * const mg = mg_find(inst, PERL_MAGIC_ext);
        mg->mg_virtual = &inline_mg_vtbl;

        sv_setiv(inst, (IV) obj);
        /*SvREADONLY_on(inst); */ /* to uncomment this means I can't
            re-bless it */
        Py_INCREF(obj);
        Printf(("Py2Pl: Instance. Obj: %p, inst_ptr: %p\n", obj, inst_ptr));

        sv_2mortal(inst_ptr);
        return inst_ptr;
    }

    /* a tuple or a list */
    else if (PySequence_Check(obj) && !is_string) {
        AV * const retval = newAV();
        int i;
        int const sz = PySequence_Length(obj);

        Printf(("sequence (%i)\n", sz));

        for (i = 0; i < sz; i++) {
            PyObject * const tmp = PySequence_GetItem(obj, i);    /* new reference */
            SV * const next = Py2Pl(tmp);
            av_push(retval, next);
            if (sv_isobject(next)) // needed because objects get mortalized in Py2Pl
                SvREFCNT_inc(next);
            Py_DECREF(tmp);
        }

        if (PyTuple_Check(obj)) {
            _inline_magic priv;
            priv.key = TUPLE_MAGIC_KEY;

            sv_magic((SV * const)retval, (SV * const)NULL, PERL_MAGIC_ext, (char *) &priv, sizeof(priv));
        }

        return newRV_noinc((SV *) retval);
    }

    /* a real plain dictionary  */
    else if (obj->ob_type == &PyDict_Type) {
        HV * const retval = newHV();
        int i;
        int const sz = PyMapping_Length(obj);
        PyObject * const keys = PyMapping_Keys(obj);   /* new reference */
        PyObject * const vals = PyMapping_Values(obj); /* new reference */

        Printf(("Py2Pl: dict/map\n"));
        Printf(("mapping (%i)\n", sz));

        for (i = 0; i < sz; i++) {
            PyObject * const key = PySequence_GetItem(keys, i), /* new reference */
                                 * const val = PySequence_GetItem(vals, i); /* new reference */
            SV       * const sv_val = Py2Pl(val);
            char     *       key_val;

            if (PyUnicode_Check(key)) {
                PyObject * const utf8_string = PyUnicode_AsUTF8String(key); /* new reference */
#if PY_MAJOR_VERSION >= 3
                key_val = PyBytes_AsString(utf8_string);
                SV * const utf8_key = newSVpv(key_val, PyBytes_Size(utf8_string));
#else
                key_val = PyString_AsString(utf8_string);
                SV * const utf8_key = newSVpv(key_val, PyString_Size(utf8_string));
#endif
                SvUTF8_on(utf8_key);

                hv_store_ent(retval, utf8_key, sv_val, 0);

                sv_2mortal(utf8_key);
                Py_DECREF(utf8_string);
            }
            else {
                PyObject * s = NULL;
#if PY_MAJOR_VERSION >= 3
                PyObject * s_bytes = NULL;
                if (PyBytes_Check(key)) {
                    key_val = PyBytes_AsString(key);
#else
                if (PyString_Check(key)) {
                    key_val = PyString_AsString(key);
#endif
                }
                else {
                    /* Warning -- encountered a non-string key value while converting a
                     * Python dictionary into a Perl hash. Perl can only use strings as
                     * key values. Using Python's string representation of the key as
                     * Perl's key value.
                     */
                    s = PyObject_Str(key); /* new reference */
#if PY_MAJOR_VERSION >= 3
                    s_bytes = PyUnicode_AsUTF8String(s); /* new reference */
                    key_val = PyBytes_AsString(s_bytes);
#else
                    key_val = PyString_AsString(s);
#endif
                    if (PL_dowarn)
                        warn("Stringifying non-string hash key value: '%s'",
                             key_val);
                }

                if (!key_val) {
                    croak("Invalid key on key %i of mapping\n", i);
                }

                hv_store(retval, key_val, strlen(key_val), sv_val, 0);
#if PY_MAJOR_VERSION >= 3
                Py_XDECREF(s_bytes);
#endif
                Py_XDECREF(s);
            }
            if (sv_isobject(sv_val)) // needed because objects get mortalized in Py2Pl
                SvREFCNT_inc(sv_val);
            Py_DECREF(key);
            Py_DECREF(val);
        }
        Py_DECREF(keys);
        Py_DECREF(vals);
        return newRV_noinc((SV *) retval);
    }

    /* a boolean */
    else if (PyBool_Check(obj)) {
        Printf(("Py2Pl: boolean\n"));
        if (obj == Py_True)
            return py_true;
        if (obj == Py_False)
            return py_false;

        croak(
            "Internal error: Pl2Py() caught a bool that is not True or False!? at %s, line %i.\n",
            __FILE__,
            __LINE__
        );
    }

    /* an int */
    else if (PyInt_Check(obj)) {
        SV * const sv = newSViv(PyInt_AsLong(obj));
        Printf(("Py2Pl: integer\n"));
        return sv;
    }

    /* a float */
    else if (PyFloat_Check(obj)) {
        SV * const sv = newSVnv(PyFloat_AsDouble(obj));
        Printf(("Py2Pl: float\n"));
        return sv;
    }

    /* a function or method */
    else if (PyFunction_Check(obj) || PyMethod_Check(obj)) {
        SV * const inst_ptr = newSViv(0);
        SV * const inst = newSVrv(inst_ptr, "Inline::Python::Function");
        _inline_magic priv;

        /* set up magic */
        priv.key = INLINE_MAGIC_KEY;
        sv_magic(inst, inst, '~', (char *) &priv, sizeof(priv));
        MAGIC * const mg = mg_find(inst, '~');
        mg->mg_virtual = &inline_mg_vtbl;

        sv_setiv(inst, (IV) obj);
        /*SvREADONLY_on(inst); */ /* to uncomment this means I can't
            re-bless it */
        Py_INCREF(obj);
        Printf(("Py2Pl: Instance. Obj: %p, inst_ptr: %p\n", obj, inst_ptr));

        sv_2mortal(inst_ptr);
        return inst_ptr;
    }

#if PY_MAJOR_VERSION >= 3
    /* In P3, I stringify Bytes explicitly to avoid "b'fdsfd'" as Perl string.
     * "PyObject_Str" is fine for string object. */
    else if (PyBytes_Check(obj)) {
        char * const str = PyBytes_AsString(obj);
        SV * const s2 = newSVpv(str, PyBytes_Size(obj));
        Printf(("Py2Pl: bytes \n"));
        return s2;
    }
#endif
    else if (PyUnicode_Check(obj)) {
        PyObject * const string = PyUnicode_AsUTF8String(obj);    /* new reference */
        if (!string) {
            Printf(("Py2Pl: string is NULL!? -> Py_None\n"));
            return &PL_sv_undef;
        }
#if PY_MAJOR_VERSION >= 3
        char * const str = PyBytes_AsString(string);
        SV * const s2 = newSVpv(str, PyBytes_Size(string));
#else
        char * const str = PyString_AsString(string);
        SV * const s2 = newSVpv(str, PyString_Size(string));
#endif
        SvUTF8_on(s2);
        Printf(("Py2Pl: utf8 string \n"));
        Py_DECREF(string);
        return s2;
    }

    /* P2: a string (or number). P3: number*/
    else {
        PyObject * const string = PyObject_Str(obj);    /* new reference */
        if (!string) {
            Printf(("Py2Pl: string is NULL!? -> Py_None\n"));
            return &PL_sv_undef;
        }
#if PY_MAJOR_VERSION >= 3
        PyObject * const string_as_bytes = PyUnicode_AsUTF8String(string);    /* new reference */
        char * const str = PyBytes_AsString(string_as_bytes);
        SV * const s2 = newSVpv(str, PyBytes_Size(string_as_bytes));
        Py_DECREF(string_as_bytes);
#else
        char * const str = PyString_AsString(string);
        SV * const s2 = newSVpv(str, PyString_Size(string));
#endif
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
PyObject *Pl2Py(SV * const obj) {
    PyObject *o;

    /* an object */
    if (sv_isobject(obj)) {
        /* We know it's a blessed reference: */

        SV * const obj_deref = SvRV(obj);

        /* First check if it's one of the Inline::Python::Boolean values */

        if (obj == py_true || obj_deref == SvRV(py_true))
            Py_RETURN_TRUE;
        if (obj == py_false || obj_deref == SvRV(py_false))
            Py_RETURN_FALSE;

        /*
         * Now it's time to check whether it's *really* a blessed Perl object,
         * or whether it's a blessed Python object with '~' magic set.
         * If '~' magic is set, we 'unwrap' it into its Python object.
         * If not, we wrap it up in a PerlObj_object. */

        /* check for magic! */
        MAGIC * const mg = mg_find(obj_deref, '~');

        if (mg && Inline_Magic_Check(mg->mg_ptr)) {
            IV const ptr = SvIV(obj_deref);
            if (!ptr) {
                croak
                    ("Internal error: Pl2Py() caught NULL PyObject* at %s, line %i.\n",
                     __FILE__, __LINE__);
            }
            o = (PyObject *) ptr;
            Py_INCREF(o);
        }
        else {
            HV * const stash = SvSTASH(obj_deref);
            char * const pkg = HvNAME(stash);
            SV * const full_pkg = newSVpvf("main::%s::", pkg);

            Printf(("A Perl object (%s, refcnt: %i). Wrapping...\n",
                    SvPV(full_pkg, PL_na), SvREFCNT(obj)));

#if PY_MAJOR_VERSION >= 3
            PyObject * const pkg_py = PyBytes_FromString(SvPV(full_pkg, PL_na));
#else
            PyObject * const pkg_py = PyString_FromString(SvPV(full_pkg, PL_na));
#endif
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
        o = PyFloat_FromDouble(SvNV(obj));
    }
    /* A string */
    else if (SvPOKp(obj)) {
        STRLEN len;
        char * const str = SvPV(obj, len);
        Printf(("string = "));
        Printf(("%s\n", str));
#if PY_MAJOR_VERSION >= 3
        if (SvUTF8(obj) || is_ascii_string((U8*) str, len))
            o = PyUnicode_DecodeUTF8(str, len, "replace");
        else
            o = PyBytes_FromStringAndSize(str, len);
#else
        if (SvUTF8(obj))
            o = PyUnicode_DecodeUTF8(str, len, "replace");
        else
            o = PyString_FromStringAndSize(str, len);
#endif
        Printf(("string ok\n"));
    }
    /* An array */
    else if (SvROK(obj) && SvTYPE(SvRV(obj)) == SVt_PVAV) {
        AV * const av = (AV *) SvRV(obj);
        int const len = av_len(av) + 1;
        int i;

        if (py_is_tuple(obj)) {
            o = PyTuple_New(len);

            Printf(("tuple (%i)\n", len));

            for (i = 0; i < len; i++) {
                SV ** const tmp = av_fetch(av, i, 0);
                if (tmp) {
                    PyObject * const tmp_py = Pl2Py(*tmp);
                    PyTuple_SetItem(o, i, tmp_py);
                }
                else {
                    Printf(("Got a NULL from av_fetch for element %i. Might be a bug!", i));
                    Py_INCREF(Py_None);
                    PyTuple_SetItem(o, i, Py_None);
                }
            }
        }
        else {
            o = PyList_New(len);

            Printf(("array (%i)\n", len));

            for (i = 0; i < len; i++) {
                SV ** const tmp = av_fetch(av, i, 0);
                if (tmp) {
                    PyObject * const tmp_py = Pl2Py(*tmp);
                    PyList_SetItem(o, i, tmp_py);
                }
                else {
                    Printf(("Got a NULL from av_fetch for element %i. Might be a bug!", i));
                    Py_INCREF(Py_None);
                    PyList_SetItem(o, i, Py_None);
                }
            }
        }
    }
    /* A hash */
    else if (SvROK(obj) && SvTYPE(SvRV(obj)) == SVt_PVHV) {
        HV * const hv = (HV *) SvRV(obj);
        int const len = hv_iterinit(hv);
        int i;

        o = PyDict_New();

        Printf(("hash (%i)\n", len));

        for (i = 0; i < len; i++) {
            HE * const next = hv_iternext(hv);
            SV * const key = hv_iterkeysv(next);
            if (!key)
                croak("Hash entry without key!?");
            STRLEN len;
            char * const key_str = SvPV(key, len);
            PyObject *py_key;
#if PY_MAJOR_VERSION >= 3
            if (SvUTF8(key) || is_ascii_string((U8*) key_str, len))
                py_key = PyUnicode_DecodeUTF8(key_str, len, "replace");
            else
                py_key = PyBytes_FromStringAndSize(key_str, len);
#else
            if (SvUTF8(key))
                py_key = PyUnicode_DecodeUTF8(key_str, len, "replace");
            else
                py_key = PyString_FromStringAndSize(key_str, len);
#endif
            PyObject * const val = Pl2Py(hv_iterval(hv, next));
            PyDict_SetItem(o, py_key, val);
            Py_DECREF(py_key);
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

void
croak_python_exception() {
    PyObject *ex_type, *ex_value, *ex_traceback;
    if (PyErr_ExceptionMatches(PyExc_Perl)) {
        PyErr_Fetch(&ex_type, &ex_value, &ex_traceback);
        PyErr_NormalizeException(&ex_type, &ex_value, &ex_traceback);
        PyObject *perl_exception_args = PyObject_GetAttrString(ex_value, "args");
        PyObject *perl_exception = PySequence_GetItem(perl_exception_args, 0);
        SV *perl_exception_object = Py2Pl(perl_exception);
        sv_2mortal(perl_exception_object);
        SV *errsv = get_sv("@", GV_ADD);
        sv_setsv(errsv, perl_exception_object);
        croak(NULL);
        Py_DECREF(perl_exception);
        Py_DECREF(perl_exception_args);
    }
    else {
        PyErr_Fetch(&ex_type, &ex_value, &ex_traceback);
        PyErr_NormalizeException(&ex_type, &ex_value, &ex_traceback);

        PyObject * const ex_message = PyObject_Str(ex_value);    /* new reference */

#if PY_MAJOR_VERSION >= 3
        PyObject * const ex_message_bytes = PyUnicode_AsUTF8String(ex_message);    /* new reference */
        char * const c_ex_message = PyBytes_AsString(ex_message_bytes);
#else
        char * const c_ex_message = PyString_AsString(ex_message);
#endif

        if (ex_traceback) {
            PyObject * const tb_lineno = PyObject_GetAttrString(ex_traceback, "tb_lineno");

            croak("%s: %s at line %li\n", ((PyTypeObject *)ex_type)->tp_name, c_ex_message, PyInt_AsLong(tb_lineno));

            Py_DECREF(tb_lineno);
        }
        else {
            croak("%s: %s", ((PyTypeObject *)ex_type)->tp_name, c_ex_message);
        }

#if PY_MAJOR_VERSION >= 3
        Py_DECREF(ex_message_bytes);
#endif
        Py_DECREF(ex_message);
    }
    Py_DECREF(ex_type);
    Py_DECREF(ex_value);
    Py_XDECREF(ex_traceback);
}

/*
 * vim: expandtab shiftwidth=4 softtabstop=4 cinoptions='\:2=2' :
 */
