#ifndef SYSTTOOLS_INTERFACE_ISYSTPROVIDERTOOL_SEEN
#define SYSTTOOLS_INTERFACE_ISYSTPROVIDERTOOL_SEEN

#include "systematicstools/interface/EventResponse_product.hh"
#include "systematicstools/interface/FHiCLSystParamHeaderConverters.hh"
#include "systematicstools/interface/SystMetaData.hh"

#include "systematicstools/utility/exceptions.hh"

#ifndef NO_ART
#include "art/Framework/Principal/Event.h"
#endif

#include "fhiclcpp/ParameterSet.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <string>

namespace systtools {

NEW_SYSTTOOLS_EXCEPT(ISystProviderTool_method_unimplemented);
NEW_SYSTTOOLS_EXCEPT(ISystProviderTool_seed_suggestion_post_configure);
NEW_SYSTTOOLS_EXCEPT(ISystProviderTool_noncontiguous_parameter_Ids);
NEW_SYSTTOOLS_EXCEPT(ISystProviderTool_metadata_not_generated);
NEW_SYSTTOOLS_EXCEPT(invalid_ToolConfigurationFHiCL);
NEW_SYSTTOOLS_EXCEPT(invalid_ToolOptions);

/// ABC defining the interface to systematic response syst_providers
class ISystProviderTool {
public:
  /// ABC constructor required for art::make_tool
  ISystProviderTool(fhicl::ParameterSet const &ps);

  ///\brief Check if instance handles parameter
  ///
  /// Uses helper methods in systematicstools/interface/SystMetaData.hh to check
  /// for parameters identified by paramId_t or std::string
  template <typename T> bool ParamIsHandled(T ident) const {
    return HasParam(fSystMetaData, ident);
  }

  ///\brief Get paramId_t for handled, named parameter
  paramId_t GetParameterId(std::string const &prettyName);

  ///\brief Get the number of variations to be calculated for parameter i
  template <typename T> size_t GetNVariations(T ident) {
    return GetParam(fSystMetaData, ident).paramVariations.size();
  }

  ///\brief Allows RNG seeds to be suggested to tool instances.
  ///
  /// Instances should use this seed to deterministically generate random
  /// numbers.
  ///
  /// This also stops many syst providers being set up in quick succession all
  /// using similar seeds.
  void SuggestSeed(uint64_t seed);

  ///\brief Suggest a list of parameter throws to an instance.
  ///
  /// Allows a meta provider to be written that delegates well-correlated
  /// throws to mutliple chiild providers.
  ///
  /// \note Unfortunately must be public as sub-classes do not get access to
  /// protected member functions via a base class pointer (as they may actually
  /// call a protected member of another subclass).
  virtual void SuggestParameterThrows(parameter_throws_list_t &&throws,
                                      bool Check = false);

  ///\brief Sub-classes may override this method to provide an example Tool
  /// Configuration FHiCL document.
  virtual fhicl::ParameterSet GetExampleToolConfiguration() {
    fhicl::ParameterSet ex_cfg;
    ex_cfg.put<std::string>("tool_type", GetToolType());
    return ex_cfg;
  }

  ///\brief Configure an ISystProvider instance with tool-specific FHiCL
  ///
  /// Takes the tool-specific FHiCL configuration and the paramId_t of the first
  /// unused paramId_t (closest to 0) and builds the parameter metadata that can
  /// be used to configure the ISystProvider for response calculation and also
  /// interpret the calculated responses.
  ///
  /// Validates that the SystParamHeaders created by the subclass in
  /// `BuildSystMetaData` are contiguous.
  void ConfigureFromToolConfig(fhicl::ParameterSet const &ps,
                               paramId_t firstId);

  ///\brief Gets the currently configured set of systematic parameter headers.
  ///
  /// Checks that the headers have been built/loaded with CheckHaveMetaData,
  /// which throws if they haven't.
  SystMetaData const &GetSystMetaData() const;

