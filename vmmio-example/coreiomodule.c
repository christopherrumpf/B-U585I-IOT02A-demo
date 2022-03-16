/*
 *
 * This file contains confidential information of Corellium LLC.
 * Unauthorized reproduction or distribution of this file is subject
 * to civil and criminal penalties.
 *
 * Copyright (C) 2021 Corellium LLC
 * All rights reserved.
 *
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <errno.h>
#include <sys/select.h>
#include "coreio.h"


typedef struct {
    PyObject *cpl;
    PyObject *cplp;
} py_completion;

typedef struct {
    PyObject *read;
    PyObject *write;
    PyObject *readp;
    PyObject *writep;
    PyObject *funcp; // Parameter passed to callbacks
} py_coreio_func_t;

static py_coreio_func_t *py_cb_funcs;

void pycb(void *cplp, int error)
{
    if (cplp == NULL) {
        PyErr_SetString(PyExc_SystemError, "pycb called without private argument");
        return;
    }

    py_completion *py_cpl = cplp;

    PyObject_CallFunction(py_cpl->cpl, "(OK)", py_cpl->cplp ?: Py_None, error);

    Py_XDECREF(py_cpl->cpl);
    Py_XDECREF(py_cpl->cplp);

    free(py_cpl);
}

PyDoc_STRVAR(connect_doc, \
"connect(target: str) -> int\n\n\
Connect to a VM.\n\
 target      string like \"10.10.0.3:23456\"\n\
Returns error flag.");

static PyObject *
py_coreio_connect(PyObject *self, PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"target", NULL};
    const char *target;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s", kwlist, &target))
        return NULL;

    int sts = coreio_connect(target);
    return PyLong_FromLong(sts);
}

PyDoc_STRVAR(disconnect_doc, \
"disconnect() -> None\n\n\
Close connection to a VM.");

static PyObject *
py_coreio_disconnect(PyObject *self, PyObject *args)
{
    coreio_disconnect();
    Py_XDECREF(py_cb_funcs->read);
    Py_XDECREF(py_cb_funcs->write);
    Py_XDECREF(py_cb_funcs->readp);
    Py_XDECREF(py_cb_funcs->writep);
    free(py_cb_funcs);
    py_cb_funcs = NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(dma_read_doc, \
"dma_read(mid: int, sid: int, addr: int, buf: bytearray, flags: int, [ cpl: Callable[cplp, int], None], cplp ]) -> int\n\n\
Start a DMA read.\n\
  mid         IOMMU ID (0 = direct)\n\
  sid         IOMMU region ID\n\
  addr        target address\n\
  buf         memory buffer to fill with data to size of buffer (writable python object supporting buffer protocol eg bytearray)\n\
  flags       bus access flags\n\
  cpl         optional completion function; if None, call is blocking.  Must accept cplp and error as an int as arguments.\n\
  cplp        optional completion function parameter\n\
Returns error flag.");

static PyObject *
py_coreio_dma_read(PyObject *self, PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"mid", "sid", "addr", "buf", "flags", "cpl", "cplp", NULL};
    unsigned mid, sid, flags;
    uint64_t addr;
    PyObject *ba;

    py_completion *py_cpl = calloc(1, sizeof(py_completion));
    if (py_cpl == NULL) {
        PyErr_SetString(PyExc_MemoryError, "unable to allocate memory for callbacks");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IIKYI|OO", kwlist, &mid, &sid, &addr, &ba, &flags, &py_cpl->cpl, &py_cpl->cplp))
        return NULL;

    Py_buffer buf;
    if (PyObject_GetBuffer(ba, &buf, PyBUF_WRITABLE) != 0)
        return NULL;

    Py_XINCREF(py_cpl->cpl);
    Py_XINCREF(py_cpl->cplp);

    int sts;
    if (py_cpl->cpl == NULL) {
        sts = coreio_dma_read(mid, sid, addr, buf.len, buf.buf, flags, NULL, NULL);
    } else {
        sts = coreio_dma_read(mid, sid, addr, buf.len, buf.buf, flags, &pycb, py_cpl);
    }
    PyBuffer_Release(&buf);
    return PyLong_FromLong(sts);
}

PyDoc_STRVAR(dma_write_doc, \
"dma_write(mid: int, sid: int, addr: int, buf: bytearray, flags: int, [ cpl: Callable[cplp, int], None], cplp ]) -> int\n\n\
Start a DMA write.\n\
  mid         IOMMU ID (0 = direct)\n\
  sid         IOMMU region ID\n\
  addr        target address\n\
  buf         memory buffer to write (python object supporting buffer protocol eg bytearray)\n\
  flags       bus access flags\n\
  cpl         optional completion function; if None, call is blocking.  Must accept cplp and error as an int as arguments.\n\
  cplp        optional completion function parameter\n\
Returns error flag.");

static PyObject *
py_coreio_dma_write(PyObject *self, PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"mid", "sid", "addr", "buf", "flags", "cpl", "cplp", NULL};
    unsigned mid, sid, flags;
    uint64_t addr;
    Py_buffer buf;

    py_completion *py_cpl = calloc(1, sizeof(py_completion));
    if (py_cpl == NULL) {
        PyErr_SetString(PyExc_MemoryError, "unable to allocate memory for callbacks");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "IIKy*I|OO", kwlist, &mid, &sid, &addr, &buf, &flags, &py_cpl->cpl, &py_cpl->cplp))
        return NULL;

    int sts;
    if (py_cpl->cpl == NULL) {
        sts = coreio_dma_write(mid, sid, addr, buf.len, buf.buf, flags, NULL, NULL);
    } else {
        if (!PyCallable_Check(py_cpl->cpl))
        {
            PyErr_SetString(PyExc_TypeError, "readp callback parameter must be callable");
            return NULL;
        }
        Py_XINCREF(py_cpl->cpl);
        Py_XINCREF(py_cpl->cplp);
        sts = coreio_dma_write(mid, sid, addr, buf.len, buf.buf, flags, &pycb, py_cpl);
    }
    PyBuffer_Release(&buf);
    return PyLong_FromLong(sts);
}

static int py_read_cb(void *funcp, uint64_t addr, size_t len, void *buf, unsigned flags)
{
    if (funcp == NULL)
    {
        PyErr_SetString(PyExc_SystemError, "pycb_read_cb called without callbacks structure");
        return 1;
    }
    py_coreio_func_t *func = funcp;
    if (func->read == NULL) {
        PyErr_SetString(PyExc_SystemError, "pycb_read_cb called without read callback");
        return 2;
    }
    PyObject *pybuf = PyByteArray_FromStringAndSize(buf, len);
    if (pybuf==NULL) {
        return 3;
    }
    PyObject *rv = PyObject_CallFunction(func->read, "(OKOK)", func->funcp ?: Py_None, addr, pybuf, flags);
    if (rv == NULL) {
        fprintf(stderr, "PY_READ_CB: cb returned error\n");
        PyErr_Print();
        return 4;
    }

    uint8_t *bufp = buf;
    char *newbuf = PyByteArray_AsString(pybuf);
    memcpy(bufp, newbuf, len);
    Py_XDECREF(pybuf);
    return PyLong_AsLong(rv);
}

static int py_write_cb(void *funcp, uint64_t addr, size_t len, void *buf, unsigned flags)
{
    if (funcp == NULL)
    {
        PyErr_SetString(PyExc_SystemError, "pycb_write_cb called without callbacks structure");
        return 1;
    }
    py_coreio_func_t *func = funcp;
    if (func->write == NULL)
    {
        PyErr_SetString(PyExc_SystemError, "pycb_write_cb called without write callback");
        return 2;
    }
    PyObject *pybuf = PyByteArray_FromStringAndSize(buf, len);
    if (pybuf == NULL)
    {
        // Not setting PyErr as PyByteArray should have already done that
        return 3;
    }
    PyObject *rv = PyObject_CallFunction(func->write, "(OKOK)", func->funcp ?: Py_None, addr, pybuf, flags);
    if (rv == NULL)
    {
        fprintf(stderr, "PY_WRITE_CB: cb returned error\n");
        PyErr_Print();
        return -ENOSYS;
    }

    Py_XDECREF(pybuf);
    return PyLong_AsLong(rv);
}

static int py_readp_cb(void *funcp, uint64_t addr, size_t len, void *buf1, void *buf2, unsigned flags)
{
    if (funcp == NULL)
    {
        PyErr_SetString(PyExc_SystemError, "pycb_readp_cb called without callbacks structure");
        return -EINVAL;
    }
    py_coreio_func_t *func = funcp;
    if (func->readp == NULL) {
        PyErr_SetString(PyExc_SystemError, "pycb_readp_cb called without readp callback");
        return -EINVAL;
    }

    PyObject *pybuf1 = PyByteArray_FromStringAndSize(buf1, len);
    if (pybuf1 == NULL)
        return -ENOMEM;

    PyObject *pybuf2 = PyByteArray_FromStringAndSize(buf2, len);
    if (pybuf2 == NULL)
        return -ENOMEM;

    PyObject *rv = PyObject_CallFunction(func->readp, "(OKOOK)", func->funcp ?: Py_None, addr, pybuf1, pybuf2, flags);
    if (rv == NULL)
    {
        // Error
        fprintf(stderr, "PY_READP_CB: cb returned error\n");
        PyErr_Print();
        return -ENOSYS;
    }

    uint8_t *bufp1 = buf1;
    char *newbuf1 = PyByteArray_AsString(pybuf1);
    memcpy(bufp1, newbuf1, len);
    Py_XDECREF(pybuf1);

    uint8_t *bufp2 = buf2;
    char *newbuf2 = PyByteArray_AsString(pybuf2);
    memcpy(bufp2, newbuf2, len);
    Py_XDECREF(pybuf2);
    return PyLong_AsLong(rv);
}

static int py_writep_cb(void *funcp, uint64_t addr, size_t len, void *buf1, void *buf2, unsigned flags)
{
    if (funcp == NULL)
    {
        PyErr_SetString(PyExc_SystemError, "pycb_writep_cb called without callbacks structure");
        return -EINVAL;
    }
    py_coreio_func_t *func = funcp;
    if (func->writep == NULL) {
        PyErr_SetString(PyExc_SystemError, "pycb_writep_cb called without writep callback");
        return -EINVAL;
    }

    PyObject *pybuf1 = PyByteArray_FromStringAndSize(buf1, len);
    if (pybuf1 == NULL)
        return -ENOMEM;

    PyObject *pybuf2 = PyByteArray_FromStringAndSize(buf2, len);
    if (pybuf2 == NULL)
        return -ENOMEM;

    PyObject *rv = PyObject_CallFunction(func->writep, "(OKOOK)", func->funcp ?: Py_None, addr, pybuf1, pybuf2, flags);
    if (rv == NULL)
    {
        fprintf(stderr, "PY_WRITE_CB: cb returned error\n");
        PyErr_Print();
        return -ENOSYS;
    }

    Py_XDECREF(pybuf1);
    Py_XDECREF(pybuf2);
    return PyLong_AsLong(rv);
}

PyDoc_STRVAR(register_doc, \
"register(rid, read, write, [ readp, writep, funcp ]) -> int\n\n\
Register an MMIO handler.\n\
  rid         MMIO range ID to register\n\
  read        io handler read function\n\
  write       io handler write function\n\
  readp       optional io handler read pair function\n\
  writep      optional io handler write pair function\n\
  funcp       optional io handler function parameter\n\
 Returns error flag.");

static PyObject *
py_coreio_register(PyObject *self, PyObject *args, PyObject *keywds)
{
    if (py_cb_funcs != NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Can only register to one target at a time.  Disconnect before attempting to re-register.");
        return NULL;
    }
    unsigned rid;
    py_coreio_func_t func = {0};
    static char *kwlist[] = {"rid", "read", "write", "readp", "writep", "funcp", NULL};
    coreio_func_t rwcallbacks = {
        .read = &py_read_cb,
        .write = &py_write_cb,
    };

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "iOO|OOO", kwlist,
                                     &rid, &func.read, &func.write, &func.readp, &func.writep, &func.funcp))
        return NULL;

    if (!PyCallable_Check(func.read))
    {
        PyErr_SetString(PyExc_TypeError, "read callback parameter must be callable");
        return NULL;
    }
    if (!PyCallable_Check(func.write)) {
        PyErr_SetString(PyExc_TypeError, "write callback parameter must be callable");
        return NULL;
    }
    if (func.readp && !PyCallable_Check(func.readp)) {
        PyErr_SetString(PyExc_TypeError, "readp callback parameter must be callable");
        return NULL;
    }
    if (func.writep && !PyCallable_Check(func.writep)) {
        PyErr_SetString(PyExc_TypeError, "writep callback parameter must be callable");
        return NULL;
    }

    py_coreio_func_t *funcptr = calloc(1, sizeof(py_coreio_func_t));
    if (funcptr == NULL) {
        PyErr_SetString(PyExc_MemoryError, "unable to allocate memory for callbacks");
        return NULL;
    }

    funcptr->read = func.read;
    Py_XINCREF(funcptr->read);
    funcptr->write = func.write;
    Py_XINCREF(funcptr->write);
    if (func.readp) {
        funcptr->readp = func.readp;
        Py_XINCREF(funcptr->readp);
        rwcallbacks.readp = &py_readp_cb;
    }
    if (func.writep) {
        funcptr->writep = func.writep;
        Py_XINCREF(funcptr->writep);
        rwcallbacks.writep = &py_writep_cb;
    }

    int sts = coreio_register(rid, &rwcallbacks, funcptr);
    py_cb_funcs = funcptr;
    return PyLong_FromLong(sts);
}

PyDoc_STRVAR(mainloop_doc, \
"mainloop(usec: int) -> int\n\n\
Simple implementation of a main loop.\n\
 usec        time to spend in loop, in microseconds; negative means forever\n\
Returns error flag.");

static PyObject *py_coreio_mainloop(PyObject *self, PyObject *args,
                                    PyObject *keywds) {
  static char *kwlist[] = {"usec", NULL};
  long long usec;
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "L", kwlist, &usec))
    return NULL;

  int sts = coreio_mainloop(usec);
  return PyLong_FromLong(sts);
}

PyDoc_STRVAR(irq_update_doc, \
"irq_update(rid, iid, state) -> int\n\n\
Send an IRQ update.\n\
 rid         MMIO range ID associated with IRQ\n\
 iid         IRQ index\n\
 state       IRQ line state (1 - active, 0 - inactive)\n\
Returns error flag.");

static PyObject *
py_coreio_irq_update(PyObject *self, PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"rid", "iid", "state", NULL};
    unsigned rid, iid, state;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "III", kwlist, &rid, &iid, &state))
        return NULL;

    int sts = coreio_irq_update(rid, iid, state);
    return PyLong_FromLong(sts);
}

static fd_set *
pylist_to_fdset(int *nfds, PyObject *pyfdlist, fd_set *fdset)
{
    if (!PyList_Check(pyfdlist)) {
        PyErr_SetString(PyExc_TypeError, "pylist_to_fdset expected a list type object containing fds");
        return NULL;
    }

    Py_ssize_t count = PyList_Size(pyfdlist);
    for (ssize_t i = 0; i < count; i++)
    {
        // TODO: Also accept .fileno()
        PyObject *pyfd = PyList_GetItem(pyfdlist, i);
        if (pyfd == NULL || !PyLong_Check(pyfd))
        {
            PyErr_SetString(PyExc_TypeError, "readfds must be a list containing only integers");
            return NULL;
        }
        int fd = PyLong_AsLong(pyfd);
        FD_SET(fd, fdset);
        if (fd >= *nfds)
            *nfds = fd+1;
    }
    return fdset;
}

PyDoc_STRVAR(preparefds_doc, \
"preparefds([readfds: list, writefds: list]) -> nfds, readable, writable\n\n\
Prepare fd_sets for select(2).\n\
 readfds     optional readfds to update\n\
 writefds    optional writefds to update\n\
Returns new nfds and updated list of readable and writable fds.");

static PyObject *
py_coreio_preparefds(PyObject *self, PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"readfds", "writefds", NULL};
    int nfds=0;
    PyObject *py_readfds=NULL, *py_writefds=NULL;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "|OO", kwlist, &py_readfds, &py_writefds))
        return NULL;

    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if (py_readfds != NULL && pylist_to_fdset(&nfds, py_readfds, &readfds) == NULL)
        return NULL;

    if (py_writefds != NULL && pylist_to_fdset(&nfds, py_writefds, &writefds) == NULL)
        return NULL;

    nfds = coreio_preparefds(nfds, &readfds, &writefds);

    py_readfds = PyList_New(0);
    py_writefds = PyList_New(0);
    for (int i = 0; i < nfds; i++) {
        if (FD_ISSET(i, &readfds))
            PyList_Append(py_readfds, PyLong_FromLong(i));
        if (FD_ISSET(i, &writefds))
            PyList_Append(py_writefds, PyLong_FromLong(i));
    }
    return PyTuple_Pack(3, PyLong_FromLong(nfds), py_readfds, py_writefds);
}

PyDoc_STRVAR(processfds_doc, \
"processfds(readable, writable) -> int\n\n\
Process fd_sets after select(2).\n\
 readfds     readfds to process\n\
 writefds    writefds to process\n\
Returns error flag.");

static PyObject *
py_coreio_processfds(PyObject *self, PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"readable", "writable", NULL};
    PyObject *py_readable = NULL, *py_writable = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "OO", kwlist, &py_readable, &py_writable))
        return NULL;

    if (!PyList_Check(py_readable))
    {
        PyErr_SetString(PyExc_TypeError, "readfds must be a list type object");
        return NULL;
    }

    if (!PyList_Check(py_writable))
    {
        PyErr_SetString(PyExc_TypeError, "writefds must be a list type object");
        return NULL;
    }

    fd_set readable, writable;
    FD_ZERO(&readable);
    FD_ZERO(&writable);

    int nfds = 0;
    if (pylist_to_fdset(&nfds, py_readable, &readable) == NULL)
        return NULL;

    if (pylist_to_fdset(&nfds, py_writable, &writable) == NULL)
        return NULL;

    int sts = coreio_processfds(&readable, &writable);
    return PyLong_FromLong(sts);
}

static PyObject *py_add_longattrib(PyObject *module, const char *attrib_name, long attrib_value) {
    PyObject *attrib = PyLong_FromLong(attrib_value);
    if (PyModule_AddObject(module, attrib_name, attrib) < 0)
    {
        Py_DECREF(attrib);
        return NULL;
    }
    return module;
}

static PyMethodDef CoreIOMethods[] = {
    {"connect", (PyCFunction)(void (*)(void))py_coreio_connect, METH_VARARGS | METH_KEYWORDS, connect_doc},
    {"disconnect", py_coreio_disconnect, METH_VARARGS, disconnect_doc},
    {"dma_read", (PyCFunction)(void (*)(void))py_coreio_dma_read, METH_VARARGS | METH_KEYWORDS, dma_read_doc},
    {"dma_write", (PyCFunction)(void (*)(void))py_coreio_dma_write, METH_VARARGS | METH_KEYWORDS, dma_write_doc},
    {"register", (PyCFunction)(void (*)(void))py_coreio_register, METH_VARARGS | METH_KEYWORDS, register_doc},
    {"mainloop", (PyCFunction)(void (*)(void))py_coreio_mainloop, METH_VARARGS | METH_KEYWORDS, mainloop_doc},
    {"irq_update", (PyCFunction)(void (*)(void))py_coreio_irq_update, METH_VARARGS | METH_KEYWORDS, irq_update_doc},
    {"preparefds", (PyCFunction)(void (*)(void))py_coreio_preparefds, METH_VARARGS | METH_KEYWORDS, preparefds_doc},
    {"processfds", (PyCFunction)(void (*)(void))py_coreio_processfds, METH_VARARGS | METH_KEYWORDS, processfds_doc},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef coreiomodule = {
    PyModuleDef_HEAD_INIT,
    "coreio", /* name of module */
    NULL,     /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                   or -1 if the module keeps state in global variables. */
    CoreIOMethods};

PyMODINIT_FUNC
PyInit_coreio(void)
{
    PyObject *coreio_module = PyModule_Create(&coreiomodule);
    if (coreio_module == NULL)
        return NULL;

    if (py_add_longattrib(coreio_module, "FLAGS_IFETCH", COREIO_FLAGS_IFETCH)==NULL ||
        py_add_longattrib(coreio_module, "FLAGS_PRIV", COREIO_FLAGS_PRIV)==NULL ||
        py_add_longattrib(coreio_module, "FLAGS_SECURE", COREIO_FLAGS_SECURE)==NULL)
    {
        // Not setting error as whatever failed should have already done that
        Py_DECREF(coreio_module);
        return NULL;
    }

    return coreio_module;
}
