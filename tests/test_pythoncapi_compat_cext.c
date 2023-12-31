// Always enable assertions
#undef NDEBUG

#include "pythoncapi_compat.h"

#ifdef NDEBUG
#  error "assertions must be enabled"
#endif

#ifdef Py_LIMITED_API
#  error "Py_LIMITED_API is not supported"
#endif

#if PY_VERSION_HEX >= 0x03000000
#  define PYTHON3 1
#endif

#if defined(_MSC_VER) && defined(__cplusplus)
#  define MODULE_NAME test_pythoncapi_compat_cppext
#elif defined(__cplusplus) && __cplusplus >= 201103
#  define MODULE_NAME test_pythoncapi_compat_cpp11ext
#elif defined(__cplusplus)
#  define MODULE_NAME test_pythoncapi_compat_cpp03ext
#else
#  define MODULE_NAME test_pythoncapi_compat_cext
#endif

#define _STR(NAME) #NAME
#define STR(NAME) _STR(NAME)
#define _CONCAT(a, b) a ## b
#define CONCAT(a, b) _CONCAT(a, b)

#define MODULE_NAME_STR STR(MODULE_NAME)

// Ignore reference count checks on PyPy
#ifndef PYPY_VERSION
#  define CHECK_REFCNT
#endif

// CPython 3.12 beta 1 implements immortal objects (PEP 683)
#if 0x030C00B1 <= PY_VERSION_HEX && !defined(PYPY_VERSION)
   // Don't check reference count of Python 3.12 immortal objects (ex: bool
   // and str types)
#  define IMMORTAL_OBJS
#endif

#ifdef CHECK_REFCNT
#  define ASSERT_REFCNT(expr) assert(expr)
#else
#  define ASSERT_REFCNT(expr)
#endif


static PyObject*
create_string(const char *str)
{
    PyObject *obj;
#ifdef PYTHON3
    obj = PyUnicode_FromString(str);
#else
    obj = PyString_FromString(str);
#endif
    assert(obj != _Py_NULL);
    return obj;
}


