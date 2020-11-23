/*
 * MxParticles.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: andy
 */

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include <MxUtil.h>
#include <MxPy.h>
#include <MxNumpy.h>

#include <algorithm>
#include <string>
#include <unordered_map>

const char* MxColor3Names[] = {
    "AliceBlue",
    "AntiqueWhite",
    "Aqua",
    "Aquamarine",
    "Azure",
    "Beige",
    "Bisque",
    "Black",
    "BlanchedAlmond",
    "Blue",
    "BlueViolet",
    "Brown",
    "BurlyWood",
    "CadetBlue",
    "Chartreuse",
    "Chocolate",
    "Coral",
    "CornflowerBlue",
    "Cornsilk",
    "Crimson",
    "Cyan",
    "DarkBlue",
    "DarkCyan",
    "DarkGoldenRod",
    "DarkGray",
    "DarkGreen",
    "DarkKhaki",
    "DarkMagenta",
    "DarkOliveGreen",
    "Darkorange",
    "DarkOrchid",
    "DarkRed",
    "DarkSalmon",
    "DarkSeaGreen",
    "DarkSlateBlue",
    "DarkSlateGray",
    "DarkTurquoise",
    "DarkViolet",
    "DeepPink",
    "DeepSkyBlue",
    "DimGray",
    "DodgerBlue",
    "FireBrick",
    "FloralWhite",
    "ForestGreen",
    "Fuchsia",
    "Gainsboro",
    "GhostWhite",
    "Gold",
    "GoldenRod",
    "Gray",
    "Green",
    "GreenYellow",
    "HoneyDew",
    "HotPink",
    "IndianRed",
    "Indigo",
    "Ivory",
    "Khaki",
    "Lavender",
    "LavenderBlush",
    "LawnGreen",
    "LemonChiffon",
    "LightBlue",
    "LightCoral",
    "LightCyan",
    "LightGoldenRodYellow",
    "LightGrey",
    "LightGreen",
    "LightPink",
    "LightSalmon",
    "LightSeaGreen",
    "LightSkyBlue",
    "LightSlateGray",
    "LightSteelBlue",
    "LightYellow",
    "Lime",
    "LimeGreen",
    "Linen",
    "Magenta",
    "Maroon",
    "MediumAquaMarine",
    "MediumBlue",
    "MediumOrchid",
    "MediumPurple",
    "MediumSeaGreen",
    "MediumSlateBlue",
    "MediumSpringGreen",
    "MediumTurquoise",
    "MediumVioletRed",
    "MidnightBlue",
    "MintCream",
    "MistyRose",
    "Moccasin",
    "NavajoWhite",
    "Navy",
    "OldLace",
    "Olive",
    "OliveDrab",
    "Orange",
    "OrangeRed",
    "Orchid",
    "PaleGoldenRod",
    "PaleGreen",
    "PaleTurquoise",
    "PaleVioletRed",
    "PapayaWhip",
    "PeachPuff",
    "Peru",
    "Pink",
    "Plum",
    "PowderBlue",
    "Purple",
    "Red",
    "RosyBrown",
    "RoyalBlue",
    "SaddleBrown",
    "Salmon",
    "SandyBrown",
    "SeaGreen",
    "SeaShell",
    "Sienna",
    "Silver",
    "SkyBlue",
    "SlateBlue",
    "SlateGray",
    "Snow",
    "SpringGreen",
    "SteelBlue",
    "Tan",
    "Teal",
    "Thistle",
    "Tomato",
    "Turquoise",
    "Violet",
    "Wheat",
    "White",
    "WhiteSmoke",
    "Yellow",
    "YellowGreen",
    NULL
};



