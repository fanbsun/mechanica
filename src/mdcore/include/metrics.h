/*
 * metrics.h
 *
 *  Created on: Nov 18, 2020
 *      Author: andy
 */

#ifndef SRC_MDCORE_INCLUDE_METRICS_H_
#define SRC_MDCORE_INCLUDE_METRICS_H_

#include "platform.h"
#include "mdcore_config.h"
#include "Magnum/Magnum.h"
#include "Magnum/Math/Vector3.h"
#include <set>

/**
 * @origin [in] origin of the sphere where we will comptute
 * the local pressure tensor.
 * @radius [in] include all partices a given radius in calculation. 
 * @typeIds [in] vector of type ids to indlude in calculation,
 * if empty, includes all particles.
 * @tensor [out] result vector, writes a 3x3 matrix in a row-major in the given
 * location.
 *
 * If periodoc, we don't include the periodic image cells, because we only
 * calculate the forces within the simulation volume. 
 */
CAPI_FUNC(HRESULT) MxCalculatePressure(FPTYPE *origin,
                                       FPTYPE radius,
                                       const std::set<short int> &typeIds,
                                       FPTYPE *tensor);

/**
 * calculate the pressure tensor for a specific list of particles.
 * currently uses center of mass as origin, may change in the
 * future with different flags.
 *
 * flags currently ignored.
 */
CAPI_FUNC(HRESULT) MxParticles_Pressure(int32_t *parts,
                                        uint16_t nr_parts,
                                        uint32_t flags,
                                        FPTYPE *tensor);

/**
 * @param result: pointer to float to store result.
 */
HRESULT MxParticles_RadiusOfGyration(int32_t *parts, uint16_t nr_parts, float* result);

/**
 * @param result: pointer to float[3] to store result
 */
HRESULT MxParticles_CenterOfMass(int32_t *parts, uint16_t nr_parts, float* result);

/**
 * @param result: pointer to float[3] to store result.
 */
HRESULT MxParticles_CenterOfGeometry(int32_t *parts, uint16_t nr_parts, float* result);

/**
 * @param result: pointer to float[9] to store result.
 */
HRESULT MxParticles_MomentOfInertia(int32_t *parts, uint16_t nr_parts, float* result);

/**
 * converts cartesian to spherical in global coord space.
 * createsa a numpy array.
 */
PyObject* MPyCartesianToSpherical(const Magnum::Vector3& postion,
                                             const Magnum::Vector3& origin);


/**
 * converts cartesian to spherical, writes spherical
 * coords in to result array.
 */
Magnum::Vector3 MxCartesianToSpherical(const Magnum::Vector3& postion,
                                          const Magnum::Vector3& origin);





#endif /* SRC_MDCORE_INCLUDE_METRICS_H_ */
