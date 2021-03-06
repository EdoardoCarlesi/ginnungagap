// Copyright (C) 2010, Steffen Knollmann
// Released under the terms of the GNU General Public License version 3.
// This file is part of `ginnungagap'.


/*--- Doxygen file description ------------------------------------------*/

/**
 * @file libgrid/gridRegular.c
 * @ingroup libgridRegular
 * @brief  This file provides the implemenation of regular grids.
 */


/*--- Includes ----------------------------------------------------------*/
#include "gridConfig.h"
#include "gridRegular.h"
#include "gridPoint.h"
#include "../libdata/dataVar.h"
#include "gridPatch.h"
#include <assert.h>
#include "../libutil/refCounter.h"
#include "../libutil/xmem.h"
#include "../libutil/xstring.h"


/*--- Implemention of main structure ------------------------------------*/
#include "gridRegular_adt.h"


/*--- Local defines -----------------------------------------------------*/


/*--- Prototypes of local functions -------------------------------------*/

/**
 * @brief  Recalculates the grid spacing.
 *
 * @param[in,out]  gridRegular
 *                    The grid for which to recalculate the spacing.  Must
 *                    not be @c NULL.
 *
 * @return  Returns nothing.
 */
inline static void
local_resetDelta(gridRegular_t gridRegular);


/**
 * @brief  Frees the patch storage of the grid.
 *
 * @param[in,out]  grid
 *                    The grid for which to delete the patches.  Must not be
 *                    @c NULL.
 *
 * @return  Returns nothing.
 */
inline static void
local_freePatches(gridRegular_t grid);


/**
 * @brief  Frees the variable storage of the grid.
 *
 * @param[in,out]  grid
 *                    The grid for which to delete the variables.  Must not
 *                    be @c NULL.
 */
inline static void
local_freeVars(gridRegular_t grid);


/**
 * @brief  Helper function to add a new variable to all attached patches.
 *
 * @param[in,out]  grid
 *                    The grid to which to add the variable.  Must not be
 *                    @c NULL.
 *
 * @return  Returns nothing.
 */
inline static void
local_addVarToAllPatches(gridRegular_t grid, dataVar_t var);


/**
 * @brief  Removes a given variable from all attached patches.
 *
 * @param[in,out]  grid
 *                    The grid for which to remove the variable.  Must not
 *                    be @c NULL.
 * @param[in]      idxOfVar
 *                    The index of the variable that should be removed.
 *                    Must be valid index.
 *
 * @return  Returns nothing.
 */
inline static void
local_removeVarFromAllPatches(gridRegular_t grid, int idxOfVar);


/**
 * @brief  Helper function to add all variables of a grid to a patch.
 *
 * @param[in,out]  grid
 *                    The grid that contains all the variables that should
 *                    be added to the provided @c patch.  Must not be
 *                    @c NULL.
 * @param[in,out]  patch
 *                    The patch to add the variables of the @c grid to.
 *                    Must not be @c NULL.  Note that the patch mustn't
 *                    contain any variables.
 *
 * @return  Returns nothing.
 */
inline static void
local_addAllVarsToPatch(gridRegular_t grid, gridPatch_t patch);


/*--- Implementations of exported functios ------------------------------*/
extern gridRegular_t
gridRegular_new(const char        *name,
                gridPointDbl_t    origin,
                gridPointDbl_t    extent,
                gridPointUint32_t dims)
{
	gridRegular_t gridRegular;

	for (int i = 0; i < NDIM; i++) {
		assert(dims[i] > 0);
		assert(extent[i] > 0.0);
	}

	gridRegular       = xmalloc(sizeof(struct gridRegular_struct));

	gridRegular->name = xstrdup(name);
	for (int i = 0; i < NDIM; i++) {
		gridRegular->origin[i]      = origin[i];
		gridRegular->extent[i]      = extent[i];
		gridRegular->dims[i]        = dims[i];
		gridRegular->dimsComplex[i] = dims[i];
		gridRegular->permute[i]     = i;
	}
	gridRegular->dimsComplex[0] = (gridRegular->dimsComplex[0]) / 2 + 1;
	local_resetDelta(gridRegular);
	gridRegular->patches        = varArr_new(1);
	gridRegular->vars           = varArr_new(1);

	refCounter_init(&(gridRegular->refCounter));

	return gridRegular_getRef(gridRegular);
}

extern gridRegular_t
gridRegular_cloneWithoutData(const gridRegular_t grid)
{
	assert(grid != NULL);

	return gridRegular_new(grid->name, grid->origin, grid->extent,
	                       grid->dims);
}

extern void
gridRegular_del(gridRegular_t *gridRegular)
{
	assert(gridRegular != NULL && *gridRegular != NULL);

	if (refCounter_deref(&((*gridRegular)->refCounter))) {
		xfree((*gridRegular)->name);
		local_freePatches(*gridRegular);
		local_freeVars(*gridRegular);
		xfree(*gridRegular);

		*gridRegular = NULL;
	}
}

extern gridRegular_t
gridRegular_getRef(gridRegular_t grid)
{
	assert(grid != NULL);

	refCounter_ref(&(grid->refCounter));

	return grid;
}