static PyObject *random_point_disk(int n) {

    try {
        std::uniform_real_distribution<double> uniform01(0.0, 1.0);

        int nd = 2;

        int typenum = NPY_DOUBLE;

        npy_intp dims[] = {n,3};

        PyArrayObject* array = (PyArrayObject*)PyArray_SimpleNew(nd, dims, typenum);

        double *data = (double*)PyArray_DATA(array);

        for(int i = 0; i < n; ++i) {
            double r = sqrt(uniform01(CRandom));
            double theta = 2 * M_PI * uniform01(CRandom);
            data[i * 3 + 0] = r * cos(theta);
            data[i * 3 + 1] = r * sin(theta);
            data[i * 3 + 2] = 0.;
        }

        return (PyObject*)array;

    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return NULL;
    }
    catch(pybind11::error_already_set &e){
        e.restore();
        return NULL;
    }
}


static PyObject* random_point_sphere(int n) {

    try {

        double radius = 1.0;



        std::uniform_real_distribution<double> uniform01(0.0, 1.0);

        int nd = 2;

        int typenum = NPY_DOUBLE;

        npy_intp dims[] = {n,3};


        PyArrayObject* array = (PyArrayObject*)PyArray_SimpleNew(nd, dims, typenum);

        double *data = (double*)PyArray_DATA(array);

        for(int i = 0; i < n; ++i) {
            double theta = 2 * M_PI * uniform01(CRandom);
            double phi = acos(1 - 2 * uniform01(CRandom));
            double x = radius * sin(phi) * cos(theta);
            double y = radius * sin(phi) * sin(theta);
            double z = radius * cos(phi);

            data[i * 3 + 0] = x;
            data[i * 3 + 1] = y;
            data[i * 3 + 2] = z;
        }

        return (PyObject*)array;

    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return NULL;
    }
    catch(pybind11::error_already_set &e){
        e.restore();
        return NULL;
    }
}

static PyObject* random_point_solidsphere(int n) {

    try {

        std::uniform_real_distribution<double> uniform01(0.0, 1.0);

        int nd = 2;

        int typenum = NPY_DOUBLE;

        npy_intp dims[] = {n,3};


        PyArrayObject* array = (PyArrayObject*)PyArray_SimpleNew(nd, dims, typenum);

        double *data = (double*)PyArray_DATA(array);

        for(int i = 0; i < n; ++i) {
            double theta = 2 * M_PI * uniform01(CRandom);
            double phi = acos(1 - 2 * uniform01(CRandom));
            double r = std::cbrt(uniform01(CRandom));
            double x = r * sin(phi) * cos(theta);
            double y = r * sin(phi) * sin(theta);
            double z = r * cos(phi);

            data[i * 3 + 0] = x;
            data[i * 3 + 1] = y;
            data[i * 3 + 2] = z;
        }

        return (PyObject*)array;

    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return NULL;
    }
    catch(pybind11::error_already_set &e){
        e.restore();
        return NULL;
    }
}

static PyObject* random_point_solidcube(int n) {

    try {


        std::uniform_real_distribution<double> uniform01(-0.5, 0.5);

        int nd = 2;

        int typenum = NPY_DOUBLE;

        npy_intp dims[] = {n,3};

        PyArrayObject* array = (PyArrayObject*)PyArray_SimpleNew(nd, dims, typenum);

        double *data = (double*)PyArray_DATA(array);

        for(int i = 0; i < n; ++i) {
            double x = uniform01(CRandom);
            double y = uniform01(CRandom);
            double z = uniform01(CRandom);
            data[i * 3 + 0] = x;
            data[i * 3 + 1] = y;
            data[i * 3 + 2] = z;
        }

        return (PyObject*)array;

    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return NULL;
    }
    catch(pybind11::error_already_set &e){
        e.restore();
        return NULL;
    }
}



PyObject* MxPoints(PyObject *m, PyObject *args, PyObject *kwargs)
{
    try {
        MxPointsType kind = arg<MxPointsType>("kind", 0, args, kwargs, MxPointsType::Sphere);
        int n  = arg<int>("n", 1, args, kwargs, 1);

        switch(kind) {
        case MxPointsType::Sphere:
            return random_point_sphere(n);
        case MxPointsType::Disk:
            return random_point_disk(n);
        case MxPointsType::SolidCube:
            return random_point_solidcube(n);
        case MxPointsType::SolidSphere:
            return random_point_solidsphere(n);
        default:
            PyErr_SetString(PyExc_ValueError, "invalid kind");
            return NULL;
        }
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return NULL;
    }
    catch(pybind11::error_already_set &e){
        e.restore();
        return NULL;
    }
}




