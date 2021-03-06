// Copyright (C) 2010, 2011, Steffen Knollmann
// Released under the terms of the GNU General Public License version 3.
// This file is part of `ginnungagap'.


/*--- Doxygen file description ------------------------------------------*/

/**
 * @file libcosmo/cosmoPk.c
 * @ingroup libcosmo libcosmoPowerspectrum
 * @brief  This file provides the implementations of power spectrum.
 */


/*--- Includes ----------------------------------------------------------*/
#include "cosmo_config.h"
#include "cosmoPk.h"
#include "cosmoFunc.h"
#include "../libutil/xmem.h"
#include "../libutil/xfile.h"
#include "../libutil/diediedie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_integration.h>


/*--- Implemention of the ADT structure ---------------------------------*/
#include "cosmoPk_adt.h"


/*--- Local defines -----------------------------------------------------*/

/**
 * @brief  Gives the maximal number of iteration to enforce sigma8.
 *
 * This is used when enforcing a certain sigma8, the method there is to
 * calculate the current sigma8, use that to compute a correction factor
 * for the power spectrum, scale the power spectrum and repeat from the
 * beginning unless an error criterion is fulfilled or the maximal
 * number of iterations are exhausted.
 */
#define LOCAL_MAX_FORCESIGMA8_ITERATIONS 42

/** @brief  Gives the size of the integration workspace. */
#define LOCAL_LIMIT 2048

/** @brief  Gives the error tolerance for the integration. */
#define LOCAL_EPSREL 1e-7

/** @brief Gives the minimal number of points in a power spectrum */
#define LOCAL_MINPOINTS 4

/**
 * @brief  Number of points to ignore at the beginning and end of the
 *         power spectrum.
 *
 * The spline interpolation tends to oscillate at the beginning and end
 * of the array, hence the first and last few points are not used when
 * evaluating the power spectrum (they are however used to generate the
 * spline interpolation).
 */
#define LOCAL_IGNOREPOINTS 2


/*--- Prototypes of local functions -------------------------------------*/
static cosmoPk_t
local_new(void);

static void
local_getMem(cosmoPk_t pk, uint32_t numPoints);

static void
local_doInterpolation(cosmoPk_t pk);

static cosmoPk_t
local_constructPkFromModel(parse_ini_t ini, const char *sectionName);

static void
local_initKs(cosmoPk_t pk, double kmin, double kmax);

static void
local_calcPkFromTransferFunction(cosmoPk_t          pk,
                                 cosmoTF_t          transferFunctionType,
                                 const cosmoModel_t model);


/*--- Local structures and typedefs -------------------------------------*/


/*--- Implementations of exported functios ------------------------------*/
extern cosmoPk_t
cosmoPk_newFromFile(const char *fname)
{
	cosmoPk_t pk;
	FILE      *f;

	assert(fname != NULL);

	pk = local_new();
	f  = xfopen(fname, "r");
	if (fscanf(f, "# %" SCNu32 " %lf %lf \n",
	           &(pk->numPoints),
	           &(pk->slopeBeforeKmin),
	           &(pk->slopeBeyondKmax)) != 3) {
		diediedie(EXIT_FAILURE);
	}
	local_getMem(pk, pk->numPoints);
	for (uint32_t i = 0; i < pk->numPoints; i++) {
		if (fscanf(f, " %lf %lf \n", pk->k + i, pk->P + i) != 2) {
			diediedie(EXIT_FAILURE);
		}
	}
	xfclose(&f);
	local_doInterpolation(pk);

	return pk;
}

extern cosmoPk_t
cosmoPk_newFromIni(parse_ini_t ini, const char *sectionName)
{
	cosmoPk_t pk;
	bool      hasFName;
	char      *fileNamePk = NULL;

	hasFName = parse_ini_get_string(ini, "powerSpectrumFileName",
	                                sectionName, &fileNamePk);
	if (hasFName) {
		pk = cosmoPk_newFromFile(fileNamePk);
		xfree(fileNamePk);
	} else {
		pk = local_constructPkFromModel(ini, sectionName);
	}

	return pk;
}

