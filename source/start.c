#include <windows.h>
#include <shlobj.h>
#include <Python.h>
#include <marshal.h>

#include "MyLoadLibrary.h"
#include "python-dynload.h"

#include <fcntl.h>


struct scriptinfo {
	int tag;
	int optimize;
	int unbuffered;
	int data_bytes;

	char zippath[0];
};

PyMODINIT_FUNC PyInit__memimporter(void);
extern void SystemError(int error, char *msg);

int run_script(void);
void fini(void);
char *pScript;
char *pZipBaseName;
int numScriptBytes;
wchar_t modulename[_MAX_PATH + _MAX_FNAME + _MAX_EXT]; // from GetModuleName()
wchar_t dirname[_MAX_PATH]; // directory part of GetModuleName()
// Hm, do we need this? wchar_t libdirname[_MAX_PATH]; // library directory - probably same as above.
wchar_t libfilename[_MAX_PATH + _MAX_FNAME + _MAX_EXT]; // library filename
struct scriptinfo *p_script_info;


BOOL calc_dirname(HMODULE hmod)
{
	int is_special;
	wchar_t *modulename_start;
	wchar_t *cp;

	// get module filename
	if (!GetModuleFileNameW(hmod, modulename, sizeof(modulename))) {
		SystemError(GetLastError(), "Retrieving module name");
		return FALSE;
	}
	// get directory of modulename.  Note that in some cases
	// (eg, ISAPI), GetModuleFileName may return a leading "\\?\"
	// (which is a special format you can pass to the Unicode API
	// to avoid MAX_PATH limitations).  Python currently can't understand
	// such names, and as it uses the ANSI API, neither does Windows!
	// So fix that up here.
	is_special = wcslen(modulename) > 4 &&
		wcsncmp(modulename, L"\\\\?\\", 4)==0;
	modulename_start = is_special ? modulename + 4 : modulename;
	wcscpy(dirname, modulename_start);
	cp = wcsrchr(dirname, L'\\');
	*cp = L'\0';
	return TRUE;
}


BOOL locate_script(HMODULE hmod)
{
	HRSRC hrsrc = FindResourceA(hmod, MAKEINTRESOURCEA(1), "PYTHONSCRIPT");
	HGLOBAL hgbl;

	// load the script resource
	if (!hrsrc) {
		SystemError(GetLastError(), "Could not locate script resource:");
		return FALSE;
	}
	hgbl = LoadResource(hmod, hrsrc);
	if (!hgbl) {
		SystemError(GetLastError(), "Could not load script resource:");
		return FALSE;
	}
	p_script_info = (struct scriptinfo *)pScript = LockResource(hgbl);
	if (!pScript)  {
		SystemError(GetLastError(), "Could not lock script resource:");
		return FALSE;
	}
	// validate script resource
	numScriptBytes = p_script_info->data_bytes;
	pScript += sizeof(struct scriptinfo);
	if (p_script_info->tag != 0x78563412) {
		SystemError (0, "Bug: Invalid script resource");
		return FALSE;
	}

	// let pScript point to the start of the python script resource
	pScript = p_script_info->zippath + strlen(p_script_info->zippath) + 1;

	// get full pathname of the 'library.zip' file
	if (p_script_info->zippath[0]) {
		_snwprintf(libfilename, sizeof(libfilename),
			   L"%s\\%S", dirname, p_script_info->zippath);
	} else {
		GetModuleFileNameW(hmod, libfilename, sizeof(libfilename));
	}
	// if needed, libdirname should be initialized here.
	return TRUE; // success
}


void fini(void)
{

	if (getenv("PYTHONINSPECT") && Py_FdIsInteractive(stdin, "<stdin>"))
		PyRun_InteractiveLoop(stdin, "<stdin>");
	/* Clean up */
	Py_Finalize();
}


int run_script(void)
{
	int rc = 0;

	PyObject *m=NULL, *d=NULL, *seq=NULL;

	m = PyImport_AddModule("__main__");
	if (m) d = PyModule_GetDict(m);
	if (d) seq = PyMarshal_ReadObjectFromString(pScript, numScriptBytes);
	if (seq) {
		Py_ssize_t i, max = PySequence_Length(seq);
		for (i=0; i<max; i++) {
			PyObject *sub = PySequence_GetItem(seq, i);
			if (sub /*&& PyCode_Check(sub) */) {
				PyObject *discard = PyEval_EvalCode(sub, d, d);
				if (!discard) {
					PyErr_Print();
					rc = 255;
				}
				Py_XDECREF(discard);
	
			}
			Py_XDECREF(sub);
		}
	}
	return rc;
}



