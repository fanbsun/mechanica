/*
 * MxSimulator.cpp
 *
 *  Created on: Feb 1, 2017
 *      Author: andy
 */

#include <MxSimulator.h>
#include <rendering/MxUI.h>
#include <rendering/MxTestView.h>

#include <Magnum/GL/Context.h>

#include "rendering/MxApplication.h"
#include "rendering/MxUniverseRenderer.h"
#include <rendering/MxGlfwApplication.h>
#include <rendering/MxWindowlessApplication.h>
#include <map>
#include <sstream>
#include <MxUniverse.h>
#include <MxConvert.hpp>

#include <MxPy.h>

// mdcore errs.h
#include <errs.h>

#include <thread>

static std::vector<Vector3> fillCubeRandom(const Vector3 &corner1, const Vector3 &corner2, int nParticles);

/* What to do if ENGINE_FLAGS was not defined? */
#ifndef ENGINE_FLAGS
#define ENGINE_FLAGS engine_flag_none
#endif
#ifndef CPU_TPS
#define CPU_TPS 2.67e+9
#endif


#define SIM_TRY() \
    try {\
        if(_Engine.flags == 0) { \
            std::string err = MX_FUNCTION; \
            err += "universe not initialized"; \
            throw std::domain_error(err.c_str()); \
        }

#define SIM_CHECK(hr) \
    if(SUCCEEDED(hr)) { Py_RETURN_NONE; } \
    else {return NULL;}

#define SIM_FINALLY(retval) \
    } \
    catch(const std::exception &e) { \
        C_EXP(e); return retval; \
    }

static MxSimulator* Simulator = NULL;

static void simulator_interactive_run();

static PyObject *ipythonInputHook(PyObject *self,
                           PyObject *const *args,
                                  Py_ssize_t nargs);

PyObject *MxSystem_ContextHasCurrent(PyObject *self) {
    
    try {
        std::thread::id id = std::this_thread::get_id();
        std::cout << MX_FUNCTION << ", thread id: " << id << std::endl;
        
        MxSimulator *sim = MxSimulator::Get();
        
        return mx::cast(sim->app->contextHasCurrent());
        
    }
    catch(const std::exception &e) {
        C_RETURN_EXP(e);
    }
}

PyObject *MxSystem_ContextMakeCurrent(PyObject *self) {
    try {
        std::thread::id id = std::this_thread::get_id();
        std::cout << MX_FUNCTION << ", thread id: " << id << std::endl;
        
        MxSimulator *sim = MxSimulator::Get();
        sim->app->contextMakeCurrent();
        
        Py_RETURN_NONE;
    }
    catch(const std::exception &e) {
        C_RETURN_EXP(e);
    }
}

PyObject *MxSystem_ContextRelease(PyObject *self) {
    try {
        std::thread::id id = std::this_thread::get_id();
        std::cout << MX_FUNCTION << ", thread id: " << id << std::endl;
        
        MxSimulator *sim = MxSimulator::Get();
        sim->app->contextRelease();
        
        Py_RETURN_NONE;
    }
    catch(const std::exception &e) {
        C_RETURN_EXP(e);
    }
}

PyObject *MxSystem_CameraRotate(PyObject *self, PyObject *args, PyObject *kwargs) {
    try {
        MxSimulator *sim = MxSimulator::Get();
        
        MxUniverseRenderer *renderer = sim->app->getRenderer();
        
        Magnum::Mechanica::ArcBall *ab = renderer->_arcball;
        
        Magnum::Vector3 eulerAngles = mx::arg<Magnum::Vector3>("euler_angles", 0, args, kwargs);
        
        ab->rotate(eulerAngles);
        
        Py_RETURN_NONE;
    }
    catch(const std::exception &e) {
        C_RETURN_EXP(e);
    }
}

MxSimulator::Config::Config():
            _title{"Mechanica Application"},
            _size{800, 600},
            _dpiScalingPolicy{DpiScalingPolicy::Default},
            queues{4},
           _windowless{ false }
{
    _windowFlags = MxSimulator::WindowFlags::Resizable |
                   MxSimulator::WindowFlags::Focused   |
                   MxSimulator::WindowFlags::Hidden;  // make the window initially hidden
}



