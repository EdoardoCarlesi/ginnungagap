// Copyright (C) 2010, 2013, Steffen Knollmann
// Released under the terms of the GNU General Public License version 3.
// This file is part of `ginnungagap'.


/*--- Includes ----------------------------------------------------------*/
#include "partConfig.h"
#include "partBunch_tests.h"
#include "partBunch.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#ifdef WITH_MPI
#  include <mpi.h>
#endif
#ifdef XMEM_TRACK_MEM
#  include "../libutil/xmem.h"
#endif
#include "../libdata/dataVar.h"
#include "../libdata/dataParticle.h"


/*--- Implemention of main structure ------------------------------------*/
#include "partBunch_adt.h"


/*--- Local defines -----------------------------------------------------*/


/*--- Prototypes of local functions -------------------------------------*/
static dataParticle_t
local_getSamplePartDesc(void);

static partBunch_t
local_getSamplePartBunch(void);


/*--- Implementations of exported functios ------------------------------*/
extern bool
partBunch_new_test(void)
{
	bool           hasPassed = true;
	int            rank      = 0;
	partBunch_t    partBunch;
	dataParticle_t desc;
#ifdef XMEM_TRACK_MEM
	size_t         allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	desc      = local_getSamplePartDesc();

	partBunch = partBunch_new(dataParticle_getRef(desc), UINT64_C(256));
	if (partBunch->desc != desc)
		hasPassed = false;
	if (partBunch->numParticles != UINT64_C(256))
		hasPassed = false;
	if (varArr_getLength(partBunch->data) != 3)
		hasPassed = false;
	if (partBunch->isAllocated != false)
		hasPassed = false;
	partBunch_del(&partBunch);

	dataParticle_del(&desc);

#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
} /* partBunch_new_test */

extern bool
partBunch_del_test(void)
{
	bool        hasPassed = true;
	int         rank      = 0;
	partBunch_t partBunch;
#ifdef XMEM_TRACK_MEM
	size_t      allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	partBunch = local_getSamplePartBunch();
	partBunch_del(&partBunch);
	if (partBunch != NULL)
		hasPassed = false;
#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
}

extern bool
partBunch_allocMem_test(void)
{
	bool        hasPassed = true;
	int         rank      = 0;
	partBunch_t partBunch;
#ifdef XMEM_TRACK_MEM
	size_t      allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	partBunch = local_getSamplePartBunch();

	partBunch_allocMem(partBunch);
	if (partBunch->isAllocated != true)
		hasPassed = false;
	if (varArr_getElementHandle(partBunch->data, 0) == NULL)
		hasPassed = false;
	if (varArr_getElementHandle(partBunch->data, 1) == NULL)
		hasPassed = false;
	if (varArr_getElementHandle(partBunch->data, 2) == NULL)
		hasPassed = false;

	partBunch_del(&partBunch);
#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
} /* partBunch_allocMem_test */

extern bool
partBunch_freeMem_test(void)
{
	bool        hasPassed = true;
	int         rank      = 0;
	partBunch_t partBunch;
#ifdef XMEM_TRACK_MEM
	size_t      allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	partBunch = local_getSamplePartBunch();

	partBunch_allocMem(partBunch);
	partBunch_freeMem(partBunch);

	if (partBunch->isAllocated != false)
		hasPassed = false;
	if (varArr_getElementHandle(partBunch->data, 0) != NULL)
		hasPassed = false;
	if (varArr_getElementHandle(partBunch->data, 1) != NULL)
		hasPassed = false;
	if (varArr_getElementHandle(partBunch->data, 2) != NULL)
		hasPassed = false;

	partBunch_del(&partBunch);
#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
} /* partBunch_freeMem_test */