extern char *
gridRegular_getName(gridRegular_t grid)
{
	assert(grid != NULL);
	assert(grid->name != NULL);

	return grid->name;
}

extern void
gridRegular_getOrigin(gridRegular_t grid, gridPointDbl_t origin)
{
	assert(grid != NULL);
	assert(origin != NULL);

	for (int i = 0; i < NDIM; i++) {
		origin[i] = grid->origin[i];
	}
}

extern void
gridRegular_getExtent(gridRegular_t grid, gridPointDbl_t extent)
{
	assert(grid != NULL);
	assert(extent != NULL);

	for (int i = 0; i < NDIM; i++) {
		extent[i] = grid->extent[i];
	}
}

extern void
gridRegular_getDelta(gridRegular_t grid, gridPointDbl_t delta)
{
	assert(grid != NULL);
	assert(delta != NULL);

	for (int i = 0; i < NDIM; i++) {
		delta[i] = grid->delta[i];
	}
}

extern void
gridRegular_getDims(gridRegular_t grid, gridPointUint32_t dims)
{
	assert(grid != NULL);
	assert(dims != NULL);

	for (int i = 0; i < NDIM; i++)
		dims[i] = grid->dims[i];
}

extern void
gridRegular_getDimsComplex(gridRegular_t     grid,
                           gridPointUint32_t dimsComplex)
{
	assert(grid != NULL);
	assert(dimsComplex != NULL);

	for (int i = 0; i < NDIM; i++)
		dimsComplex[i] = grid->dimsComplex[i];
}

extern void
gridRegular_getPermute(gridRegular_t grid, gridPointInt_t permute)
{
	assert(grid != NULL);
	assert(permute != NULL);

	for (int i = 0; i < NDIM; i++)
		permute[i] = grid->permute[i];
}

extern int
gridRegular_getNumVars(gridRegular_t grid)
{
	assert(grid != NULL);

	return varArr_getLength(grid->vars);
}

extern uint64_t
gridRegular_getNumCellsTotal(gridRegular_t grid)
{
	uint64_t numCellsTotal = UINT64_C(1);

	assert(grid != NULL);

	for (int i = 0; i < NDIM; i++)
		numCellsTotal *= grid->dims[i];

	return numCellsTotal;
}

extern void
gridRegular_setComplexified(gridRegular_t grid, int idxOfVar)
{
	dataVar_t var;

	assert(grid != NULL);
	assert(idxOfVar >= 0 && idxOfVar < varArr_getLength(grid->vars));

	var = gridRegular_getVarHandle(grid, idxOfVar);
	dataVar_setComplexified(var);
}

extern void
gridRegular_unsetComplexified(gridRegular_t grid, int idxOfVar)
{
	dataVar_t var;

	assert(grid != NULL);
	assert(idxOfVar >= 0 && idxOfVar < varArr_getLength(grid->vars));

	var = gridRegular_getVarHandle(grid, idxOfVar);
	dataVar_unsetComplexified(var);
}

extern bool
gridRegular_isComplexified(gridRegular_t grid, int idxOfVar)
{
	dataVar_t var;

	assert(grid != NULL);
	assert(idxOfVar >= 0 && idxOfVar < varArr_getLength(grid->vars));

	var = gridRegular_getVarHandle(grid, idxOfVar);
	return dataVar_isComplexified(var);
}

extern int
gridRegular_attachVar(gridRegular_t grid, dataVar_t var)
{
	int idxOfVar;

	assert(grid != NULL);
	assert(var != NULL);

	idxOfVar = varArr_insert(grid->vars, var);
	local_addVarToAllPatches(grid, var);

	return idxOfVar;
}

extern dataVar_t
gridRegular_detachVar(gridRegular_t grid, int idxOfVar)
{
	assert(grid != NULL);
	assert(idxOfVar >= 0 && idxOfVar < varArr_getLength(grid->vars));

	local_removeVarFromAllPatches(grid, idxOfVar);

	return varArr_remove(grid->vars, idxOfVar);
}

extern dataVar_t
gridRegular_getVarHandle(gridRegular_t grid, int idxOfVar)
{
	assert(grid != NULL);
	assert(idxOfVar >= 0 && idxOfVar < varArr_getLength(grid->vars));

	return varArr_getElementHandle(grid->vars, idxOfVar);
}

extern void
gridRegular_freeVarData(gridRegular_t grid, int idxOfVarData)
{
	assert(grid != NULL);
	assert(idxOfVarData >= 0 && idxOfVarData < varArr_getLength(grid->vars));

	for (int i = 0; i < gridRegular_getNumPatches(grid); i++) {
		gridPatch_t patch;
		patch = gridRegular_getPatchHandle(grid, i);
		gridPatch_freeVarData(patch, idxOfVarData);
	}
}

extern int
gridRegular_getNumPatches(gridRegular_t grid)
{
	assert(grid != NULL);

	return varArr_getLength(grid->patches);
}