MxSimulator::GLConfig::GLConfig():
_colorBufferSize{8, 8, 8, 0}, _depthBufferSize{24}, _stencilBufferSize{0},
_sampleCount{0}, _version{GL::Version::None},
#ifndef MAGNUM_TARGET_GLES
_flags{Flag::ForwardCompatible},
#else
_flags{},
#endif
_srgbCapable{false} {}

MxSimulator::GLConfig::~GLConfig() = default;




#define SIMULATOR_CHECK()  if (!Simulator) { return mx_error(E_INVALIDARG, "Simulator is not initialized"); }

#define PY_CHECK(hr) {if(!SUCCEEDED(hr)) { throw py::error_already_set();}}

#define PYSIMULATOR_CHECK() { \
    if(!Simulator) { \
        throw std::domain_error(std::string("Simulator Error in ") + MX_FUNCTION + ": Simulator not initialized"); \
    } \
}

/**
 * Make a Arguments struct from a python string list,
 * Agh!!! Magnum has different args for different app types,
 * so this needs to be a damned template.
 */
template<typename T>
struct ArgumentsWrapper  {

    ArgumentsWrapper(PyObject *args) {

        for(int i = 0; i < PyList_Size(args); ++i) {
            PyObject *o = PyList_GetItem(args, i);
            strings.push_back(mx::cast<std::string>(o));
            cstrings.push_back(strings.back().c_str());

            std::cout << "args: " << cstrings.back() << std::endl;
        }

        // stupid thing is a int reference, keep an ivar around for it
        // to point to.
        argsSeriouslyTakesAFuckingIntReference = cstrings.size();
        char** fuckingConstBullshit = const_cast<char**>(cstrings.data());

        pArgs = new T(argsSeriouslyTakesAFuckingIntReference, fuckingConstBullshit);
    }

    ~ArgumentsWrapper() {
        delete pArgs;
    }


    // OMG this is a horrible design.
    // how I hate C++
    std::vector<std::string> strings;
    std::vector<const char*> cstrings;
    T *pArgs = NULL;
    int argsSeriouslyTakesAFuckingIntReference;
};


static void parse_kwargs(PyObject *kwargs, MxSimulator::Config &conf) {

    PyObject *o;

    if((o = PyDict_GetItemString(kwargs, "dim"))) {
        conf.universeConfig.dim = mx::cast<Magnum::Vector3>(o);
    }

    if((o = PyDict_GetItemString(kwargs, "cutoff"))) {
        conf.universeConfig.cutoff = mx::cast<double>(o);
    }

    if((o = PyDict_GetItemString(kwargs, "cells"))) {
        conf.universeConfig.spaceGridSize = mx::cast<Vector3i>(o);
    }

    if((o = PyDict_GetItemString(kwargs, "threads"))) {
        conf.universeConfig.threads = mx::cast<unsigned>(o);
    }

    if((o = PyDict_GetItemString(kwargs, "integrator"))) {
        int kind = mx::cast<int>(o);
        switch (kind) {
            case FORWARD_EULER:
            case RUNGE_KUTTA_4:
                conf.universeConfig.integrator = (EngineIntegrator)kind;
                break;
            default: {
                std::string msg = "invalid integrator kind: ";
                msg += std::to_string(kind);
                throw std::logic_error(msg);
            }
        }
    }

    if((o = PyDict_GetItemString(kwargs, "dt"))) {
        conf.universeConfig.dt = mx::cast<double>(o);
    }

    if((o = PyDict_GetItemString(kwargs, "bc"))) {
        conf.universeConfig.setBoundaryConditions(o);
    }

    if((o = PyDict_GetItemString(kwargs, "boundary_conditions"))) {
        conf.universeConfig.setBoundaryConditions(o);
    }

    if((o = PyDict_GetItemString(kwargs, "max_distance"))) {
        conf.universeConfig.max_distance = mx::cast<double>(o);
    }

    if((o = PyDict_GetItemString(kwargs, "windowless"))) {
        conf.setWindowless(mx::cast<bool>(o));
        if(conf.windowless()) {
            conf.setWindowSize({1024,1024});
        }
    }

    if((o = PyDict_GetItemString(kwargs, "window_size"))) {
        Magnum::Vector2i windowSize = mx::cast<Magnum::Vector2i>(o);
        conf.setWindowSize(windowSize);
    }

    if((o = PyDict_GetItemString(kwargs, "perfcounters"))) {
        conf.universeConfig.timers_mask = mx::cast<uint32_t>(o);
    }

    if((o = PyDict_GetItemString(kwargs, "perfcounter_period"))) {
        conf.universeConfig.timer_output_period = mx::cast<int>(o);
    }
}