extern cosmoPk_t
cosmoPk_newFromModel(const cosmoModel_t model,
                     double             kmin,
                     double             kmax,
                     uint32_t           numPoints,
                     cosmoTF_t          transferFunctionType)
{
	cosmoPk_t pk;

	pk                  = local_new();
	pk->numPoints       = numPoints;
	pk->slopeBeforeKmin = cosmoModel_getNs(model);
	pk->slopeBeyondKmax = -3;
	local_getMem(pk, pk->numPoints);
	local_initKs(pk, kmin, kmax);

	local_calcPkFromTransferFunction(pk, transferFunctionType, model);
	local_doInterpolation(pk);

	return pk;
}

extern cosmoPk_t
cosmoPk_newFromArrays(uint32_t     numPoints,
                      const double *k,
                      const double *P,
                      double       slopeBeforeKmin,
                      double       slopeBeyondKmax)
{
	cosmoPk_t pk;

	assert(numPoints > LOCAL_MINPOINTS);
	assert(k != NULL && P != NULL);

	pk = local_new();
	local_getMem(pk, numPoints);
	memcpy(pk->k, k, sizeof(double) * numPoints);
	memcpy(pk->P, P, sizeof(double) * numPoints);
	pk->slopeBeforeKmin = slopeBeforeKmin;
	pk->slopeBeyondKmax = slopeBeyondKmax;
	local_doInterpolation(pk);

	return pk;
}

extern void
cosmoPk_del(cosmoPk_t *pk)
{
	assert(pk != NULL && *pk != NULL);

	if ((*pk)->k != NULL)
		xfree((*pk)->k);
	if ((*pk)->acc != NULL)
		gsl_interp_accel_free((*pk)->acc);
	if ((*pk)->spline != NULL)
		gsl_spline_free((*pk)->spline);
	xfree(*pk);
	*pk = NULL;
}

extern double
cosmoPk_getKminSecure(const cosmoPk_t pk)
{
	assert(pk != NULL);

	return pk->k[LOCAL_IGNOREPOINTS];
}

extern double
cosmoPk_getKmaxSecure(const cosmoPk_t pk)
{
	assert(pk != NULL);

	return pk->k[pk->numPoints - 1 - LOCAL_IGNOREPOINTS];
}

extern void
cosmoPk_dumpToFile(cosmoPk_t pk, const char *fname, uint32_t numSubSample)
{
	FILE   *f;
	assert(pk != NULL && fname != NULL);
	double k, P;

	numSubSample = (numSubSample == 0) ? 1 : numSubSample;

	f            = xfopen(fname, "w");
	for (uint32_t i = 0; i < pk->numPoints - 1; i++) {
		for (uint32_t j = 0; j < numSubSample; j++) {
			k = pk->k[i] + j * (pk->k[i + 1] - pk->k[i]) / numSubSample;
			P = cosmoPk_eval(pk, k);
			fprintf(f, "%15.12g\t%15.12g\n", k, P);
		}
	}
	xfclose(&f);
}

extern double
cosmoPk_eval(cosmoPk_t pk, double k)
{
	assert(pk != NULL);
	assert(isgreater(k, 0.0));

	if (isless(k, pk->k[0]))
		return pk->P[0] * pow(k / (pk->k[0]), pk->slopeBeforeKmin);

	if (isgreater(k, pk->k[pk->numPoints - 1]))
		return pk->P[0] * pow(k / (pk->k[pk->numPoints - 1]),
		                      pk->slopeBeyondKmax);

	return gsl_spline_eval(pk->spline, k, pk->acc);
}

extern double
cosmoPk_evalGSL(double k, void *param)
{
	assert(param != NULL);
	assert(isgreater(k, 0.0));
	return cosmoPk_eval((cosmoPk_t)param, k);
}