static PyObject *
test_object(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyObject *obj = PyList_New(0);
    if (obj == _Py_NULL) {
        return _Py_NULL;
    }
    Py_ssize_t refcnt = Py_REFCNT(obj);

    // Py_NewRef()
    PyObject *ref = Py_NewRef(obj);
    assert(ref == obj);
    assert(Py_REFCNT(obj) == (refcnt + 1));
    Py_DECREF(ref);

    // Py_XNewRef()
    PyObject *xref = Py_XNewRef(obj);
    assert(xref == obj);
    assert(Py_REFCNT(obj) == (refcnt + 1));
    Py_DECREF(xref);

    assert(Py_XNewRef(_Py_NULL) == _Py_NULL);

    // Py_SETREF()
    PyObject *setref = Py_NewRef(obj);
    PyObject *none = Py_None;
    assert(Py_REFCNT(obj) == (refcnt + 1));

    Py_SETREF(setref, none);
    assert(setref == none);
    assert(Py_REFCNT(obj) == refcnt);
    Py_INCREF(setref);

    Py_SETREF(setref, _Py_NULL);
    assert(setref == _Py_NULL);

    // Py_XSETREF()
    PyObject *xsetref = _Py_NULL;

    Py_INCREF(obj);
    assert(Py_REFCNT(obj) == (refcnt + 1));
    Py_XSETREF(xsetref, obj);
    assert(xsetref == obj);

    Py_XSETREF(xsetref, _Py_NULL);
    assert(Py_REFCNT(obj) == refcnt);
    assert(xsetref == _Py_NULL);

    // Py_SET_REFCNT
    Py_SET_REFCNT(obj, Py_REFCNT(obj));
    // Py_SET_TYPE
    Py_SET_TYPE(obj, Py_TYPE(obj));
    // Py_SET_SIZE
    Py_SET_SIZE(obj, Py_SIZE(obj));
    // Py_IS_TYPE()
    int is_type = Py_IS_TYPE(obj, Py_TYPE(obj));
    assert(is_type);

    assert(Py_REFCNT(obj) == refcnt);
    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyObject *
test_py_is(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyObject *o_none = Py_None;
    PyObject *o_true = Py_True;
    PyObject *o_false = Py_False;
    PyObject *obj = PyList_New(0);
    if (obj == _Py_NULL) {
        return _Py_NULL;
    }

    /* test Py_Is() */
    assert(Py_Is(obj, obj));
    assert(!Py_Is(obj, o_none));

    /* test Py_IsNone() */
    assert(Py_IsNone(o_none));
    assert(!Py_IsNone(obj));

    /* test Py_IsTrue() */
    assert(Py_IsTrue(o_true));
    assert(!Py_IsTrue(o_false));
    assert(!Py_IsTrue(obj));

    /* testPy_IsFalse() */
    assert(Py_IsFalse(o_false));
    assert(!Py_IsFalse(o_true));
    assert(!Py_IsFalse(obj));

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


#ifndef PYPY_VERSION
static void
test_frame_getvar(PyFrameObject *frame)
{
    // Make the assumption that test_frame_getvar() is only called by
    // _run_tests() of test_pythoncapi_compat.py and so that the "name"
    // variable exists.

    // test PyFrame_GetVar() and PyFrame_GetVarString()
    PyObject *attr = PyUnicode_FromString("name");
    assert(attr != _Py_NULL);
    PyObject *name1 = PyFrame_GetVar(frame, attr);
    Py_DECREF(attr);
    assert(name1 != _Py_NULL);
    Py_DECREF(name1);

    PyObject *name2 = PyFrame_GetVarString(frame, "name");
    assert(name2 != _Py_NULL);
    Py_DECREF(name2);

    // test PyFrame_GetVar() and PyFrame_GetVarString() NameError
    PyObject *attr3 = PyUnicode_FromString("dontexist");
    assert(attr3 != _Py_NULL);
    PyObject *name3 = PyFrame_GetVar(frame, attr3);
    Py_DECREF(attr3);
    assert(name3 == _Py_NULL);
    assert(PyErr_ExceptionMatches(PyExc_NameError));
    PyErr_Clear();

    PyObject *name4 = PyFrame_GetVarString(frame, "dontexist");
    assert(name4 == _Py_NULL);
    assert(PyErr_ExceptionMatches(PyExc_NameError));
    PyErr_Clear();
}


static PyObject *
test_frame(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyThreadState *tstate = PyThreadState_Get();

    // test PyThreadState_GetFrame()
    PyFrameObject *frame = PyThreadState_GetFrame(tstate);
    if (frame == _Py_NULL) {
        PyErr_SetString(PyExc_AssertionError, "PyThreadState_GetFrame failed");
        return _Py_NULL;
    }

    // test _PyThreadState_GetFrameBorrow()
    Py_ssize_t frame_refcnt = Py_REFCNT(frame);
    PyFrameObject *frame2 = _PyThreadState_GetFrameBorrow(tstate);
    assert(frame2 == frame);
    assert(Py_REFCNT(frame) == frame_refcnt);

    // test PyFrame_GetCode()
    PyCodeObject *code = PyFrame_GetCode(frame);
    assert(code != _Py_NULL);
    assert(PyCode_Check(code));

    // test _PyFrame_GetCodeBorrow()
    Py_ssize_t code_refcnt = Py_REFCNT(code);
    PyCodeObject *code2 = _PyFrame_GetCodeBorrow(frame);
    assert(code2 == code);
    assert(Py_REFCNT(code) == code_refcnt);
    Py_DECREF(code);

    // PyFrame_GetBack()
    PyFrameObject* back = PyFrame_GetBack(frame);
    if (back != _Py_NULL) {
        assert(PyFrame_Check(back));
    }

    // test _PyFrame_GetBackBorrow()
    if (back != _Py_NULL) {
        Py_ssize_t back_refcnt = Py_REFCNT(back);
        PyFrameObject *back2 = _PyFrame_GetBackBorrow(frame);
        assert(back2 == back);
        assert(Py_REFCNT(back) == back_refcnt);
    }
    else {
        PyFrameObject *back2 = _PyFrame_GetBackBorrow(frame);
        assert(back2 == back);
    }
    Py_XDECREF(back);

    // test PyFrame_GetLocals()
    PyObject *locals = PyFrame_GetLocals(frame);
    assert(locals != _Py_NULL);
    assert(PyDict_Check(locals));

    // test PyFrame_GetGlobals()
    PyObject *globals = PyFrame_GetGlobals(frame);
    assert(globals != _Py_NULL);
    assert(PyDict_Check(globals));

    // test PyFrame_GetBuiltins()
    PyObject *builtins = PyFrame_GetBuiltins(frame);
    assert(builtins != _Py_NULL);
    assert(PyDict_Check(builtins));

    assert(locals != globals);
    assert(globals != builtins);
    assert(builtins != locals);

    Py_DECREF(locals);
    Py_DECREF(globals);
    Py_DECREF(builtins);

    // test PyFrame_GetLasti()
    int lasti = PyFrame_GetLasti(frame);
    assert(lasti >= 0);

    // test PyFrame_GetVar() and PyFrame_GetVarString()
    test_frame_getvar(frame);

    // done
    Py_DECREF(frame);
    Py_RETURN_NONE;
}
#endif


static PyObject *
test_thread_state(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyThreadState *tstate = PyThreadState_Get();

    // test PyThreadState_GetInterpreter()
    PyInterpreterState *interp = PyThreadState_GetInterpreter(tstate);
    assert(interp != _Py_NULL);

#ifndef PYPY_VERSION
    // test PyThreadState_GetFrame()
    PyFrameObject *frame = PyThreadState_GetFrame(tstate);
    if (frame != _Py_NULL) {
        assert(PyFrame_Check(frame));
    }
    Py_XDECREF(frame);
#endif

#if 0x030700A1 <= PY_VERSION_HEX && !defined(PYPY_VERSION)
    uint64_t id = PyThreadState_GetID(tstate);
    assert(id > 0);
#endif

#ifndef PYPY_VERSION
    // PyThreadState_EnterTracing(), PyThreadState_LeaveTracing()
    PyThreadState_EnterTracing(tstate);
    PyThreadState_LeaveTracing(tstate);
#endif

    Py_RETURN_NONE;
}


static PyObject *
test_interpreter(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    // test PyInterpreterState_Get()
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != _Py_NULL);
    PyThreadState *tstate = PyThreadState_Get();
    PyInterpreterState *interp2 = PyThreadState_GetInterpreter(tstate);
    assert(interp == interp2);

    Py_RETURN_NONE;
}


static PyObject *
test_calls(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyObject *func = _Py_CAST(PyObject*, &PyUnicode_Type);

    // test PyObject_CallNoArgs(): str() returns ''
    PyObject *res = PyObject_CallNoArgs(func);
    if (res == _Py_NULL) {
        return _Py_NULL;
    }
    assert(PyUnicode_Check(res));
    Py_DECREF(res);

    // test PyObject_CallOneArg(): str(1) returns '1'
    PyObject *arg = PyLong_FromLong(1);
    if (arg == _Py_NULL) {
        return _Py_NULL;
    }
    res = PyObject_CallOneArg(func, arg);
    Py_DECREF(arg);
    if (res == _Py_NULL) {
        return _Py_NULL;
    }
    assert(PyUnicode_Check(res));
    Py_DECREF(res);

    Py_RETURN_NONE;
}


static PyObject *
test_gc(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyObject *tuple = PyTuple_New(1);
    Py_INCREF(Py_None);
    PyTuple_SET_ITEM(tuple, 0, Py_None);

#if !defined(PYPY_VERSION)
    // test PyObject_GC_IsTracked()
    int tracked = PyObject_GC_IsTracked(tuple);
    assert(tracked);
#endif

#if PY_VERSION_HEX >= 0x030400F0 && !defined(PYPY_VERSION)
    // test PyObject_GC_IsFinalized()
    int finalized = PyObject_GC_IsFinalized(tuple);
    assert(!finalized);
#endif

    Py_DECREF(tuple);
    Py_RETURN_NONE;
}


static int
check_module_attr(PyObject *module, const char *name, PyObject *expected)
{
    PyObject *attr = PyObject_GetAttrString(module, name);
    if (attr == _Py_NULL) {
        return -1;
    }
    assert(attr == expected);
    Py_DECREF(attr);

    if (PyObject_DelAttrString(module, name) < 0) {
        return -1;
    }
    return 0;
}


// test PyModule_AddType()
static int
test_module_add_type(PyObject *module)
{
    PyTypeObject *type = &PyUnicode_Type;
#ifdef PYTHON3
    const char *type_name = "str";
#else
    const char *type_name = "unicode";
#endif
#ifdef CHECK_REFCNT
    Py_ssize_t refcnt = Py_REFCNT(type);
#endif

    if (PyModule_AddType(module, type) < 0) {
        return -1;
    }
#ifndef IMMORTAL_OBJS
    ASSERT_REFCNT(Py_REFCNT(type) == refcnt + 1);
#endif

    if (check_module_attr(module, type_name, _Py_CAST(PyObject*, type)) < 0) {
        return -1;
    }
    ASSERT_REFCNT(Py_REFCNT(type) == refcnt);
    return 0;
}


// test PyModule_AddObjectRef()
static int
test_module_addobjectref(PyObject *module)
{
    const char *name = "test_module_addobjectref";
    PyObject *obj = PyUnicode_FromString(name);
    assert(obj != _Py_NULL);
#ifdef CHECK_REFCNT
    Py_ssize_t refcnt = Py_REFCNT(obj);
#endif

    if (PyModule_AddObjectRef(module, name, obj) < 0) {
        ASSERT_REFCNT(Py_REFCNT(obj) == refcnt);
        Py_DECREF(obj);
        return -1;
    }
    ASSERT_REFCNT(Py_REFCNT(obj) == refcnt + 1);

    if (check_module_attr(module, name, obj) < 0) {
        Py_DECREF(obj);
        return -1;
    }
    ASSERT_REFCNT(Py_REFCNT(obj) == refcnt);

    // PyModule_AddObjectRef() with value=NULL must not crash
    assert(!PyErr_Occurred());
    int res = PyModule_AddObjectRef(module, name, _Py_NULL);
    assert(res < 0);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    PyErr_Clear();

    Py_DECREF(obj);
    return 0;
}


// test PyModule_Add()
static int
test_module_add(PyObject *module)
{
    const char *name = "test_module_add";
    PyObject *obj = PyUnicode_FromString(name);
    assert(obj != _Py_NULL);
#ifdef CHECK_REFCNT
    Py_ssize_t refcnt = Py_REFCNT(obj);
#endif

    if (PyModule_Add(module, name, Py_NewRef(obj)) < 0) {
        ASSERT_REFCNT(Py_REFCNT(obj) == refcnt);
        Py_DECREF(obj);
        return -1;
    }
    ASSERT_REFCNT(Py_REFCNT(obj) == refcnt + 1);

    if (check_module_attr(module, name, obj) < 0) {
        Py_DECREF(obj);
        return -1;
    }
    ASSERT_REFCNT(Py_REFCNT(obj) == refcnt);

    // PyModule_Add() with value=NULL must not crash
    assert(!PyErr_Occurred());
    int res = PyModule_Add(module, name, _Py_NULL);
    assert(res < 0);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    PyErr_Clear();

    Py_DECREF(obj);
    return 0;
}


static PyObject *
test_module(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyObject *module = PyImport_ImportModule("sys");
    if (module == _Py_NULL) {
        return _Py_NULL;
    }
    assert(PyModule_Check(module));

    if (test_module_add_type(module) < 0) {
        goto error;
    }
    if (test_module_addobjectref(module) < 0) {
        goto error;
    }
    if (test_module_add(module) < 0) {
        goto error;
    }

    Py_DECREF(module);
    Py_RETURN_NONE;

error:
    Py_DECREF(module);
    return _Py_NULL;
}


#if (PY_VERSION_HEX <= 0x030B00A1 || 0x030B00A7 <= PY_VERSION_HEX) && !defined(PYPY_VERSION)
static PyObject *
test_float_pack(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    const int big_endian = 0;
    const int little_endian = 1;
    char data[8];
    const double d = 1.5;

#if PY_VERSION_HEX >= 0x030600B1
#  define HAVE_FLOAT_PACK2
#endif

    // Test Pack (big endian)
#ifdef HAVE_FLOAT_PACK2
    assert(PyFloat_Pack2(d, data, big_endian) == 0);
    assert(memcmp(data, ">\x00", 2) == 0);
#endif

    assert(PyFloat_Pack4(d, data, big_endian) == 0);
    assert(memcmp(data, "?\xc0\x00\x00", 2) == 0);

    assert(PyFloat_Pack8(d, data, big_endian) == 0);
    assert(memcmp(data, "?\xf8\x00\x00\x00\x00\x00\x00", 2) == 0);

    // Test Pack (little endian)
#ifdef HAVE_FLOAT_PACK2
    assert(PyFloat_Pack2(d, data, little_endian) == 0);
    assert(memcmp(data, "\x00>", 2) == 0);
#endif

    assert(PyFloat_Pack4(d, data, little_endian) == 0);
    assert(memcmp(data, "\x00\x00\xc0?", 2) == 0);

    assert(PyFloat_Pack8(d, data, little_endian) == 0);
    assert(memcmp(data, "\x00\x00\x00\x00\x00\x00\xf8?", 2) == 0);

    // Test Unpack (big endian)
#ifdef HAVE_FLOAT_PACK2
    assert(PyFloat_Unpack2(">\x00", big_endian) == d);
#endif
    assert(PyFloat_Unpack4("?\xc0\x00\x00", big_endian) == d);
    assert(PyFloat_Unpack8("?\xf8\x00\x00\x00\x00\x00\x00", big_endian) == d);

    // Test Unpack (little endian)
#ifdef HAVE_FLOAT_PACK2
    assert(PyFloat_Unpack2("\x00>", little_endian) == d);
#endif
    assert(PyFloat_Unpack4("\x00\x00\xc0?", little_endian) == d);
    assert(PyFloat_Unpack8("\x00\x00\x00\x00\x00\x00\xf8?", little_endian) == d);

    Py_RETURN_NONE;
}
#endif


#if !defined(PYPY_VERSION)
static PyObject *
test_code(PyObject *Py_UNUSED(module), PyObject* Py_UNUSED(ignored))
{
    PyThreadState *tstate = PyThreadState_Get();
    PyFrameObject *frame = PyThreadState_GetFrame(tstate);
    if (frame == _Py_NULL) {
        PyErr_SetString(PyExc_AssertionError, "PyThreadState_GetFrame failed");
        return _Py_NULL;
    }
    PyCodeObject *code = PyFrame_GetCode(frame);

    // PyCode_GetCode()
    {
        PyObject *co_code = PyCode_GetCode(code);
        assert(co_code != _Py_NULL);
        assert(PyBytes_Check(co_code));
        Py_DECREF(co_code);
    }

    // PyCode_GetVarnames
    {
        PyObject *co_varnames = PyCode_GetVarnames(code);
        assert(co_varnames != _Py_NULL);
        assert(PyTuple_CheckExact(co_varnames));
        assert(PyTuple_GET_SIZE(co_varnames) != 0);
        Py_DECREF(co_varnames);
    }

    // PyCode_GetCellvars
    {
        PyObject *co_cellvars = PyCode_GetCellvars(code);
        assert(co_cellvars != _Py_NULL);
        assert(PyTuple_CheckExact(co_cellvars));
        assert(PyTuple_GET_SIZE(co_cellvars) == 0);
        Py_DECREF(co_cellvars);
    }

    // PyCode_GetFreevars
    {
        PyObject *co_freevars = PyCode_GetFreevars(code);
        assert(co_freevars != _Py_NULL);
        assert(PyTuple_CheckExact(co_freevars));
        assert(PyTuple_GET_SIZE(co_freevars) == 0);
        Py_DECREF(co_freevars);
    }

    Py_DECREF(code);
    Py_DECREF(frame);
    Py_RETURN_NONE;
}
#endif


#ifdef __cplusplus
// Class to test operator casting an object to PyObject*
class StrongRef
{
public:
    StrongRef(PyObject *obj) : m_obj(obj) {
        Py_INCREF(this->m_obj);
    }

    ~StrongRef() {
        Py_DECREF(this->m_obj);
    }

    // Cast to PyObject*: get a borrowed reference
    inline operator PyObject*() const { return this->m_obj; }

private:
    PyObject *m_obj;  // Strong reference
};
#endif


static PyObject *
test_api_casts(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *obj = Py_BuildValue("(ii)", 1, 2);
    if (obj == _Py_NULL) {
        return _Py_NULL;
    }
    Py_ssize_t refcnt = Py_REFCNT(obj);
    assert(refcnt >= 1);

    // gh-92138: For backward compatibility, functions of Python C API accepts
    // "const PyObject*". Check that using it does not emit C++ compiler
    // warnings.
    const PyObject *const_obj = obj;
    Py_INCREF(const_obj);
    Py_DECREF(const_obj);
    PyTypeObject *type = Py_TYPE(const_obj);
    assert(Py_REFCNT(const_obj) == refcnt);
    assert(type == &PyTuple_Type);
    assert(PyTuple_GET_SIZE(const_obj) == 2);
    PyObject *one = PyTuple_GET_ITEM(const_obj, 0);
    assert(PyLong_AsLong(one) == 1);

#ifdef __cplusplus
    // gh-92898: StrongRef doesn't inherit from PyObject but has an operator to
    // cast to PyObject*.
    StrongRef strong_ref(obj);
    assert(Py_TYPE(strong_ref) == &PyTuple_Type);
    assert(Py_REFCNT(strong_ref) == (refcnt + 1));
    Py_INCREF(strong_ref);
    Py_DECREF(strong_ref);
#endif

    // gh-93442: Pass 0 as NULL for PyObject*
    Py_XINCREF(0);
    Py_XDECREF(0);
#if _cplusplus >= 201103
    // Test nullptr passed as PyObject*
    Py_XINCREF(nullptr);
    Py_XDECREF(nullptr);
#endif

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyObject *
test_import(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *mod = PyImport_ImportModule("sys");
    if (mod == _Py_NULL) {
        return _Py_NULL;
    }
    Py_ssize_t refcnt = Py_REFCNT(mod);

    // test PyImport_AddModuleRef()
    PyObject *mod2 = PyImport_AddModuleRef("sys");
    if (mod2 == _Py_NULL) {
        return _Py_NULL;
    }
    assert(PyModule_Check(mod2));
    assert(Py_REFCNT(mod) == (refcnt + 1));

    Py_DECREF(mod2);
    Py_DECREF(mod);

    Py_RETURN_NONE;
}


static void
gc_collect(void)
{
#if defined(PYPY_VERSION)
    PyObject *mod = PyImport_ImportModule("gc");
    assert(mod != _Py_NULL);

    PyObject *res = PyObject_CallMethod(mod, "collect", _Py_NULL);
    Py_DECREF(mod);
    assert(res != _Py_NULL);
    Py_DECREF(res);
#else
    PyGC_Collect();
#endif
}


static PyObject *
func_varargs(PyObject *Py_UNUSED(module), PyObject *args, PyObject *kwargs)
{
    if (kwargs != _Py_NULL) {
        return PyTuple_Pack(2, args, kwargs);
    }
    else {
        return PyTuple_Pack(1, args);
    }
}


static void
check_int(PyObject *obj, int value)
{
#ifdef PYTHON3
    assert(PyLong_Check(obj));
    assert(PyLong_AsLong(obj) == value);
#else
    assert(PyInt_Check(obj));
    assert(PyInt_AsLong(obj) == value);
#endif
}


static PyObject *
test_weakref(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Create a new heap type, create an instance of this type, and delete the
    // type. This object supports weak references.
    PyObject *new_type = PyObject_CallFunction((PyObject*)&PyType_Type,
                                               "s(){}", "TypeName");
    if (new_type == _Py_NULL) {
        return _Py_NULL;
    }
    PyObject *obj = PyObject_CallNoArgs(new_type);
    Py_DECREF(new_type);
    if (obj == _Py_NULL) {
        return _Py_NULL;
    }
    Py_ssize_t refcnt = Py_REFCNT(obj);

    // create a weak reference
    PyObject *weakref = PyWeakref_NewRef(obj, _Py_NULL);
    if (weakref == _Py_NULL) {
        return _Py_NULL;
    }

    // test PyWeakref_GetRef(), reference is alive
    PyObject *ref = Py_True;  // marker to check that value was set
    assert(PyWeakref_GetRef(weakref, &ref) == 1);
    assert(ref == obj);
    assert(Py_REFCNT(obj) == (refcnt + 1));
    Py_DECREF(ref);

    // delete the referenced object: clear the weakref
    Py_DECREF(obj);
    gc_collect();

    // test PyWeakref_GetRef(), reference is dead
    ref = Py_True;
    assert(PyWeakref_GetRef(weakref, &ref) == 0);
    assert(ref == _Py_NULL);

    // test PyWeakref_GetRef(), invalid type
    PyObject *invalid_weakref = Py_None;
    assert(!PyErr_Occurred());
    ref = Py_True;
    assert(PyWeakref_GetRef(invalid_weakref, &ref) == -1);
    assert(PyErr_ExceptionMatches(PyExc_TypeError));
    assert(ref == _Py_NULL);
    PyErr_Clear();

#ifndef PYPY_VERSION
    // test PyWeakref_GetRef(NULL)
    ref = Py_True;
    assert(PyWeakref_GetRef(_Py_NULL, &ref) == -1);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    assert(ref == _Py_NULL);
    PyErr_Clear();
#endif

    Py_DECREF(weakref);
    Py_RETURN_NONE;
}


static void
test_vectorcall_noargs(PyObject *func_varargs)
{
    PyObject *res = PyObject_Vectorcall(func_varargs, _Py_NULL, 0, _Py_NULL);
    assert(res != _Py_NULL);

    assert(PyTuple_Check(res));
    assert(PyTuple_GET_SIZE(res) == 1);
    PyObject *posargs = PyTuple_GET_ITEM(res, 0);

    assert(PyTuple_Check(posargs));
    assert(PyTuple_GET_SIZE(posargs) == 0);

    Py_DECREF(res);
}


static void
test_vectorcall_args(PyObject *func_varargs)
{
    PyObject *args_tuple = Py_BuildValue("ii", 1, 2);
    assert(args_tuple != _Py_NULL);
    size_t nargs = (size_t)PyTuple_GET_SIZE(args_tuple);
    PyObject **args = &PyTuple_GET_ITEM(args_tuple, 0);

    PyObject *res = PyObject_Vectorcall(func_varargs, args, nargs, _Py_NULL);
    Py_DECREF(args_tuple);
    assert(res != _Py_NULL);

    assert(PyTuple_Check(res));
    assert(PyTuple_GET_SIZE(res) == 1);
    PyObject *posargs = PyTuple_GET_ITEM(res, 0);

    assert(PyTuple_Check(posargs));
    assert(PyTuple_GET_SIZE(posargs) == 2);
    check_int(PyTuple_GET_ITEM(posargs, 0), 1);
    check_int(PyTuple_GET_ITEM(posargs, 1), 2);

    Py_DECREF(res);
}


static void
test_vectorcall_args_offset(PyObject *func_varargs)
{
    // args contains 3 values, but only pass 2 last values
    PyObject *args_tuple = Py_BuildValue("iii", 1, 2, 3);
    assert(args_tuple != _Py_NULL);
    size_t nargs = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject **args = &PyTuple_GET_ITEM(args_tuple, 1);
    PyObject *arg0 = PyTuple_GET_ITEM(args_tuple, 0);

    PyObject *res = PyObject_Vectorcall(func_varargs, args, nargs, _Py_NULL);
    assert(PyTuple_GET_ITEM(args_tuple, 0) == arg0);
    Py_DECREF(args_tuple);
    assert(res != _Py_NULL);

    assert(PyTuple_Check(res));
    assert(PyTuple_GET_SIZE(res) == 1);
    PyObject *posargs = PyTuple_GET_ITEM(res, 0);

    assert(PyTuple_Check(posargs));
    assert(PyTuple_GET_SIZE(posargs) == 2);
    check_int(PyTuple_GET_ITEM(posargs, 0), 2);
    check_int(PyTuple_GET_ITEM(posargs, 1), 3);

    Py_DECREF(res);
}


static void
test_vectorcall_args_kwnames(PyObject *func_varargs)
{
    PyObject *args_tuple = Py_BuildValue("iiiii", 1, 2, 3, 4, 5);
    assert(args_tuple != _Py_NULL);
    PyObject **args = &PyTuple_GET_ITEM(args_tuple, 0);

#ifdef PYTHON3
    PyObject *key1 = PyUnicode_FromString("key1");
    PyObject *key2 = PyUnicode_FromString("key2");
#else
    PyObject *key1 = PyString_FromString("key1");
    PyObject *key2 = PyString_FromString("key2");
#endif
    assert(key1 != _Py_NULL);
    assert(key2 != _Py_NULL);
    PyObject *kwnames = PyTuple_Pack(2, key1, key2);
    assert(kwnames != _Py_NULL);
    size_t nargs = (size_t)(PyTuple_GET_SIZE(args_tuple) - PyTuple_GET_SIZE(kwnames));

    PyObject *res = PyObject_Vectorcall(func_varargs, args, nargs, kwnames);
    Py_DECREF(args_tuple);
    Py_DECREF(kwnames);
    assert(res != _Py_NULL);

    assert(PyTuple_Check(res));
    assert(PyTuple_GET_SIZE(res) == 2);
    PyObject *posargs = PyTuple_GET_ITEM(res, 0);
    PyObject *kwargs = PyTuple_GET_ITEM(res, 1);

    assert(PyTuple_Check(posargs));
    assert(PyTuple_GET_SIZE(posargs) == 3);
    check_int(PyTuple_GET_ITEM(posargs, 0), 1);
    check_int(PyTuple_GET_ITEM(posargs, 1), 2);
    check_int(PyTuple_GET_ITEM(posargs, 2), 3);

    assert(PyDict_Check(kwargs));
    assert(PyDict_Size(kwargs) == 2);

    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
#ifdef PYTHON3
        assert(PyUnicode_Check(key));
#else
        assert(PyString_Check(key));
#endif
        if (PyObject_RichCompareBool(key, key1, Py_EQ)) {
            check_int(value, 4);
        }
        else {
            assert(PyObject_RichCompareBool(key, key2, Py_EQ));
            check_int(value, 5);
        }
    }

    Py_DECREF(res);
    Py_DECREF(key1);
    Py_DECREF(key2);
}


static PyObject *
test_vectorcall(PyObject *module, PyObject *Py_UNUSED(args))
{
#ifndef PYTHON3
    module = PyImport_ImportModule(MODULE_NAME_STR);
    assert(module != _Py_NULL);
#endif
    PyObject *func_varargs = PyObject_GetAttrString(module, "func_varargs");
#ifndef PYTHON3
    Py_DECREF(module);
#endif
    if (func_varargs == _Py_NULL) {
        return _Py_NULL;
    }

    // test PyObject_Vectorcall()
    test_vectorcall_noargs(func_varargs);
    test_vectorcall_args(func_varargs);
    test_vectorcall_args_offset(func_varargs);
    test_vectorcall_args_kwnames(func_varargs);

    Py_DECREF(func_varargs);
    Py_RETURN_NONE;
}


static PyObject *
test_getattr(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    assert(!PyErr_Occurred());

    PyObject *obj = PyImport_ImportModule("sys");
    if (obj == _Py_NULL) {
        return _Py_NULL;
    }
    PyObject *attr_name;
    PyObject *value;

    // test PyObject_GetOptionalAttr(): attribute exists
    attr_name = create_string("version");
    value = Py_True;  // marker value
    assert(PyObject_GetOptionalAttr(obj, attr_name, &value) == 1);
    assert(value != _Py_NULL);
    Py_DECREF(value);
    Py_DECREF(attr_name);

    // test PyObject_GetOptionalAttrString(): attribute exists
    value = Py_True;  // marker value
    assert(PyObject_GetOptionalAttrString(obj, "version", &value) == 1);
    assert(value != _Py_NULL);
    Py_DECREF(value);

    // test PyObject_GetOptionalAttr(): attribute doesn't exist
    attr_name = create_string("nonexistant_attr_name");
    value = Py_True;  // marker value
    assert(PyObject_GetOptionalAttr(obj, attr_name, &value) == 0);
    assert(value == _Py_NULL);
    Py_DECREF(attr_name);
    assert(!PyErr_Occurred());

    // test PyObject_GetOptionalAttrString(): attribute doesn't exist
    value = Py_True;  // marker value
    assert(PyObject_GetOptionalAttrString(obj, "nonexistant_attr_name", &value) == 0);
    assert(value == _Py_NULL);
    assert(!PyErr_Occurred());

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyObject *
test_getitem(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    assert(!PyErr_Occurred());

    PyObject *value = Py_BuildValue("s", "value");
    assert(value != _Py_NULL);
    PyObject *obj = Py_BuildValue("{sO}", "key", value);
    assert(obj != _Py_NULL);
    PyObject *key;
    PyObject *item;

    // test PyMapping_GetOptionalItem(): key is present
    key = create_string("key");
    item = Py_True;  // marker value
    assert(PyMapping_GetOptionalItem(obj, key, &item) == 1);
    assert(item == value);
    Py_DECREF(item);
    Py_DECREF(key);

    // test PyMapping_GetOptionalItemString(): key is present
    item = Py_True;  // marker value
    assert(PyMapping_GetOptionalItemString(obj, "key", &item) == 1);
    assert(item == value);
    Py_DECREF(item);

    // test PyMapping_GetOptionalItem(): missing key
    key = create_string("dontexist");
    item = Py_True;  // marker value
    assert(PyMapping_GetOptionalItem(obj, key, &item) == 0);
    assert(item == _Py_NULL);
    Py_DECREF(key);

    // test PyMapping_GetOptionalItemString(): missing key
    item = Py_True;  // marker value
    assert(PyMapping_GetOptionalItemString(obj, "dontexist", &item) == 0);
    assert(item == _Py_NULL);

    Py_DECREF(obj);
    Py_DECREF(value);
    Py_RETURN_NONE;
}


static PyObject *
test_dict_getitemref(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    assert(!PyErr_Occurred());

    PyObject *dict = NULL, *key = NULL, *missing_key = NULL, *value = NULL;
    PyObject *invalid_key = NULL;
    PyObject *invalid_dict = NULL;
    PyObject *get_value = NULL;
    int res;

    // test PyDict_New()
    dict = PyDict_New();
    if (dict == NULL) {
        goto error;
    }

    key = PyUnicode_FromString("key");
    if (key == NULL) {
        goto error;
    }

    missing_key = PyUnicode_FromString("missing_key");
    if (missing_key == NULL) {
        goto error;
    }

    value = PyUnicode_FromString("value");
    if (value == NULL) {
        goto error;
    }

    res = PyDict_SetItemString(dict, "key", value);
    if (res < 0) {
        goto error;
    }
    assert(res == 0);

    // test PyDict_GetItemRef(), key is present
    get_value = Py_Ellipsis;  // marker value
    assert(PyDict_GetItemRef(dict, key, &get_value) == 1);
    assert(get_value == value);
    Py_DECREF(get_value);

    // test PyDict_GetItemStringRef(), key is present
    get_value = Py_Ellipsis;  // marker value
    assert(PyDict_GetItemStringRef(dict, "key", &get_value) == 1);
    assert(get_value == value);
    Py_DECREF(get_value);

    // test PyDict_GetItemRef(), missing key
    get_value = Py_Ellipsis;  // marker value
    assert(PyDict_GetItemRef(dict, missing_key, &get_value) == 0);
    assert(!PyErr_Occurred());
    assert(get_value == NULL);

    // test PyDict_GetItemStringRef(), missing key
    get_value = Py_Ellipsis;  // marker value
    assert(PyDict_GetItemStringRef(dict, "missing_key", &get_value) == 0);
    assert(!PyErr_Occurred());
    assert(get_value == NULL);

    // test PyDict_GetItemRef(), invalid dict
    invalid_dict = key;  // borrowed reference
    get_value = Py_Ellipsis;  // marker value
    assert(PyDict_GetItemRef(invalid_dict, key, &get_value) == -1);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    PyErr_Clear();
    assert(get_value == NULL);

    // test PyDict_GetItemStringRef(), invalid dict
    get_value = Py_Ellipsis;  // marker value
    assert(PyDict_GetItemStringRef(invalid_dict, "key", &get_value) == -1);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    PyErr_Clear();
    assert(get_value == NULL);

    invalid_key = PyList_New(0);  // not hashable key
    if (invalid_key == NULL) {
        goto error;
    }

    // test PyDict_GetItemRef(), invalid key
    get_value = Py_Ellipsis;  // marker value
    assert(PyDict_GetItemRef(dict, invalid_key, &get_value) == -1);
    assert(PyErr_ExceptionMatches(PyExc_TypeError));
    PyErr_Clear();
    assert(get_value == NULL);

    Py_DECREF(dict);
    Py_DECREF(key);
    Py_DECREF(missing_key);
    Py_DECREF(value);
    Py_DECREF(invalid_key);

    Py_RETURN_NONE;

error:
    Py_XDECREF(dict);
    Py_XDECREF(key);
    Py_XDECREF(missing_key);
    Py_XDECREF(value);
    Py_XDECREF(invalid_key);
    return NULL;
}


static struct PyMethodDef methods[] = {
    {"test_object", test_object, METH_NOARGS, _Py_NULL},
    {"test_py_is", test_py_is, METH_NOARGS, _Py_NULL},
#ifndef PYPY_VERSION
    {"test_frame", test_frame, METH_NOARGS, _Py_NULL},
#endif
    {"test_thread_state", test_thread_state, METH_NOARGS, _Py_NULL},
    {"test_interpreter", test_interpreter, METH_NOARGS, _Py_NULL},
    {"test_calls", test_calls, METH_NOARGS, _Py_NULL},
    {"test_gc", test_gc, METH_NOARGS, _Py_NULL},
    {"test_module", test_module, METH_NOARGS, _Py_NULL},
#if (PY_VERSION_HEX <= 0x030B00A1 || 0x030B00A7 <= PY_VERSION_HEX) && !defined(PYPY_VERSION)
    {"test_float_pack", test_float_pack, METH_NOARGS, _Py_NULL},
#endif
#ifndef PYPY_VERSION
    {"test_code", test_code, METH_NOARGS, _Py_NULL},
#endif
    {"test_api_casts", test_api_casts, METH_NOARGS, _Py_NULL},
    {"test_import", test_import, METH_NOARGS, _Py_NULL},
    {"test_weakref", test_weakref, METH_NOARGS, _Py_NULL},
    {"func_varargs", (PyCFunction)(void*)func_varargs, METH_VARARGS | METH_KEYWORDS, _Py_NULL},
    {"test_vectorcall", test_vectorcall, METH_NOARGS, _Py_NULL},
    {"test_getattr", test_getattr, METH_NOARGS, _Py_NULL},
    {"test_getitem", test_getitem, METH_NOARGS, _Py_NULL},
    {"test_dict_getitemref", test_dict_getitemref, METH_NOARGS, _Py_NULL},
    {_Py_NULL, _Py_NULL, 0, _Py_NULL}
};


#ifdef __cplusplus
static int
module_exec(PyObject *module)
{
    if (PyModule_AddIntMacro(module, __cplusplus)) {
        return -1;
    }
    return 0;
}
#else
static int
module_exec(PyObject *Py_UNUSED(module))
{
    return 0;
}
#endif


#if PY_VERSION_HEX >= 0x03050000
static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, _Py_CAST(void*, module_exec)},
    {0, _Py_NULL}
};
#endif