static PyObject *simulator_init(PyObject *self, PyObject *args, PyObject *kwargs);

static std::string gl_info(const Magnum::Utility::Arguments &args);

static PyObject *not_initialized_error();

// (5) Initializer list constructor
const std::map<std::string, int> configItemMap {
    {"none", MXSIMULATOR_NONE},
    {"windowless", MXSIMULATOR_WINDOWLESS},
    {"glfw", MXSIMULATOR_GLFW}
};

static int init(PyObject *self, PyObject *args, PyObject *kwds)
{
    std::cout << MX_FUNCTION << std::endl;

    MxSimulator *s = new (self) MxSimulator();
    return 0;
}

static PyObject *simulator_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return NULL;
}

#define MX_CLASS METH_CLASS | METH_VARARGS | METH_KEYWORDS


CAPI_FUNC(MxSimulator*) MxSimulator_New(PyObject *_args, PyObject *_kw_args)
{
    return NULL;
}

CAPI_FUNC(MxSimulator*) MxSimulator_Get()
{
    return Simulator;
}


PyObject *not_initialized_error() {
    PyErr_SetString((PyObject*)&MxSimulator_Type, "simulator not initialized");
    Py_RETURN_NONE;
}

CAPI_FUNC(HRESULT) MxSimulator_PollEvents()
{
    SIMULATOR_CHECK();
    return Simulator->app->pollEvents();
}

CAPI_FUNC(HRESULT) MxSimulator_WaitEvents()
{
    SIMULATOR_CHECK();
    return Simulator->app->waitEvents();
}

CAPI_FUNC(HRESULT) MxSimulator_WaitEventsTimeout(double timeout)
{
    SIMULATOR_CHECK();
    return Simulator->app->waitEventsTimeout(timeout);
}

CAPI_FUNC(HRESULT) MxSimulator_PostEmptyEvent()
{
    SIMULATOR_CHECK();
    return Simulator->app->postEmptyEvent();
}

HRESULT MxSimulator_SwapInterval(int si)
{
    SIMULATOR_CHECK();
    return Simulator->app->setSwapInterval(si);
}


int universe_init (const MxUniverseConfig &conf ) {

    Magnum::Vector3 tmp = conf.dim - conf.origin;
    Magnum::Vector3d length{tmp[0], tmp[1], tmp[2]};
    Magnum::Vector3i cells = conf.spaceGridSize;

    Magnum::Vector3d spaceGridSize{(float)cells[0],
                                   (float)cells[1],
                                   (float)cells[2]};

    double   cutoff = conf.cutoff;

    int  nr_runners = conf.threads;

    double _origin[3];
    double _dim[3];
    for(int i = 0; i < 3; ++i) {
        _origin[i] = conf.origin[i];
        _dim[i] = conf.dim[i];
    }


    printf("main: initializing the engine... "); fflush(stdout);
    if ( engine_init( &_Engine , _origin , _dim , cells.data() , cutoff , conf.boundaryConditionsPtr ,
            conf.maxTypes , engine_flag_none ) != 0 ) {
        throw std::runtime_error(errs_getstring(0));
    }

    _Engine.dt = conf.dt;
    _Engine.temperature = conf.temp;
    _Engine.integrator = conf.integrator;

    _Engine.timers_mask = conf.timers_mask;
    _Engine.timer_output_period = conf.timer_output_period;

    if(conf.max_distance >= 0) {
        // max_velocity is in absolute units, convert
        // to scale fraction.

        _Engine.particle_max_dist_fraction = conf.max_distance / _Engine.s.h[0];
    }

    const char* inte = NULL;

    switch(_Engine.integrator) {
    case EngineIntegrator::FORWARD_EULER:
        inte = "Forward Euler";
        break;
    case EngineIntegrator::RUNGE_KUTTA_4:
        inte = "Ruge-Kutta-4";
        break;
    }

    printf("engine integrator: %s \n", inte);
    printf("engine: n_cells: %i, cell width set to %22.16e.\n", _Engine.s.nr_cells, cutoff);
    printf("engine: cell dimensions = [ %i , %i , %i ].\n", _Engine.s.cdim[0] , _Engine.s.cdim[1] , _Engine.s.cdim[2] );
    printf("engine: cell size = [ %e , %e , %e ].\n" , _Engine.s.h[0] , _Engine.s.h[1] , _Engine.s.h[2] );
    printf("engine: cutoff set to %22.16e.\n", cutoff);
    printf("engine: nr tasks: %i.\n",_Engine.s.nr_tasks);
    printf("engine: nr cell pairs: %i.\n",_Engine.s.nr_pairs);


    printf("engine: dt: %22.16e.\n",_Engine.dt);
    printf("engine: max distance fraction: %22.16e.\n",_Engine.particle_max_dist_fraction);

    // start the engine

    if ( engine_start( &_Engine , nr_runners , nr_runners ) != 0 ) {
        throw std::runtime_error(errs_getstring(0));
    }

    fflush(stdout);

    return 0;
}