extern double
cosmoPk_calcMomentFiltered(cosmoPk_t pk,
                           uint32_t moment,
                           double (*windowFunc
                                   )(double, void *),
                           void *paramWindowFunc,
                           double kmin,
                           double kmax,
                           double *error)
{
	double                      sigmaSqr;
	double                      exponentK = (2. * moment + 2.);
	cosmoFunc_product3_struct_t param;
	gsl_integration_workspace   *w;
	gsl_function                F;

	assert(pk != NULL && error != NULL && windowFunc != NULL);

	param.f1      = &cosmoPk_evalGSL;
	param.paramF1 = (void *)pk;
	param.f2      = windowFunc;
	param.paramF2 = paramWindowFunc;
	param.f3      = &cosmoFunc_xPowerY;
	param.paramF3 = (void *)&exponentK;
	F.function    = &cosmoFunc_product3;
	F.params      = (void *)&param;

	w             = gsl_integration_workspace_alloc(LOCAL_LIMIT);

	gsl_integration_qag(&F, kmin, kmax, 0, LOCAL_EPSREL, LOCAL_LIMIT,
	                    GSL_INTEG_GAUSS61, w, &sigmaSqr, error);

	gsl_integration_workspace_free(w);

	return 1. / (2 * POW2(M_PI)) * sigmaSqr;
}

extern double
cosmoPk_calcSigma8(cosmoPk_t pk,
                   double    kmin,
                   double    kmax,
                   double    *error)
{
	double scale = 8.0;
	double sigma8Sqr;

	assert(pk != NULL && isgreater(kmax, kmin) && isgreater(kmin, 0.0)
	       && error != NULL);

	sigma8Sqr = cosmoPk_calcMomentFiltered(pk, UINT32_C(0),
	                                       &cosmoFunc_tophatSqr,
	                                       &scale, kmin, kmax, error);
	return sqrt(sigma8Sqr);
}

extern void
cosmoPk_scale(cosmoPk_t pk, double factor)
{
	assert(pk != NULL);
	assert(isgreater(factor, 0.0));

	for (uint32_t i = 0; i < pk->numPoints; i++)
		pk->P[i] *= factor;
	local_doInterpolation(pk);
}

extern double
cosmoPk_forceSigma8(cosmoPk_t pk,
                    double    sigma8,
                    double    kmin,
                    double    kmax,
                    double    *error)
{
	double sigma8Actual;
	double sigma8First;
	int    numIter    = 0;
	int    numIterMax = LOCAL_MAX_FORCESIGMA8_ITERATIONS;

	assert(pk != NULL);
	assert(isgreater(sigma8, 0.0));
	assert(isgreater(kmin, 0.0));
	assert(isgreater(kmax, kmin));
	assert(error != NULL);

	sigma8Actual = cosmoPk_calcSigma8(pk, kmin, kmax, error);
	sigma8First  = sigma8Actual;
	do {
		cosmoPk_scale(pk, POW2(sigma8 / sigma8Actual));
		sigma8Actual = cosmoPk_calcSigma8(pk, kmin, kmax, error);
		*error       = fabs(1. - sigma8 / sigma8Actual);
		numIter++;
	} while (numIter < numIterMax && isgreater(*error, 1e-10));
	if (numIter >= numIterMax)
		fprintf(stderr, "Exhausted iterations in %s: error %15.13e\n",
		        __func__, *error);

	return POW2(sigma8 / sigma8First);
}

extern double
cosmoPk_forceAmplitude(cosmoPk_t pk, double amplitudeAtK, double k)
{
	double amplitudeActual;

	assert(pk != NULL);
	assert(isfinite(amplitudeAtK) && amplitudeAtK > 0.0);
	assert(isfinite(k) && k > 0.0);

	amplitudeActual = cosmoPk_eval(pk, k);
	cosmoPk_scale(pk, amplitudeAtK / amplitudeActual);
	amplitudeActual = cosmoPk_eval(pk, k);

	return fabs(1. - amplitudeAtK / amplitudeActual);
}

extern void
cosmoPk_findKWindowForSigma8(cosmoPk_t pk, double *kmin, double *kmax)
{
	double error;
	double sigma8;
	double sigma8Old;
	double pkKmin = pk->k[0];
	double pkKmax = pk->k[pk->numPoints - 1];

	sigma8 = cosmoPk_calcSigma8(pk, *kmin, *kmax, &error);
	do {
		sigma8Old = sigma8;
		*kmin    *= 0.9;
		if (*kmin < pkKmin)
			*kmin = pkKmin;
		*kmax *= 1.1;
		if (*kmax > pkKmax)
			*kmax = pkKmax;
		sigma8 = cosmoPk_calcSigma8(pk, *kmin, *kmax, &error);
	} while (isgreater(fabs(1. - sigma8 / sigma8Old), 1e-6)
	         && (*kmin > pkKmin || *kmax < pkKmax));
}

