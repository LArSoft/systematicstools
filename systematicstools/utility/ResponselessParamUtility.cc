#include "systematicstools/utility/ResponselessParamUtility.hh"

#include "systematicstools/interface/ISystProviderTool.hh"

#include <iomanip>

namespace systtools {

void FinalizeAndValidateDependentParameters(
    SystMetaData &metadata, std::string const &response_parameter_name,
    std::vector<std::string> const &dependent_parameter_names) {

  uint64_t NVariations = 0;

  if (!HasParam(metadata, response_parameter_name)) {
    return;
  }

  SystParamHeader &resp_hdr = GetParam(metadata, response_parameter_name);

  for (auto const &param_name : dependent_parameter_names) {
    if (HasParam(metadata, param_name)) {
      SystParamHeader const &hdr = GetParam(metadata, param_name);
      if (NVariations && (NVariations != hdr.paramVariations.size())) {
        throw invalid_ToolConfigurationFHiCL()
            << "[ERROR]: Parameter " << std::quoted(param_name)
            << " configured with " << hdr.paramVariations.size()
            << " variations, but another responseless parameter with the "
               "same response parameter: "
            << std::quoted(resp_hdr.prettyName) << " is configured with "
            << NVariations << " variations.";
      }
      else{
        NVariations = hdr.paramVariations.size();
      }
    }
  }

  resp_hdr.paramVariations.clear();
  for (size_t i = 0; i < NVariations; ++i) {
    resp_hdr.paramVariations.push_back(i);
  }
}

} // namespace systtools