static std::vector<Vector3> fillCubeRandom(const Vector3 &corner1, const Vector3 &corner2, int nParticles) {
    std::vector<Vector3> result;

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<float> disx(corner1[0], corner2[0]);
    std::uniform_real_distribution<float> disy(corner1[1], corner2[1]);
    std::uniform_real_distribution<float> disz(corner1[2], corner2[2]);

    for(int i = 0; i < nParticles; ++i) {
        result.push_back(Vector3{disx(gen), disy(gen), disz(gen)});

    }

    return result;
}

CAPI_FUNC(HRESULT) MxSimulator_Run(double et)
{
    SIMULATOR_CHECK();

    std::cout << "simulator run(" << et << ")" << std::endl;

    return Simulator->app->run(et);
}

CAPI_FUNC(HRESULT) MxSimulator_InteractiveRun()
{
    std::cout << MX_FUNCTION << ",  start " << std::endl;
    
    SIMULATOR_CHECK();

    MxUniverse_SetFlag(MX_RUNNING, true);


    std::fprintf(stderr, "checking for ipython \n");
    if (Mx_IsIpython()) {

        if (!MxUniverse_Flag(MxUniverse_Flags::MX_IPYTHON_MSGLOOP)) {
            // ipython message loop, this exits right away
            simulator_interactive_run();
        }

        std::fprintf(stderr, "in ipython, calling interactive \n");

        Simulator->app->show();
        
        std::cout << MX_FUNCTION << ",  finished" << std::endl;

        return S_OK;
    }
    else {
        std::fprintf(stderr, "not ipython, returning MxSimulator_Run \n");
        return MxSimulator_Run(-1);
    }
}

static PyObject *simulator_init(PyObject *self, PyObject *args, PyObject *kwargs) {

    std::thread::id id = std::this_thread::get_id();
    std::cout << MX_FUNCTION << ", thread id: " << id << std::endl;

    try {

        if(Simulator) {
            throw std::domain_error( "Error, Simulator is already initialized" );
        }

        MxSimulator *sim = new MxSimulator();

        // get the argv,
        PyObject * argv = NULL;
        if(kwargs == NULL || (argv = PyDict_GetItemString(kwargs, "argv")) == NULL) {
            PyObject *sys_name = mx::cast(std::string("sys"));
            PyObject *sys = PyImport_Import(sys_name);
            argv = PyObject_GetAttrString(sys, "argv");
            
            Py_DECREF(sys_name);
            Py_DECREF(sys);
            
            if(!argv) {
                throw std::logic_error("could not get argv from sys module");
            }
        }

        MxSimulator::Config conf;
        MxSimulator::GLConfig glConf;
        
        if(PyList_Size(argv) > 0) {
            std::string name = mx::cast<std::string>(PyList_GetItem(argv, 0));
            Universe.name = name;
            conf.setTitle(name);
        }

        if(kwargs && PyDict_Size(kwargs) > 0) {
            parse_kwargs(kwargs, conf);
        }


        // init the engine first
        /* Initialize scene particles */
        universe_init(conf.universeConfig);

        if(conf.windowless()) {
            ArgumentsWrapper<MxWindowlessApplication::Arguments> margs(argv);

            std::cout << "creating Windowless app" << std::endl;

            MxWindowlessApplication *windowlessApp = new MxWindowlessApplication(*margs.pArgs);

            if(FAILED(windowlessApp->createContext(conf))) {
                delete windowlessApp;

                throw std::domain_error("could not create windowless gl context");
            }
            else {
                sim->app = windowlessApp;
            }
        }
        else {
            ArgumentsWrapper<MxGlfwApplication::Arguments> margs(argv);

            std::cout << "creating GLFW app" << std::endl;

            MxGlfwApplication *glfwApp = new MxGlfwApplication(*margs.pArgs);

            glfwApp->createContext(conf);

            sim->app = glfwApp;
        }

        std::cout << MX_FUNCTION << std::endl;

        Simulator = sim;
        Py_RETURN_NONE;
    }
    catch(const std::exception &e) {
        C_EXP(e); return NULL;
    }
}


