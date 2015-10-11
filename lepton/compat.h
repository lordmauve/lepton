/* Compatibility macros for Python2/3

*/

#ifndef _COMPAT_H_
#define _COMPAT_H_

#if PY_MAJOR_VERSION >= 3
    #define PyString_InternFromString PyUnicode_InternFromString
    #define PyString_FromString PyUnicode_InternFromString
    #define PyString_FromFormat PyUnicode_FromFormat
    #define PyInt_AsLong PyLong_AsLong
    #define PyInt_FromLong PyLong_FromLong
    #define PyString_AS_STRING PyUnicode_AsUTF8
    #define PyString_AsString PyUnicode_AsUTF8

    #define PY_FIND_METHOD(methods, obj, name) \
        PyObject_GenericGetAttr((PyObject *) obj, name)
#else
    #define PY_FIND_METHOD(methods, obj, name) \
        Py_FindMethod(methods, (PyObject *) obj, PyString_AS_STRING(name))
#endif

#if PY_MAJOR_VERSION >= 3
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
          ob = PyModule_Create(&moduledef);
#else
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
  #define MOD_INIT(name) void init##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          ob = Py_InitModule3(name, methods, doc);
#endif

/* Prepare a PyTypeObject */
extern int prepare_type(PyTypeObject *type);

#endif