HRESULT _MxUtil_init(PyObject *m)
{
    pybind11::enum_<MxPointsType>(m, "RandomPoints")
    .value("Sphere", MxPointsType::Sphere)
    .value("SolidSphere", MxPointsType::SolidSphere)
    .value("Disk", MxPointsType::Disk)
    .value("SolidCube", MxPointsType::SolidCube)
    .value("Cube", MxPointsType::Cube)
    .export_values();

    return S_OK;
}

Magnum::Color3 Color3_Parse(const std::string &s)
{
    if(s.length() < 2) {
        // TODO ???
        return Magnum::Color3{};
    }

    // #ff6347
    if(s.length() >= 0 && s[0] == '#') {
        std::string srgb = s.substr(1, s.length() - 1);

        char* ss;
        unsigned long rgb = strtoul(srgb.c_str(), &ss, 16);

        return Magnum::Color3::fromSrgb(rgb);
    }

    std::string str = s;
    std::transform(str.begin(), str.end(),str.begin(), ::toupper);

    // TODO, thread safe???
    static std::unordered_map<std::string, Magnum::Color3> colors;
    if(colors.size() == 0) {
        colors["INDIANRED"]         = Magnum::Color3::fromSrgb(0xCD5C5C);
        colors["LIGHTCORAL"]        = Magnum::Color3::fromSrgb(0xF08080);
        colors["SALMON"]            = Magnum::Color3::fromSrgb(0xFA8072);
        colors["DARKSALMON"]        = Magnum::Color3::fromSrgb(0xE9967A);
        colors["LIGHTSALMON"]       = Magnum::Color3::fromSrgb(0xFFA07A);
        colors["CRIMSON"]           = Magnum::Color3::fromSrgb(0xDC143C);
        colors["RED"]               = Magnum::Color3::fromSrgb(0xFF0000);
        colors["FIREBRICK"]         = Magnum::Color3::fromSrgb(0xB22222);
        colors["DARKRED"]           = Magnum::Color3::fromSrgb(0x8B0000);
        colors["PINK"]              = Magnum::Color3::fromSrgb(0xFFC0CB);
        colors["LIGHTPINK"]         = Magnum::Color3::fromSrgb(0xFFB6C1);
        colors["HOTPINK"]           = Magnum::Color3::fromSrgb(0xFF69B4);
        colors["DEEPPINK"]          = Magnum::Color3::fromSrgb(0xFF1493);
        colors["MEDIUMVIOLETRED"]   = Magnum::Color3::fromSrgb(0xC71585);
        colors["PALEVIOLETRED"]     = Magnum::Color3::fromSrgb(0xDB7093);
        colors["LIGHTSALMON"]       = Magnum::Color3::fromSrgb(0xFFA07A);
        colors["CORAL"]             = Magnum::Color3::fromSrgb(0xFF7F50);
        colors["TOMATO"]            = Magnum::Color3::fromSrgb(0xFF6347);
        colors["ORANGERED"]         = Magnum::Color3::fromSrgb(0xFF4500);
        colors["DARKORANGE"]        = Magnum::Color3::fromSrgb(0xFF8C00);
        colors["ORANGE"]            = Magnum::Color3::fromSrgb(0xFFA500);
        colors["GOLD"]              = Magnum::Color3::fromSrgb(0xFFD700);
        colors["YELLOW"]            = Magnum::Color3::fromSrgb(0xFFFF00);
        colors["LIGHTYELLOW"]       = Magnum::Color3::fromSrgb(0xFFFFE0);
        colors["LEMONCHIFFON"]      = Magnum::Color3::fromSrgb(0xFFFACD);
        colors["LIGHTGOLDENRODYELLOW"] = Magnum::Color3::fromSrgb(0xFAFAD2);
        colors["PAPAYAWHIP"]        = Magnum::Color3::fromSrgb(0xFFEFD5);
        colors["MOCCASIN"]          = Magnum::Color3::fromSrgb(0xFFE4B5);
        colors["PEACHPUFF"]         = Magnum::Color3::fromSrgb(0xFFDAB9);
        colors["PALEGOLDENROD"]     = Magnum::Color3::fromSrgb(0xEEE8AA);
        colors["KHAKI"]             = Magnum::Color3::fromSrgb(0xF0E68C);
        colors["DARKKHAKI"]         = Magnum::Color3::fromSrgb(0xBDB76B);
        colors["LAVENDER"]          = Magnum::Color3::fromSrgb(0xE6E6FA);
        colors["THISTLE"]           = Magnum::Color3::fromSrgb(0xD8BFD8);
        colors["PLUM"]              = Magnum::Color3::fromSrgb(0xDDA0DD);
        colors["VIOLET"]            = Magnum::Color3::fromSrgb(0xEE82EE);
        colors["ORCHID"]            = Magnum::Color3::fromSrgb(0xDA70D6);
        colors["FUCHSIA"]           = Magnum::Color3::fromSrgb(0xFF00FF);
        colors["MAGENTA"]           = Magnum::Color3::fromSrgb(0xFF00FF);
        colors["MEDIUMORCHID"]      = Magnum::Color3::fromSrgb(0xBA55D3);
        colors["MEDIUMPURPLE"]      = Magnum::Color3::fromSrgb(0x9370DB);
        colors["REBECCAPURPLE"]     = Magnum::Color3::fromSrgb(0x663399);
        colors["BLUEVIOLET"]        = Magnum::Color3::fromSrgb(0x8A2BE2);
        colors["DARKVIOLET"]        = Magnum::Color3::fromSrgb(0x9400D3);
        colors["DARKORCHID"]        = Magnum::Color3::fromSrgb(0x9932CC);
        colors["DARKMAGENTA"]       = Magnum::Color3::fromSrgb(0x8B008B);
        colors["PURPLE"]            = Magnum::Color3::fromSrgb(0x800080);
        colors["INDIGO"]            = Magnum::Color3::fromSrgb(0x4B0082);
        colors["SLATEBLUE"]         = Magnum::Color3::fromSrgb(0x6A5ACD);
        colors["DARKSLATEBLUE"]     = Magnum::Color3::fromSrgb(0x483D8B);
        colors["MEDIUMSLATEBLUE"]   = Magnum::Color3::fromSrgb(0x7B68EE);
        colors["GREENYELLOW"]       = Magnum::Color3::fromSrgb(0xADFF2F);
        colors["CHARTREUSE"]        = Magnum::Color3::fromSrgb(0x7FFF00);
        colors["LAWNGREEN"]         = Magnum::Color3::fromSrgb(0x7CFC00);
        colors["LIME"]              = Magnum::Color3::fromSrgb(0x00FF00);
        colors["LIMEGREEN"]         = Magnum::Color3::fromSrgb(0x32CD32);
        colors["PALEGREEN"]         = Magnum::Color3::fromSrgb(0x98FB98);
        colors["LIGHTGREEN"]        = Magnum::Color3::fromSrgb(0x90EE90);
        colors["MEDIUMSPRINGGREEN"] = Magnum::Color3::fromSrgb(0x00FA9A);
        colors["SPRINGGREEN"]       = Magnum::Color3::fromSrgb(0x00FF7F);
        colors["MEDIUMSEAGREEN"]    = Magnum::Color3::fromSrgb(0x3CB371);
        colors["SEAGREEN"]          = Magnum::Color3::fromSrgb(0x2E8B57);
        colors["FORESTGREEN"]       = Magnum::Color3::fromSrgb(0x228B22);
        colors["GREEN"]             = Magnum::Color3::fromSrgb(0x008000);
        colors["DARKGREEN"]         = Magnum::Color3::fromSrgb(0x006400);
        colors["YELLOWGREEN"]       = Magnum::Color3::fromSrgb(0x9ACD32);
        colors["OLIVEDRAB"]         = Magnum::Color3::fromSrgb(0x6B8E23);
        colors["OLIVE"]             = Magnum::Color3::fromSrgb(0x808000);
        colors["DARKOLIVEGREEN"]    = Magnum::Color3::fromSrgb(0x556B2F);
        colors["MEDIUMAQUAMARINE"]  = Magnum::Color3::fromSrgb(0x66CDAA);
        colors["DARKSEAGREEN"]      = Magnum::Color3::fromSrgb(0x8FBC8B);
        colors["LIGHTSEAGREEN"]     = Magnum::Color3::fromSrgb(0x20B2AA);
        colors["DARKCYAN"]          = Magnum::Color3::fromSrgb(0x008B8B);
        colors["TEAL"]              = Magnum::Color3::fromSrgb(0x008080);
        colors["AQUA"]              = Magnum::Color3::fromSrgb(0x00FFFF);
        colors["CYAN"]              = Magnum::Color3::fromSrgb(0x00FFFF);
        colors["LIGHTCYAN"]         = Magnum::Color3::fromSrgb(0xE0FFFF);
        colors["PALETURQUOISE"]     = Magnum::Color3::fromSrgb(0xAFEEEE);
        colors["AQUAMARINE"]        = Magnum::Color3::fromSrgb(0x7FFFD4);
        colors["TURQUOISE"]         = Magnum::Color3::fromSrgb(0x40E0D0);
        colors["MEDIUMTURQUOISE"]   = Magnum::Color3::fromSrgb(0x48D1CC);
        colors["DARKTURQUOISE"]     = Magnum::Color3::fromSrgb(0x00CED1);
        colors["CADETBLUE"]         = Magnum::Color3::fromSrgb(0x5F9EA0);
        colors["STEELBLUE"]         = Magnum::Color3::fromSrgb(0x4682B4);
        colors["LIGHTSTEELBLUE"]    = Magnum::Color3::fromSrgb(0xB0C4DE);
        colors["POWDERBLUE"]        = Magnum::Color3::fromSrgb(0xB0E0E6);
        colors["LIGHTBLUE"]         = Magnum::Color3::fromSrgb(0xADD8E6);
        colors["SKYBLUE"]           = Magnum::Color3::fromSrgb(0x87CEEB);
        colors["LIGHTSKYBLUE"]      = Magnum::Color3::fromSrgb(0x87CEFA);
        colors["DEEPSKYBLUE"]       = Magnum::Color3::fromSrgb(0x00BFFF);
        colors["DODGERBLUE"]        = Magnum::Color3::fromSrgb(0x1E90FF);
        colors["CORNFLOWERBLUE"]    = Magnum::Color3::fromSrgb(0x6495ED);
        colors["MEDIUMSLATEBLUE"]   = Magnum::Color3::fromSrgb(0x7B68EE);
        colors["ROYALBLUE"]         = Magnum::Color3::fromSrgb(0x4169E1);
        colors["BLUE"]              = Magnum::Color3::fromSrgb(0x0000FF);
        colors["MEDIUMBLUE"]        = Magnum::Color3::fromSrgb(0x0000CD);
        colors["DARKBLUE"]          = Magnum::Color3::fromSrgb(0x00008B);
        colors["NAVY"]              = Magnum::Color3::fromSrgb(0x000080);
        colors["MIDNIGHTBLUE"]      = Magnum::Color3::fromSrgb(0x191970);
        colors["CORNSILK"]          = Magnum::Color3::fromSrgb(0xFFF8DC);
        colors["BLANCHEDALMOND"]    = Magnum::Color3::fromSrgb(0xFFEBCD);
        colors["BISQUE"]            = Magnum::Color3::fromSrgb(0xFFE4C4);
        colors["NAVAJOWHITE"]       = Magnum::Color3::fromSrgb(0xFFDEAD);
        colors["WHEAT"]             = Magnum::Color3::fromSrgb(0xF5DEB3);
        colors["BURLYWOOD"]         = Magnum::Color3::fromSrgb(0xDEB887);
        colors["TAN"]               = Magnum::Color3::fromSrgb(0xD2B48C);
        colors["ROSYBROWN"]         = Magnum::Color3::fromSrgb(0xBC8F8F);
        colors["SANDYBROWN"]        = Magnum::Color3::fromSrgb(0xF4A460);
        colors["GOLDENROD"]         = Magnum::Color3::fromSrgb(0xDAA520);
        colors["DARKGOLDENROD"]     = Magnum::Color3::fromSrgb(0xB8860B);
        colors["PERU"]              = Magnum::Color3::fromSrgb(0xCD853F);
        colors["CHOCOLATE"]         = Magnum::Color3::fromSrgb(0xD2691E);
        colors["SADDLEBROWN"]       = Magnum::Color3::fromSrgb(0x8B4513);
        colors["SIENNA"]            = Magnum::Color3::fromSrgb(0xA0522D);
        colors["BROWN"]             = Magnum::Color3::fromSrgb(0xA52A2A);
        colors["MAROON"]            = Magnum::Color3::fromSrgb(0x800000);
        colors["WHITE"]             = Magnum::Color3::fromSrgb(0xFFFFFF);
        colors["SNOW"]              = Magnum::Color3::fromSrgb(0xFFFAFA);
        colors["HONEYDEW"]          = Magnum::Color3::fromSrgb(0xF0FFF0);
        colors["MINTCREAM"]         = Magnum::Color3::fromSrgb(0xF5FFFA);
        colors["AZURE"]             = Magnum::Color3::fromSrgb(0xF0FFFF);
        colors["ALICEBLUE"]         = Magnum::Color3::fromSrgb(0xF0F8FF);
        colors["GHOSTWHITE"]        = Magnum::Color3::fromSrgb(0xF8F8FF);
        colors["WHITESMOKE"]        = Magnum::Color3::fromSrgb(0xF5F5F5);
        colors["SEASHELL"]          = Magnum::Color3::fromSrgb(0xFFF5EE);
        colors["BEIGE"]             = Magnum::Color3::fromSrgb(0xF5F5DC);
        colors["OLDLACE"]           = Magnum::Color3::fromSrgb(0xFDF5E6);
        colors["FLORALWHITE"]       = Magnum::Color3::fromSrgb(0xFFFAF0);
        colors["IVORY"]             = Magnum::Color3::fromSrgb(0xFFFFF0);
        colors["ANTIQUEWHITE"]      = Magnum::Color3::fromSrgb(0xFAEBD7);
        colors["LINEN"]             = Magnum::Color3::fromSrgb(0xFAF0E6);
        colors["LAVENDERBLUSH"]     = Magnum::Color3::fromSrgb(0xFFF0F5);
        colors["MISTYROSE"]         = Magnum::Color3::fromSrgb(0xFFE4E1);
        colors["GAINSBORO"]         = Magnum::Color3::fromSrgb(0xDCDCDC);
        colors["LIGHTGRAY"]         = Magnum::Color3::fromSrgb(0xD3D3D3);
        colors["SILVER"]            = Magnum::Color3::fromSrgb(0xC0C0C0);
        colors["DARKGRAY"]          = Magnum::Color3::fromSrgb(0xA9A9A9);
        colors["GRAY"]              = Magnum::Color3::fromSrgb(0x808080);
        colors["DIMGRAY"]           = Magnum::Color3::fromSrgb(0x696969);
        colors["LIGHTSLATEGRAY"]    = Magnum::Color3::fromSrgb(0x778899);
        colors["SLATEGRAY"]         = Magnum::Color3::fromSrgb(0x708090);
        colors["DARKSLATEGRAY"]     = Magnum::Color3::fromSrgb(0x2F4F4F);
        colors["BLACK"]             = Magnum::Color3::fromSrgb(0x000000);
    }

    std::unordered_map<std::string, Magnum::Color3>::const_iterator got =
            colors.find (str);

    if (got != colors.end()) {
        return got->second;
    }
    
    std::string warning = std::string("Warning, \"") + s + "\" is not a valid color name.";
    
    PyErr_WarnEx(PyExc_Warning, warning.c_str(), 0);

    return Magnum::Color3{};
}