void set_vars(HMODULE hmod_pydll)
{
	int *pflag;


	if (p_script_info->unbuffered) {
		_setmode(_fileno(stdin), O_BINARY);
		_setmode(_fileno(stdout), O_BINARY);
		setvbuf(stdin,	(char *)NULL, _IONBF, 0);
		setvbuf(stdout, (char *)NULL, _IONBF, 0);
		setvbuf(stderr, (char *)NULL, _IONBF, 0);

		pflag = (int *)MyGetProcAddress(hmod_pydll, "Py_UnbufferedStdioFlag");
		if (pflag) *pflag = 1;
	}

	pflag = (int *)MyGetProcAddress(hmod_pydll, "Py_IsolatedFlag");
	if (pflag) *pflag = 1;

	pflag = (int *)MyGetProcAddress(hmod_pydll, "Py_NoSiteFlag");
	if (pflag) *pflag = 1;

	pflag = (int *)MyGetProcAddress(hmod_pydll, "Py_IgnoreEnvironmentFlag");
	if (pflag) *pflag = 1;

	pflag = (int *)MyGetProcAddress(hmod_pydll, "Py_NoUserSiteDirectory");
	if (pflag) *pflag = 1;

	pflag = (int *)MyGetProcAddress(hmod_pydll, "Py_OptimizeFlag");
	if (pflag) *pflag = p_script_info->optimize;

	pflag = (int *)MyGetProcAddress(hmod_pydll, "Py_VerboseFlag");
	if (pflag) {
		if (getenv("PEXE37_VERBOSE"))
			*pflag = atoi(getenv("PEXE37_VERBOSE"));
		else
			*pflag = 0;
	}
}

HMODULE load_pythondll(void)
{
	HMODULE hmod_pydll;
	HANDLE hrsrc;
	HMODULE hmod = LoadLibraryExW(libfilename, NULL, LOAD_LIBRARY_AS_DATAFILE);

	// Try to locate pythonxy.dll as resource in the exe
	hrsrc = FindResourceA(hmod, MAKEINTRESOURCEA(1), PYTHONDLL);
	if (hrsrc) {
		HGLOBAL hgbl;
		DWORD size;
		char *ptr;
		hgbl = LoadResource(hmod, hrsrc);
		size = SizeofResource(hmod, hrsrc);
		ptr = LockResource(hgbl);
		hmod_pydll = MyLoadLibrary(PYTHONDLL, ptr, NULL);
	} else
		/*
		  XXX We should probably call LoadLibraryEx with
		  LOAD_WITH_ALTERED_SEARCH_PATH so that really our own one is
		  used.
		 */
		hmod_pydll = LoadLibraryA(PYTHONDLL);
	FreeLibrary(hmod);
	return hmod_pydll;
}

int init_with_instance(HMODULE hmod_exe, char *frozen)
{

	int rc = 0;
	HMODULE hmod_pydll;

/*	Py_NoSiteFlag = 1; /* Suppress 'import site' */
/*	Py_InspectFlag = 1; /* Needed to determine whether to exit at SystemExit */

	calc_dirname(hmod_exe);
//	wprintf(L"modulename %s\n", modulename);
//	wprintf(L"dirname %s\n", dirname);

	if (!locate_script(hmod_exe)) {
		SystemError(-1, "FATAL ERROR: Could not locate script");
//		printf("FATAL ERROR locating script\n");
		return -1;
	}

	hmod_pydll = load_pythondll();
	if (hmod_pydll == NULL) {
		SystemError(-1, "FATAL ERROR: Could not load python library");
//		printf("FATAL Error: could not load python library\n");
		return -1;
	}
	if (PythonLoaded(hmod_pydll) < 0) {
		SystemError(-1, "FATAL ERROR: Failed to load some Python symbols");
//		printf("FATAL Error: failed to load some Python symbols\n");
		return -1;
	}

	set_vars(hmod_pydll);

	/*
	  _memimporter contains the magic which allows to load
	  dlls from memory, without unpacking them to the file-system.

	  It is compiled into all the exe-stubs.
	*/
	PyImport_AppendInittab("_memimporter", PyInit__memimporter);

	/*
	  Start the ball rolling.
	*/
	Py_SetProgramName(modulename);
	Py_SetPath(libfilename);
	Py_Initialize();



	*/
	if (frozen == NULL)
		PySys_SetObject("frozen", PyBool_FromLong(1));
	else {
		PyObject *o = PyUnicode_FromString(frozen);
		if (o) {
			PySys_SetObject("frozen", o);
			Py_DECREF(o);
		}
	}
	return rc;
}

int init(char *frozen)
{
	return init_with_instance(NULL, frozen);
}

static PyObject *Py_MessageBox(PyObject *self, PyObject *args)
{
	HWND hwnd;
	char *message;
	char *title = NULL;
	int flags = MB_OK;

	if (!PyArg_ParseTuple(args, "is|zi", &hwnd, &message, &title, &flags))
		return NULL;
	return PyLong_FromLong(MessageBoxA(hwnd, message, title, flags));
}

static PyObject *Py_SHGetSpecialFolderPath(PyObject *self, PyObject *args)
{
	wchar_t path[MAX_PATH];
	int nFolder;
	if (!PyArg_ParseTuple(args, "i", &nFolder))
		return NULL;
	SHGetSpecialFolderPathW(NULL, path, nFolder, TRUE);
	return PyUnicode_FromWideChar(path, -1);
}

PyMethodDef method[] = {
	{ "_MessageBox", Py_MessageBox, METH_VARARGS },
	{ "_SHGetSpecialFolderPath", Py_SHGetSpecialFolderPath, METH_VARARGS },
};


int start(int argc, wchar_t **argv)
{
	int rc;
	PyObject *mod;
	PySys_SetArgvEx(argc, argv, 0);

	mod = PyImport_ImportModule("sys");
	if (mod) {
		PyObject_SetAttrString(mod,
				       method[0].ml_name,
				       PyCFunction_New(&method[0], NULL));
		PyObject_SetAttrString(mod,
				       method[1].ml_name,
				       PyCFunction_New(&method[1], NULL));
	}

	rc = run_script();
	fini();
	return rc;
}
