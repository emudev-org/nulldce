#pragma once
#define OP_ON 1
#define OP_OFF 2
#include "types.h"
//TA stuff


//Debugging stuff
#define DO_VERIFY OP_OFF


//DO NOT EDIT -- overrides for default acording to build options
#ifdef _DEBUG
#undef DO_VERIFY
#define DO_VERIFY OP_ON
#endif