/*--- Implementations of local functions --------------------------------*/
static cosmoPk_t
local_new(void)
{
	cosmoPk_t pk = xmalloc(sizeof(struct cosmoPk_struct));

	pk->numPoints       = UINT32_C(0);
	pk->k               = NULL;
	pk->P               = NULL;
	pk->slopeBeyondKmax = 1e10;
	pk->slopeBeforeKmin = 1e10;
	pk->acc             = NULL;
	pk->spline          = NULL;

	return pk;
}

static void
local_getMem(cosmoPk_t pk, uint32_t numPoints)
{
	if (numPoints < LOCAL_MINPOINTS) {
		fprintf(stderr, "P(k) needs to have at least %i points!\n",
		        LOCAL_MINPOINTS);
		diediedie(EXIT_FAILURE);
	}
	if (pk->k != NULL)
		xfree(pk->k);
	pk->k         = xmalloc(sizeof(double) * numPoints * 2);
	pk->P         = pk->k + numPoints;
	pk->numPoints = numPoints;
}

static void
local_doInterpolation(cosmoPk_t pk)
{
	if (pk->acc != NULL)
		gsl_interp_accel_free(pk->acc);
	if (pk->spline != NULL)
		gsl_spline_free(pk->spline);

	pk->acc    = gsl_interp_accel_alloc();
	pk->spline = gsl_spline_alloc(gsl_interp_cspline,
	                              (int)(pk->numPoints));
	gsl_spline_init(pk->spline, pk->k, pk->P, (int)(pk->numPoints));
}

static cosmoPk_t
local_constructPkFromModel(parse_ini_t ini, const char *sectionName)
{
	cosmoPk_t    pk;
	cosmoModel_t model = cosmoModel_newFromIni(ini, sectionName);
	double       kmin, kmax;
	uint32_t     numPoints;

	getFromIni(&kmin, parse_ini_get_double, ini,
	           "powerSpectrumKmin", sectionName);
	getFromIni(&kmax, parse_ini_get_double, ini,
	           "powerSpectrumKmax", sectionName);
	getFromIni(&numPoints, parse_ini_get_uint32, ini,
	           "powerSpectrumNumPoints", sectionName);

	pk = cosmoPk_newFromModel(model, kmin, kmax, numPoints,
	                          cosmoTF_getTypeFromIni(ini, sectionName));
	cosmoModel_del(&model);

	return pk;
}

static void
local_initKs(cosmoPk_t pk, double kmin, double kmax)
{
	pk->k[0]                 = kmin;
	pk->k[pk->numPoints - 1] = kmax;

	if (pk->numPoints > 1) {
		double logKmin = log(kmin);
		double logdk   = (log(kmax) - logKmin)
		                 / ((double)(pk->numPoints - 1));
		for (uint32_t i = 1; i < pk->numPoints - 1; i++)
			pk->k[i] = exp(logKmin + logdk * i);
	}
}

static void
local_calcPkFromTransferFunction(cosmoPk_t          pk,
                                 cosmoTF_t          transferFunctionType,
                                 const cosmoModel_t model)
{
	switch (transferFunctionType) {
	case COSMOTF_TYPE_SCALEFREE:
		cosmoTF_scaleFree(pk->numPoints, pk->k, pk->P);
		break;
	case COSMOTF_TYPE_ANATOLY2000:
	case COSMOTF_TYPE_EISENSTEINHU1998:
	default:
		cosmoTF_eisensteinHu1998(cosmoModel_getOmegaMatter0(model),
		                         cosmoModel_getOmegaBaryon0(model),
		                         cosmoModel_getSmallH(model),
		                         cosmoModel_getTempCMB(model),
		                         pk->numPoints, pk->k, pk->P);
		break;
	}

	for (uint32_t i = 0; i < pk->numPoints; i++) {
		pk->P[i] *= pk->P[i] * pow(pk->k[i], cosmoModel_getNs(model));
	}
}