  ///\brief Build the Parameter Headers FHiCL document that can be used to
  /// re-configure an instance of this tool via ConfigureFromParameterHeaders
  ///
  /// If a sub-class requires extra configuration options they should be exposed
  /// through GetExtraToolOptions
  fhicl::ParameterSet GetParameterHeadersDocument();

  ///\brief Try and read parameter configuration from input fhicl file.
  ///
  /// After reading parameters, the pure virtual SetupResponseCalculator method
  /// is called for any final subclass configuration.
  ///
  ///\note Sub-classes may not alter fSystMetaData during the configure call.
  /// This is checked for by md5-ing the stringified fhicl representation of the
  /// parameters before and after the call.
  bool ConfigureFromParameterHeaders(fhicl::ParameterSet const &ps);

#ifndef NO_ART
  virtual std::unique_ptr<EventResponse>
  GetEventResponse(art::Event const &) = 0;

  //==== return 1-filled event_unit_response_t
  systtools::event_unit_response_t GetDefaultEventResponse() const;

  std::unique_ptr<EventAndCVResponse>
  GetEventVariationAndCVResponse(art::Event const &);
#endif

  std::string const &GetToolType() const { return fToolType; }
  std::string const &GetFullyQualifiedName() const { return fFQName; }
  std::string const &GetInstanceName() const { return fInstanceName; }

  ///\brief Sub-classes may override this method to provide
  /// string-representations of their state.
  virtual std::string AsString() { return ""; }

  virtual ~ISystProviderTool(){};

protected:
  ///\brief Convert tool-specific configuration fhicl parameter set into generic
  /// SystParamHeaders.
  virtual SystMetaData BuildSystMetaData(fhicl::ParameterSet const &,
                                         paramId_t) = 0;

  ///\brief Gets any extra tool options generated during
  /// ConfigureFromToolConfig that aren't de-serializable to the SystParamHeader
  /// format.
  virtual fhicl::ParameterSet GetExtraToolOptions() {
    return fhicl::ParameterSet();
  }

  ///\brief Any further configuration required by a subclass before
  /// GetEventResponse can be called.
  ///
  /// This is meant for setting up slave weight calculators that are needed to
  /// calculate event responses but not for parameter variation
  /// re-interpretation.
  ///
  /// Configuration returned by GetExtraToolOptions after initial Tool
  /// Configuration will be passed into here during
  /// ConfigureFromParameterHeaders
  virtual bool SetupResponseCalculator(fhicl::ParameterSet const &) = 0;

  ///\brief Checks if internal parameter metadata has been generated or loaded
  /// from a Parameter Headers file.
  ///
  /// If i is passed then it only checks for that specific paramId_t.
  ///
  /// Throws if no such metadata can be found.
  void CheckHaveMetaData(paramId_t i = kParamUnhandled<paramId_t>) const;

  /// Class name of the tool implementation
  std::string fToolType;
  /// A name for an instance of a tool used to disambiguate multiple instances
  /// of the same tool.
  std::string fInstanceName;
  /// The unique name of the tool instance: <fToolType>_<fInstanceName>
  std::string fFQName;

  /// \brief A suggested seed
  ///
  /// Should be used by tool implementations to seed any RNGs to allow
  /// deterministic random numbers across separate executions.
  uint64_t fSeedSuggestion;

  ///\brief Whether this instance successfully configured itself
  ///
  /// Tools may be configured either from a "tool configuration" file, or from a
  /// "parameter headers" file.
  bool fIsFullyConfigured;

private:
  /// Whether this instance has generated/loaded its parameter set.
  bool fHaveSystMetaData;
  /// \brief The SystMetaData describing the parameters handled by this tool.
  ///
  /// \note Only the base class is allowed to alter the SystMetaData after the
  /// original generation. Subclasses and external callers may use
  /// GetSystMetaData to inspect it.
  SystMetaData fSystMetaData;
};

ParamResponses responses_for(SystParamHeader const& sph);

} // namespace systtools

#endif