static PyObject *ipythonInputHook(PyObject *self,
                                  PyObject *const *args,
                                  Py_ssize_t nargs) {
    
    
    //std::cout << MX_FUNCTION << std::endl;
    SIM_TRY();
    
    if(nargs < 1) {
        throw std::logic_error("argument count to mechanica ipython input hook is 0");
    }
    
    PyObject *context = args[0];
    if(context == NULL) {
        throw std::logic_error("mechanica ipython input hook context argument is NULL");
    }
    
    PyObject *input_is_ready = PyObject_GetAttrString(context, "input_is_ready");
    if(input_is_ready == NULL) {
        throw std::logic_error("mechanica ipython input hook context has no \"input_is_ready\" attribute");
    }
    
    PyObject *input_args = PyTuple_New(0);
    
    auto get_ready = [input_is_ready, input_args]() -> bool {
        PyObject *ready = PyObject_Call(input_is_ready, input_args, NULL);
        if(!ready) {
            PyObject* err = PyErr_Occurred();
            std::string str = "error calling input_is_ready";
            str += carbon::str(err);
            throw std::logic_error(str);
        }
        
        bool bready = mx::cast<bool>(ready);
        Py_DECREF(ready);
        return bready;
    };
    
    Py_XDECREF(input_args);
    
    while(!get_ready()) {
        Simulator->app->mainLoopIteration(0.001);
    }
    
    //std::cout << MX_FUNCTION << ", all done" << std::endl;
    
    Py_RETURN_NONE;
    
    SIM_FINALLY(NULL);
}


