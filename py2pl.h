#ifndef PY2PL_H
#define PY2PL_H

extern SV* Py2Pl (PyObject * const obj);
extern PyObject *Pl2Py(SV * const obj);
extern void croak_python_exception();
extern SV* py_true;
extern SV* py_false;
extern PyObject *PyExc_Perl;

#endif

