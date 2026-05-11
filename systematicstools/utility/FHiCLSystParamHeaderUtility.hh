#ifndef SYSTTOOLS_UTILITY_FHICLSYSTPARAMHEADERUTILITY_SEEN
#define SYSTTOOLS_UTILITY_FHICLSYSTPARAMHEADERUTILITY_SEEN

#include "systematicstools/utility/exceptions.hh"

#include <string>
#include <cstdint>  // for uint64_t

namespace fhicl {
class ParameterSet;
}
namespace systtools {
struct SystParamHeader;
}

namespace systtools {

NEW_SYSTTOOLS_EXCEPT(invalid_FHiCL_variation_descriptor);
NEW_SYSTTOOLS_EXCEPT(invalid_FHiCL_random_distribution_descriptor);

///\brief Set up SystParamHeader variation definitions from common format
///
/// Returns whether any setup occured
///
/// Uses string value of key in paramset to initialize SystParamHeader variation
/// datamembers for a few common uses:
///
/// * New central value: { CV_key: <value> }
///   - SystParamHeader::isCorrection = true
///   - SystParamHeader::centralParamValue = <value>
/// * One sigma shifts (sym): { vardescriptor_key: "{<shift>}"}
///   - SystParamHeader::isRandomlyThrown = true
///   - SystParamHeader::oneSigmaShifts = {-<shift>, <shift>}
/// * One sigma shifts (asym): { vardescriptor_key: "{<low>, <high>}"}
///   - SystParamHeader::isRandomlyThrown = true
///   - SystParamHeader::oneSigmaShifts = {<low>, <high>}
/// * List descriptor: { vardescriptor_key: "(1,2,0.5)"}
///   - SystParamHeader::isSplineable = true
///   - SystParamHeader::paramVariations = {1, 1.5, 2}
/// * Discrete variations: { vardescriptor_key: "[5, 3, 1, 4]" }
///   - SystParamHeader::paramVariations = {5, 3, 1, 4}
///
/// throws invalid_FHiCL_variation_descriptor on error
bool ParseFHiCLVariationDescriptor(fhicl::ParameterSet const &paramset,
                                   std::string const &CV_key,
                                   std::string const &vardescriptor_key,
                                   SystParamHeader &hdr);

///\brief Throws random parameter variations
///
/// Returns whether any throws were made

///
/// If distribution_key is not found, a gaussian distribution will be used.
/// Currently handles "normal", "gaussian", and "uniform" distributions, other
/// values will cause a invalid_FHiCL_random_distribution_descriptor exception
/// to be thrown.
///
/// If SystParamHeader::isRandomlyThrown is not true, or nthrows_key cannot be
/// found in paramset and the NThrows argument is 0, hdr is not modified.
///
/// If no seed is passed, the current time will be used.
bool MakeFHiCLDefinedRandomVariations(fhicl::ParameterSet const &paramset,
                                      std::string const &nthrows_key,
                                      SystParamHeader &hdr,
                                      std::string const &distribution_key = "",
                                      uint64_t seed = 0, size_t NThrows = 0);

///\brief Checks if paramset appears to provide standardized Tool Configuration
/// for a named parameter
///
/// If either "<parameter_name>_central_value" or
/// "<parameter_name>_variation_descriptor" exist, the parameter named
/// <parameter_name> is considered to exist in the configuration.
bool FHiCLSimpleToolConfigurationParameterExists(
    fhicl::ParameterSet const &paramset, std::string const &parameter_name);

///\brief Builds SystParamHeader from standardized FHiCL that can be used to
/// write Tool Configuration files.
///
/// Returns if parameter configuation keys were found.
///
/// Looks for the following keys in paramset:
/// * <parameter_name>_central_value
/// * <parameter_name>_variation_descriptor
///
/// e.g { MyParam_central_value: 0.5 MyParam_tweak_definition: (-3,3,1) }
/// Will build the SystParamHeader: {
///  prettyName = MyParam
///  centralParamValue = 0.5
///  paramVariations = [-3, -2, -1, 0, 1, 2, 3]
/// }
///
/// Uses ParseFHiCLVariationDescriptor and MakeFHiCLDefinedRandomVariations
bool ParseFHiCLSimpleToolConfigurationParameter(
    fhicl::ParameterSet const &paramset, std::string const &parameter_name,
    SystParamHeader &hdr, uint64_t seed = 0, size_t NThrows = 0);

} // namespace systtools
#endif