static void simulator_interactive_run() {
    std::cout << "entering " << MX_FUNCTION << std::endl;

    if (MxUniverse_Flag(MxUniverse_Flags::MX_POLLING_MSGLOOP)) {
        return;
    }

    // interactive run only works in terminal ipytythn.
    PyObject *ipy = CIPython_Get();
    const char* ipyname = ipy ? ipy->ob_type->tp_name : "NULL";
    std::cout << "ipy type: " << ipyname << std::endl;

    if(ipy && strcmp("TerminalInteractiveShell", ipy->ob_type->tp_name) == 0) {

        std::cerr << "calling python interactive loop" << std::endl;
        
        PyObject *mx_str = mx::cast(std::string("mechanica"));

        // Try to import ipython

        /**
         *        """
            Registers the mechanica input hook with the ipython pt_inputhooks
            class.

            The ipython TerminalInteractiveShell.enable_gui('name') method
            looks in the registered input hooks in pt_inputhooks, and if it
            finds one, it activtes that hook.

            To acrtivate the gui mode, call:

            ip = IPython.get_ipython()
            ip.
            """
            import IPython.terminal.pt_inputhooks as pt_inputhooks
            pt_inputhooks.register("mechanica", inputhook)
         *
         */

        PyObject *pt_inputhooks = PyImport_ImportString("IPython.terminal.pt_inputhooks");
        
        std::cout << "pt_inputhooks: " << carbon::str(pt_inputhooks) << std::endl;
        
        PyObject *reg = PyObject_GetAttrString(pt_inputhooks, "register");
        
        std::cout << "reg: " << carbon::str(reg) << std::endl;
        
        PyObject *ih = PyObject_GetAttrString((PyObject*)&MxSimulator_Type, "_input_hook");
        
        std::cout << "ih: " << carbon::str(ih) << std::endl;

        //py::cpp_function ih(ipythonInputHook);
        
        //reg("mechanica", ih);
        
        std::cout << "calling reg...." << std::endl;
        
        PyObject *args = PyTuple_Pack(2, mx_str, ih);
        PyObject *reg_result = PyObject_Call(reg, args, NULL);
        Py_XDECREF(args);
        
        if(reg_result == NULL) {
            throw std::logic_error("error calling IPython.terminal.pt_inputhooks.register()");
        }
        
        Py_XDECREF(reg_result);

        // import IPython
        // ip = IPython.get_ipython()
        PyObject *ipython = PyImport_ImportString("IPython");
        std::cout << "ipython: " << carbon::str(ipython) << std::endl;
        
        PyObject *get_ipython = PyObject_GetAttrString(ipython, "get_ipython");
        std::cout << "get_ipython: " << carbon::str(get_ipython) << std::endl;
        
        args = PyTuple_New(0);
        PyObject *ip = PyObject_Call(get_ipython, args, NULL);
        Py_XDECREF(args);
        
        if(ip == NULL) {
            throw std::logic_error("error calling IPython.get_ipython()");
        }
        
        PyObject *enable_gui = PyObject_GetAttrString(ip, "enable_gui");
        
        if(enable_gui == NULL) {
            throw std::logic_error("error calling ipython has no enable_gui attribute");
        }
        
        args = PyTuple_Pack(1, mx_str);
        PyObject *enable_gui_result = PyObject_Call(enable_gui, args, NULL);
        Py_XDECREF(args);
        Py_XDECREF(mx_str);
        
        if(enable_gui_result == NULL) {
            throw std::logic_error("error calling ipython.enable_gui(\"mechanica\")");
        }
        
        Py_XDECREF(enable_gui_result);

        MxUniverse_SetFlag(MxUniverse_Flags::MX_IPYTHON_MSGLOOP, true);

        // show the app
        Simulator->app->show();
    }
    else {
        // not in ipython, so run regular run.
        MxSimulator_Run(-1);
        return;
    }

    Py_XDECREF(ipy);
    std::cerr << "leaving " << MX_FUNCTION << std::endl;
}


CAPI_FUNC(HRESULT) MxSimulator_Show()
{
    SIMULATOR_CHECK();

    std::fprintf(stderr, "checking for ipython \n");
    if (Mx_IsIpython()) {

        if (!MxUniverse_Flag(MxUniverse_Flags::MX_IPYTHON_MSGLOOP)) {
            // ipython message loop, this exits right away
            simulator_interactive_run();
        }

        std::fprintf(stderr, "in ipython, calling interactive \n");

        Simulator->app->show();
        
        std::cout << MX_FUNCTION << ", Simulator->app->show() all done" << std::endl;

        return S_OK;
    }
    else {
        std::fprintf(stderr, "not ipython, returning Simulator->app->show() \n");
        return Simulator->app->show();
    }
}

CAPI_FUNC(HRESULT) MxSimulator_Redraw()
{
    SIMULATOR_CHECK();
    return Simulator->app->redraw();
}

CAPI_FUNC(HRESULT) MxSimulator_InitConfig(const MxSimulator::Config &conf, const MxSimulator::GLConfig &glConf)
{
    if(Simulator) {
        return mx_error(E_FAIL, "simulator already initialized");
    }

    MxSimulator *sim = new MxSimulator();

    // init the engine first
    /* Initialize scene particles */
    universe_init(conf.universeConfig);


    if(conf.windowless()) {

        /*



        MxWindowlessApplication::Configuration windowlessConf;

        MxWindowlessApplication *windowlessApp = new MxWindowlessApplication(*margs.pArgs);

        if(!windowlessApp->tryCreateContext(conf)) {
            delete windowlessApp;

            throw std::domain_error("could not create windowless gl context");
        }
        else {
            sim->app = windowlessApp;
        }
        */
    }
    else {

        std::cout << "creating GLFW app" << std::endl;

        int argc = conf.argc;

        MxGlfwApplication::Arguments args{argc, conf.argv};

        MxGlfwApplication *glfwApp = new MxGlfwApplication(args);

        glfwApp->createContext(conf);

        sim->app = glfwApp;
    }

    std::cout << MX_FUNCTION << std::endl;

    Simulator = sim;

    return S_OK;
}

