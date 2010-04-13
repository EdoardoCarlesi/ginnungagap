// Copyright (C) 2010, Steffen Knollmann
// Released under the terms of the GNU General Public License version 3.
// This file is part of `ginnungagap'.

#ifndef GRIDVARTYPE_TESTS_H
#define GRIDVARTYPE_TESTS_H


/*--- Includes ----------------------------------------------------------*/
#include "gridConfig.h"
#include <stdbool.h>


/*--- Prototypes of exported functions ----------------------------------*/
extern bool
gridVarType_sizeof_test(void);

extern bool
gridVarType_isFloating_test(void);

extern bool
gridVarType_isInteger_test(void);

extern bool
gridVarType_isNativeFloat_test(void);

extern bool
gridVarType_isNativeDouble_test(void);


#endif
