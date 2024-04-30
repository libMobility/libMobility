/*Raul P. Pelaez 2021.
The MOBILITY_PYTHONIFY(className, description) macro creates a pybind11 module from a class (called className) that inherits from libmobility::Mobility. "description" is a string that will be printed when calling help(className) from python (accompanied by the default documentation of the mobility interface.
 */
#include <stdexcept>
#ifndef MOBILITY_PYTHONIFY_H
#include"MobilityInterface.h"
#include<pybind11/pybind11.h>
#include<pybind11/numpy.h>
namespace py = pybind11;
using namespace pybind11::literals;
using pyarray = py::array_t<libmobility::real>;

#define MOBILITYSTR(s) xMOBILITYSTR(s)
#define xMOBILITYSTR(s) #s

inline auto string2Periodicity(std::string per){
  using libmobility::periodicity_mode;
  if(per == "open") return periodicity_mode::open;
  else if(per == "unspecified") return periodicity_mode::unspecified;
  else if(per == "single_wall") return periodicity_mode::single_wall;
  else if(per == "two_walls") return periodicity_mode::two_walls;
  else if(per == "periodic") return periodicity_mode::periodic;
  else throw std::runtime_error("[libMobility] Invalid periodicity");
}

inline auto createConfiguration(std::string perx, std::string pery, std::string perz){
  libmobility::Configuration conf;
  conf.periodicityX = string2Periodicity(perx);
  conf.periodicityY = string2Periodicity(pery);
  conf.periodicityZ = string2Periodicity(perz);
  return conf;
}

const char *constructor_docstring = R"pbdoc(
Initialize the module with a given set of periodicity conditions.

Each periodicity condition can be one of the following:
	- open: No periodicity in the corresponding direction.
	- unspecified: The periodicity is not specified.
	- single_wall: The system is bounded by a single wall in the corresponding direction.
	- two_walls: The system is bounded by two walls in the corresponding direction.
	- periodic: The system is periodic in the corresponding direction.

Parameters
----------
periodicityX : str
		Periodicity condition in the x direction.
periodicityY : str
		Periodicity condition in the y direction.
periodicityZ : str
		Periodicity condition in the z direction.
)pbdoc";

#define xMOBILITY_PYTHONIFY(MODULENAME, EXTRACODE, documentation)	\
  PYBIND11_MODULE(MODULENAME, m){		      \
  using real = libmobility::real;		      \
  using Parameters = libmobility::Parameters;				\
  using Configuration = libmobility::Configuration;			\
  auto solver = py::class_<MODULENAME>(m, MOBILITYSTR(MODULENAME), documentation); \
  solver.def(py::init([](std::string perx, std::string pery, std::string perz){	\
    return std::unique_ptr<MODULENAME>(new MODULENAME(createConfiguration(perx, pery, perz))); }), \
    constructor_docstring, "periodicityX"_a, "periodicityY"_a, "periodicityZ"_a). \
  def("initialize", [](MODULENAME &myself, real T, real eta, real a, int N){ \
    Parameters par;							\
    par.temperature = T;						\
    par.viscosity = eta;						\
    par.hydrodynamicRadius = {a};					\
    par.numberParticles = N;						\
    myself.initialize(par);						\
  },									\
    R"pbdoc(Initialize the module with a given set of parameters.)pbdoc",		\
    "temperature"_a, "viscosity"_a,					\
    "hydrodynamicRadius"_a,						\
    "numberParticles"_a).						\
    def("setPositions", [](MODULENAME &myself, pyarray pos){myself.setPositions(pos.data());}, \
	"The module will compute the mobility according to this set of positions.", \
	"positions"_a).							\
    def("Mdot", [](MODULENAME &myself, pyarray forces, pyarray result){\
      auto f = forces.size()?forces.data():nullptr;			\
      myself.Mdot(f, result.mutable_data());},			\
      "Computes the product of the RPY Mobility matrix with a group of forces.",	\
      "forces"_a = pyarray(), "result"_a).	\
    def("sqrtMdotW", [](MODULENAME &myself, pyarray result, libmobility::real prefactor){ \
      myself.sqrtMdotW(result.mutable_data(), prefactor);}, \
      "Computes the stochastic contribution, sqrt(2*T*M) dW, where M is the mobility and dW is a Weiner process.", \
      "result"_a = pyarray(), "prefactor"_a = 1.0).			\
    def("hydrodynamicVelocities", [](MODULENAME &myself, pyarray forces,\
					       pyarray result, libmobility::real prefactor){ \
      auto f = forces.size()?forces.data():nullptr;			\
      myself.hydrodynamicVelocities(f, result.mutable_data(), prefactor);}, \
	"Computes the hydrodynamic (deterministic and stochastic) velocities. If the forces are ommited only the stochastic part is computed. If the temperature is zero (default) the stochastic part is ommited. The result is equivalent to calling Mdot followed by stochasticDisplacements.", \
	"forces"_a = pyarray(), "result"_a  = pyarray(), "prefactor"_a = 1). \
    def("clean", &MODULENAME::clean, "Frees any memory allocated by the module."). \
    def_property_readonly_static("precision", [](py::object){return MODULENAME::precision;}, "Compilation precision, a string holding either float or double."); \
  EXTRACODE\
}
#define MOBILITY_PYTHONIFY(MODULENAME, documentationPrelude) xMOBILITY_PYTHONIFY(MODULENAME,; ,documentationPrelude)
#define MOBILITY_PYTHONIFY_WITH_EXTRA_CODE(MODULENAME, EXTRA, documentationPrelude) xMOBILITY_PYTHONIFY(MODULENAME,  EXTRA, documentationPrelude)
#endif
