#ifndef PY2PL_H
#define PY2PL_H

extern SV* Py2Pl (PyObject * const obj);
extern PyObject *Pl2Py(SV * const obj);
extern void croak_python_exception();
extern SV* py_true;
extern SV* py_false;
extern PyObject *PyExc_Perl;
#if PY_MAJOR_VERSION < 3
#define PY_INSTANCE_CHECK(obj) PyInstance_Check((obj))
#define PY_IS_STRING(obj) (PyString_Check((obj)) || PyUnicode_Check((obj)))
#else
#define PY_INSTANCE_CHECK(obj) 0
#define PY_IS_STRING(obj) (PyBytes_Check((obj)) || PyUnicode_Check((obj)))
#endif
#define PY_IS_OBJECT(obj) \
    (((obj)->ob_type->tp_flags & Py_TPFLAGS_HEAPTYPE) \
        || PY_INSTANCE_CHECK((obj)) \
        || (! is_string && PyMapping_Check((obj)) && ((obj)->ob_type != &PyDict_Type) && \
            ((obj)->ob_type != &PyList_Type) && ((obj)->ob_type != &PyTuple_Type)) )

#endif