CAPI_FUNC(HRESULT) MxSimulator_Close()
{
    SIMULATOR_CHECK();
    return Simulator->app->close();
}

CAPI_FUNC(HRESULT) MxSimulator_Destroy()
{
    SIMULATOR_CHECK();
    return Simulator->app->destroy();
}


CAPI_FUNC(bool) Mx_IsIpython() {
    PyObject* ipy = CIPython_Get();
    bool result = false;

    if (ipy && strcmp("TerminalInteractiveShell", ipy->ob_type->tp_name) == 0) {
        result = true;
    }

    Py_XDECREF(ipy);
    return result;
}


/**
 * gets the global simulator object, throws exception if fail.
 */
MxSimulator *MxSimulator::Get() {
    if(Simulator) {
        return Simulator;
    }
    throw std::logic_error("Simulator is not initiazed");
}

static PyObject *simulator_poll_events(PyObject *self, PyObject *args, PyObject *kwargs) {
    SIM_TRY();
    SIM_CHECK(MxSimulator_PollEvents());
    SIM_FINALLY(NULL);
}

static PyObject *simulator_wait_events(PyObject *self, PyObject *args, PyObject *kwargs) {
    SIM_TRY();
    
    if(PyTuple_Size(args) == 0) {
        SIM_CHECK(MxSimulator_WaitEvents());
    }
    else {
        double t = mx::arg<double>("timout", 0, args, kwargs);
        SIM_CHECK(MxSimulator_WaitEventsTimeout(t));
    }
    SIM_FINALLY(NULL);
}

static PyObject *post_empty_event(PyObject *self, PyObject *args, PyObject *kwargs) {
    SIM_TRY();
    SIM_CHECK(MxSimulator_PostEmptyEvent());
    SIM_FINALLY(NULL);
}

static PyObject *simulator_run(PyObject *self, PyObject *args, PyObject *kwargs) {
    SIM_TRY();
    double et = mx::arg<double>("et", 0, args, kwargs, -1);
    SIM_CHECK(MxSimulator_Run(et));
    SIM_FINALLY(NULL);
}

static PyObject *simultor_irun(PyObject *self, PyObject *args, PyObject *kwargs) {
    SIM_TRY();
    SIM_CHECK(MxSimulator_InteractiveRun());
    SIM_FINALLY(NULL);
}

static PyObject *simulator_show(PyObject *self, PyObject *args, PyObject *kwargs) {
    SIM_TRY();
    SIM_CHECK(MxSimulator_Show());
    SIM_FINALLY(NULL);
}

static PyObject *simulator_close(PyObject *self) {
    SIM_TRY();
    SIM_CHECK(MxSimulator_Close());
    SIM_FINALLY(NULL);
}

static PyMethodDef simulator_methods[] = {
    { "poll_events", (PyCFunction)simulator_poll_events, METH_STATIC| METH_VARARGS | METH_KEYWORDS, NULL },
    { "wait_events", (PyCFunction)simulator_wait_events, METH_STATIC| METH_VARARGS | METH_KEYWORDS, NULL },
    { "post_empty_event", (PyCFunction)post_empty_event, METH_STATIC| METH_VARARGS | METH_KEYWORDS, NULL },
    { "run", (PyCFunction)simulator_run, METH_STATIC| METH_VARARGS | METH_KEYWORDS, NULL },
    { "irun", (PyCFunction)simultor_irun, METH_STATIC| METH_VARARGS | METH_KEYWORDS, NULL },
    { "show", (PyCFunction)simulator_show, METH_STATIC| METH_VARARGS | METH_KEYWORDS, NULL },
    { "close", (PyCFunction)simulator_close, METH_STATIC| METH_NOARGS, NULL },
    { "context_has_current", (PyCFunction)MxSystem_ContextHasCurrent, METH_STATIC| METH_NOARGS, NULL },
    { "context_make_current", (PyCFunction)MxSystem_ContextMakeCurrent, METH_STATIC| METH_NOARGS, NULL },
    { "context_release", (PyCFunction)MxSystem_ContextRelease, METH_STATIC| METH_NOARGS, NULL },
    { "camera_rotate", (PyCFunction)MxSystem_CameraRotate, METH_STATIC| METH_VARARGS | METH_KEYWORDS, NULL },
    { "_input_hook", (PyCFunction)ipythonInputHook, METH_STATIC | METH_FASTCALL, NULL },
    { NULL, NULL, 0, NULL }
};