#ifdef PYTHON3
static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    MODULE_NAME_STR,     // m_name
    _Py_NULL,            // m_doc
    0,                   // m_doc
    methods,             // m_methods
#if PY_VERSION_HEX >= 0x03050000
    module_slots,        // m_slots
#else
    _Py_NULL,            // m_reload
#endif
    _Py_NULL,            // m_traverse
    _Py_NULL,            // m_clear
    _Py_NULL,            // m_free
};


#define INIT_FUNC CONCAT(PyInit_, MODULE_NAME)

#if PY_VERSION_HEX >= 0x03050000
PyMODINIT_FUNC
INIT_FUNC(void)
{
    return PyModuleDef_Init(&module_def);
}
#else
// Python 3.4
PyMODINIT_FUNC
INIT_FUNC(void)
{
    PyObject *module = PyModule_Create(&module_def);
    if (module == _Py_NULL) {
        return _Py_NULL;
    }
    if (module_exec(module) < 0) {
        Py_DECREF(module);
        return _Py_NULL;
    }
    return module;
}
#endif

#else
// Python 2

#define INIT_FUNC CONCAT(init, MODULE_NAME)

PyMODINIT_FUNC
INIT_FUNC(void)
{
    PyObject *module;
    module = Py_InitModule4(MODULE_NAME_STR,
                            methods,
                            _Py_NULL,
                            _Py_NULL,
                            PYTHON_API_VERSION);
    if (module == _Py_NULL) {
        return;
    }

    if (module_exec(module) < 0) {
        return;
    }
}
#endif
