// Copyright (C) 2010, 2012, Steffen Knollmann
// Released under the terms of the GNU General Public License version 3.
// This file is part of `ginnungagap'.

#ifndef GRIDREADER_TESTS_H
#define GRIDREADER_TESTS_H


/*--- Doxygen file description ------------------------------------------*/

/**
 * @file libgrid/gridReader_tests.h
 * @ingroup libgridIOInInterfaceTests
 * @brief  This file provides the interface to the testing routines.
 */



/*--- Includes ----------------------------------------------------------*/
#include "gridConfig.h"
#include <stdbool.h>


/*--- Prototypes of exported functions ----------------------------------*/

/** @brief  Tests gridReader_setFileName(). */
extern bool
gridReader_setFileName_test(void);

/** @brief  Tests gridReader_overlayFileName(). */
extern bool
gridReader_overlayFileName_test(void);

/** @brief  Tests gridReader_getFileName(). */
extern bool
gridReader_getFileName_test(void);


/*--- Doxygen group definitions -----------------------------------------*/

/**
 * @defgroup libgridIOInInterfaceTests Tests
 * @ingroup libgridIOInInterface
 * @brief This provides the tests for the grid reader.
 */


#endif