extern bool
partBunch_resize_test(void)
{
	bool        hasPassed = true;
	int         rank      = 0;
	partBunch_t partBunch;
#ifdef XMEM_TRACK_MEM
	size_t      allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	partBunch = local_getSamplePartBunch();

	partBunch_resize(partBunch, UINT64_C(64));
	if (partBunch->numParticles != UINT64_C(64))
		hasPassed = false;
	if (partBunch->isAllocated != false)
		hasPassed = false;

	partBunch_allocMem(partBunch);
	partBunch_resize(partBunch, UINT64_C(128));
	if (partBunch->numParticles != UINT64_C(128))
		hasPassed = false;
	if (partBunch->isAllocated != true)
		hasPassed = false;

	partBunch_del(&partBunch);
#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
} /* partBunch_resize_test */

extern bool
partBunch_isAllocated_test(void)
{
	bool        hasPassed = true;
	int         rank      = 0;
	partBunch_t partBunch;
#ifdef XMEM_TRACK_MEM
	size_t      allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	partBunch = local_getSamplePartBunch();

	if (partBunch->isAllocated != partBunch_isAllocated(partBunch))
		hasPassed = false;

	partBunch_allocMem(partBunch);
	if (partBunch->isAllocated != partBunch_isAllocated(partBunch))
		hasPassed = false;

	partBunch_del(&partBunch);
#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
}

extern bool
partBunch_getNumParticles_test(void)
{
	bool        hasPassed = true;
	int         rank      = 0;
	partBunch_t partBunch;
#ifdef XMEM_TRACK_MEM
	size_t      allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	partBunch = local_getSamplePartBunch();

	if (partBunch->numParticles != partBunch_getNumParticles(partBunch))
		hasPassed = false;

	partBunch_del(&partBunch);
#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
}

extern bool
partBunch_at_test(void)
{
	bool        hasPassed = true;
	int         rank      = 0;
	partBunch_t partBunch;
#ifdef XMEM_TRACK_MEM
	size_t      allocatedBytes = global_allocated_bytes;
#endif
#ifdef WITH_MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

	if (rank == 0)
		printf("Testing %s... ", __func__);

	partBunch = local_getSamplePartBunch();
	partBunch_allocMem(partBunch);

	double *var0 = varArr_getElementHandle(partBunch->data, 0);
	double *var1 = varArr_getElementHandle(partBunch->data, 1);
	double *var2 = varArr_getElementHandle(partBunch->data, 2);

	if (var0 != partBunch_at(partBunch, 0, 0))
		hasPassed = false;
	if (var1 != partBunch_at(partBunch, 1, 0))
		hasPassed = false;
	if (var2 != partBunch_at(partBunch, 2, 0))
		hasPassed = false;

	if (var0 + 5 != partBunch_at(partBunch, 0, 5))
		hasPassed = false;
	if (var1 + 10 * 3 != partBunch_at(partBunch, 1, 10))
		hasPassed = false;
	if (var2 +100 != partBunch_at(partBunch, 2, 100))
		hasPassed = false;

	partBunch_del(&partBunch);
#ifdef XMEM_TRACK_MEM
	if (allocatedBytes != global_allocated_bytes)
		hasPassed = false;
#endif

	return hasPassed ? true : false;
}

/*--- Implementations of local functions --------------------------------*/
static dataParticle_t
local_getSamplePartDesc(void)
{
	dataVar_t      var;
	dataParticle_t dataParticle = dataParticle_new("Sample Particle", 0, 3);

	var = dataVar_new("Var1", DATAVARTYPE_DOUBLE, 1);
	dataParticle_addVar(dataParticle, var);

	var = dataVar_new("Var2", DATAVARTYPE_DOUBLE, 3);
	dataParticle_addVar(dataParticle, var);

	var = dataVar_new("Var3", DATAVARTYPE_DOUBLE, 1);
	dataParticle_addVar(dataParticle, var);

	dataParticle_lock(dataParticle);

	return dataParticle;
}

static partBunch_t
local_getSamplePartBunch(void)
{
	partBunch_t    bunch = NULL;
	dataParticle_t desc  = local_getSamplePartDesc();

	bunch = partBunch_new(desc, UINT64_C(256));

	return bunch;
}