extern int
gridRegular_attachPatch(gridRegular_t grid, gridPatch_t patch)
{
	int idxOfPatch;

	assert(grid != NULL);
	assert(patch != NULL);

	local_addAllVarsToPatch(grid, patch);
	idxOfPatch = varArr_insert(grid->patches, patch);

	return idxOfPatch;
}

extern gridPatch_t
gridRegular_detachPatch(gridRegular_t grid, int idxOfPatch)
{
	assert(grid != NULL);
	assert(idxOfPatch >= 0 && idxOfPatch < varArr_getLength(grid->patches));

	return varArr_remove(grid->patches, idxOfPatch);
}

extern gridPatch_t
gridRegular_getPatchHandle(gridRegular_t grid, int idxPatchToGet)
{
	assert(grid != NULL);
	assert(idxPatchToGet >= 0
	       && idxPatchToGet < varArr_getLength(grid->patches));

	return varArr_getElementHandle(grid->patches, idxPatchToGet);
}

extern void
gridRegular_replacePatch(gridRegular_t grid,
                         int           idxOfPatch,
                         gridPatch_t   newPatch)
{
	gridPatch_t oldPatch;

	assert(grid != NULL);
	assert(idxOfPatch >= 0 && idxOfPatch < varArr_getLength(grid->patches));
	assert(newPatch != NULL);

	oldPatch = varArr_replace(grid->patches, idxOfPatch, newPatch);
	if (oldPatch != newPatch)
		gridPatch_del(&oldPatch);
}

/** @cond IGNORE */
#define LOCAL_SWAP(a, b, tmp) \
	{                         \
		tmp = a;              \
		a   = b;              \
		b   = tmp;            \
	}
/** @endcond */
extern void
gridRegular_transpose(gridRegular_t grid, int dimA, int dimB)
{
	double   tmpDbl;
	uint32_t tmpInt;
	int      tmpIntSign;

	assert(grid != NULL);
	assert(dimA >= 0 && dimA < NDIM);
	assert(dimB >= 0 && dimB < NDIM);

	LOCAL_SWAP(grid->origin[dimA], grid->origin[dimB], tmpDbl);
	LOCAL_SWAP(grid->extent[dimA], grid->extent[dimB], tmpDbl);
	LOCAL_SWAP(grid->delta[dimA], grid->delta[dimB], tmpDbl);

	LOCAL_SWAP(grid->dims[dimA], grid->dims[dimB], tmpInt);
	LOCAL_SWAP(grid->dimsComplex[dimA], grid->dimsComplex[dimB], tmpInt);

	LOCAL_SWAP(grid->permute[dimA], grid->permute[dimB], tmpIntSign);

	for (int i = 0; i < varArr_getLength(grid->patches); i++)
		gridPatch_transpose(varArr_getElementHandle(grid->patches, i),
		                    dimA, dimB);
}

#undef LOCAL_SWAP

extern int
gridRegular_getCurrentDim(const gridRegular_t grid, int originalDim)
{
	assert(grid != NULL);
	assert(originalDim >=0 && originalDim < NDIM);

	int currentDim = -1;

	for (int i = 0; i < NDIM; i++) {
		if (grid->permute[i] == originalDim) {
			currentDim = i;
			break;
		}
	}

	assert(currentDim != -1);

	return currentDim;
}

/*--- Implementations of local functions --------------------------------*/
inline static void
local_resetDelta(gridRegular_t gridRegular)
{
	for (int i = 0; i < NDIM; i++) {
		gridRegular->delta[i] = gridRegular->extent[i]
		                        / ((double)(gridRegular->dims[i]));
	}
}

inline static void
local_freePatches(gridRegular_t grid)
{
	while (varArr_getLength(grid->patches) != 0) {
		gridPatch_t patch;
		patch = varArr_remove(grid->patches, 0);
		gridPatch_del(&patch);
	}
	varArr_del(&(grid->patches));
}

inline static void
local_freeVars(gridRegular_t grid)
{
	while (varArr_getLength(grid->vars) != 0) {
		dataVar_t var;
		var = varArr_remove(grid->vars, 0);
		dataVar_del(&var);
	}
	varArr_del(&(grid->vars));
}

inline static void
local_addVarToAllPatches(gridRegular_t grid, dataVar_t var)
{
	int numPatches = varArr_getLength(grid->patches);

	for (int i = 0; i < numPatches; i++) {
		gridPatch_t patch = varArr_getElementHandle(grid->patches, i);
		gridPatch_attachVar(patch, var);
	}
}

inline static void
local_removeVarFromAllPatches(gridRegular_t grid, int idxOfVar)
{
	int numPatches = varArr_getLength(grid->patches);

	for (int i = 0; i < numPatches; i++) {
		gridPatch_t patch = varArr_getElementHandle(grid->patches, i);
		dataVar_t   var   = gridPatch_detachVar(patch, idxOfVar);
		dataVar_del(&var);
	}
}

inline static void
local_addAllVarsToPatch(gridRegular_t grid, gridPatch_t patch)
{
	int numVars = varArr_getLength(grid->vars);

	for (int i = 0; i < numVars; i++) {
		dataVar_t var = varArr_getElementHandle(grid->vars, i);
		gridPatch_attachVar(patch, var);
	}
}
