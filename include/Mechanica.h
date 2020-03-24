/*
 * caiman.h
 *
 *  Created on: Jun 30, 2015
 *      Author: andy
 */

//#ifdef _INCLUDED_MECHANICA_PRIVATE_H_
//#error "Can not incude internal private header file in or before a public header"
//#endif

#ifndef _INCLUDED_MECHANICA_H_
#define _INCLUDED_MECHANICA_H_

typedef double MxReal;

#include <carbon.h>
#include <mx_config.h>


#if defined(MX_APPLE)
    #include "Magnum/Platform/WindowlessCglApplication.h"
#elif defined(MX_LINUX)
    #include "Magnum/Platform/WindowlessEglApplication.h"
#elif defined(MX_WINDOWS)
#include "Magnum/Platform/WindowlessWglApplication.h"
#else
#error no windowless application available on this platform
#endif

#include <mx_object.hpp>
#include <mx_particle.h>
#include <mx_module.h>
#include <mx_number.h>
#include <mx_runtime.h>
#include <mx_import.h>
#include <mx_tupleobject.h>
#include <mx_abstract.h>
#include <mx_float.h>
#include <mx_int.h>
#include <mx_callable.h>
#include <mx_composite_particle.h>
#include <mx_lattice.h>
#include <mx_function.h>
#include <mx_eval.h>
#include <mx_mesh.h>
#include <mx_particles.h>
#include <mx_simulator.h>
#include <mx_ui.h>



#endif /* _INCLUDED_MECHANICA_H_ */
