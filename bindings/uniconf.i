%module "uniconf"


%{
#include "uniconf.h"
%}


/**
 * WvString to native string conversion.  These aren't horribly efficient
 * (for instance, it makes extra copies of both WvString and WvFastString
 * objects), deliberately for now to keep the code easier to follow.
 */

%typemap(python, typecheck, precedence = SWIG_TYPECHECK_INTEGER) WvStringParm
{
    $1 = ($input == NULL || $input == Py_None || PyString_Check($input)) ? 1 : 0;
}

%typemap(tcl8, typecheck, precedence = SWIG_TYPECHECK_STRING) WvStringParm
{
    /* All TCL objects are strings */
    $1 = 1;
}

%typemap(python, in) WvStringParm(WvString temp)
{
    if ($input == NULL || $input == Py_None)
        temp = WvString::null;
    else if (PyString_Check($input))
        temp = PyString_AsString($input);
    else
    {
        PyErr_SetString(PyExc_TypeError, "not a string");
        return NULL;
    }
    $1 = &temp;
}

%typemap(tcl8, in) WvStringParm(WvString temp)
{
    if ($input == NULL)
        temp = WvString::null;
    else
        temp = Tcl_GetString($input);
    $1 = &temp;
}
 
%typemap(python, out) WvStringParm
{
    if ($1.isnull())
    {
        Py_INCREF(Py_None);
        $result = Py_None;
    }
    else
        $result = PyString_FromString($1);
}

%typemap(tcl8, out) WvStringParm
{
    Tcl_SetStringObj($result, $1.edit(), $1.len());
}

%apply WvStringParm { WvString }


%import "wvstring.h"

%include "uniconf.h"
%include "uniconfroot.h"