PyGetSetDef simulator_getsets[] = {
    {
        .name = "threads",
        .get = [](PyObject *obj, void *p) -> PyObject* {
            SIM_TRY();
            return mx::cast(_Engine.nr_runners);
            SIM_FINALLY(0);
        },
        .set = [](PyObject *obj, PyObject *val, void *p) -> int {
            PyErr_SetString(PyExc_PermissionError, "read only");
            return -1;
        },
        .doc = "test doc",
        .closure = NULL
    },
    {
        .name = "window",
        .get = [](PyObject *obj, void *p) -> PyObject* {
            SIM_TRY();
            PyObject *r = Simulator->app->getWindow();
            Py_INCREF(r);
            return r;
            SIM_FINALLY(0);
        },
        .set = [](PyObject *obj, PyObject *val, void *p) -> int {
            PyErr_SetString(PyExc_PermissionError, "read only");
            return -1;
        },
        .doc = "test doc",
        .closure = NULL
    },
    {NULL}
};

PyTypeObject MxSimulator_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name =           "MxSimulator",
    .tp_basicsize =      sizeof(PyObject),
    .tp_itemsize =       0,
    .tp_dealloc =        0,
                         0, // .tp_print changed to tp_vectorcall_offset in python 3.8
    .tp_getattr =        0,
    .tp_setattr =        0,
    .tp_as_async =       0,
    .tp_repr =           0,
    .tp_as_number =      0,
    .tp_as_sequence =    0,
    .tp_as_mapping =     0,
    .tp_hash =           0,
    .tp_call =           simulator_init,
    .tp_str =            0,
    .tp_getattro =       0,
    .tp_setattro =       0,
    .tp_as_buffer =      0,
    .tp_flags =          Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "Custom objects",
    .tp_traverse =       0,
    .tp_clear =          0,
    .tp_richcompare =    0,
    .tp_weaklistoffset = 0,
    .tp_iter =           0,
    .tp_iternext =       0,
    .tp_methods =        simulator_methods,
    .tp_members =        0,
    .tp_getset =         simulator_getsets,
    .tp_base =           0,
    .tp_dict =           0,
    .tp_descr_get =      0,
    .tp_descr_set =      0,
    .tp_dictoffset =     0,
    .tp_init =           0,
    .tp_alloc =          0,
    .tp_new =            0,
    .tp_free =           0,
    .tp_is_gc =          0,
    .tp_bases =          0,
    .tp_mro =            0,
    .tp_cache =          0,
    .tp_subclasses =     0,
    .tp_weaklist =       0,
    .tp_del =            0,
    .tp_version_tag =    0,
    .tp_finalize =       0,
};

HRESULT _MxSimulator_init(PyObject* m) {

    std::cout << MX_FUNCTION << std::endl;

    PyModule_AddIntConstant(m, "FORWARD_EULER", EngineIntegrator::FORWARD_EULER);
    PyModule_AddIntConstant(m, "RUNGE_KUTTA_4", EngineIntegrator::RUNGE_KUTTA_4);

    
    if (PyType_Ready((PyTypeObject*)&MxSimulator_Type) < 0) {
        return E_FAIL;
    }
    
    PyObject* s = PyObject_New(PyObject, &MxSimulator_Type);

    if(!s) {
        return c_error(E_FAIL, "could not create simulator API");
    }

    PyModule_AddObject(m, "Simulator", s);
    
    PyModule_AddObject(m, "simulator", s);

    return S_OK;
}
