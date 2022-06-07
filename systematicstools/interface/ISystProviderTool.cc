#include "systematicstools/interface/ISystProviderTool.hh"

namespace systtools {

ISystProviderTool::ISystProviderTool(fhicl::ParameterSet const &ps)
    : fToolType{ps.get<std::string>("tool_type")}, fSeedSuggestion{0},
      fIsFullyConfigured{false}, fHaveSystMetaData{false} {
  if (!ps.has_key("instance_name")) {
    fInstanceName = "";
    fFQName = fToolType;
  } else {
    fInstanceName = ps.get<std::string>("instance_name");
    fFQName = fToolType + "_" + fInstanceName;
  }
}

paramId_t ISystProviderTool::GetParameterId(std::string const &prettyName) {
  return GetParamId(fSystMetaData, prettyName);
}

void ISystProviderTool::SuggestSeed(uint64_t seed) {
  if (fHaveSystMetaData) {
    throw ISystProviderTool_seed_suggestion_post_configure()
        << "[ERROR]: Seed was suggested to ISystProvider: "
        << std::quoted(GetFullyQualifiedName()) << " after configuration.";
  }
  fSeedSuggestion = seed;
}

void ISystProviderTool::SuggestParameterThrows(parameter_throws_list_t &&,
                                               bool) {
  throw ISystProviderTool_method_unimplemented()
      << "[ERROR]: Attempted to suggest parameter throws to provider tool "
      << std::quoted(GetToolType())
      << ", but it doesn't handle suggested throws.";
}

void ISystProviderTool::ConfigureFromToolConfig(fhicl::ParameterSet const &ps,
                                                paramId_t firstId) {
  fSystMetaData = this->BuildSystMetaData(ps, firstId);

  // The following check expects them to be ordered, but the provider isn't
  // under any obligation to order them.
  std::stable_sort(fSystMetaData.begin(), fSystMetaData.end(),
                   [](SystParamHeader const &l, SystParamHeader const &r) {
                     return l.systParamId < r.systParamId;
                   });

  for (auto &hdr : fSystMetaData) {
    if (hdr.systParamId != firstId) {
      throw ISystProviderTool_noncontiguous_parameter_Ids()
          << "[ERROR]: Provider " << std::quoted(GetFullyQualifiedName())
          << " failed to set parameter " << std::quoted(hdr.prettyName)
          << " to firstId " << firstId << " != " << hdr.systParamId;
    }
    firstId++;
  }
  fHaveSystMetaData = true;
}

SystMetaData const &ISystProviderTool::GetSystMetaData() const{
  CheckHaveMetaData();
  return fSystMetaData;
}

fhicl::ParameterSet ISystProviderTool::GetParameterHeadersDocument() {

  CheckHaveMetaData();

  fhicl::ParameterSet ParamHeadersDoc;
  std::vector<std::string> HeaderKeys;
  for (auto &hdr : GetSystMetaData()) {
    ParamHeadersDoc.put(hdr.prettyName, SystParamHeaderToFHiCL(hdr));
    HeaderKeys.push_back(hdr.prettyName);
  }
  ParamHeadersDoc.put("parameter_headers", HeaderKeys);
  ParamHeadersDoc.put("tool_type", GetToolType());
  if (GetInstanceName().size()) {
    ParamHeadersDoc.put("instance_name", GetInstanceName());
  }
  fhicl::ParameterSet ToolOptions = GetExtraToolOptions();
  if (!ToolOptions.is_empty()) {
    ParamHeadersDoc.put("tool_options", ToolOptions);
  }

  return ParamHeadersDoc;
}

bool ISystProviderTool::ConfigureFromParameterHeaders(
    fhicl::ParameterSet const &ps) {
  std::vector<std::string> const &ParamHeaderNames =
      ps.get<std::vector<std::string>>("parameter_headers");

  for (auto const &paramName : ParamHeaderNames) {
    fSystMetaData.emplace_back(
        FHiCLToSystParamHeader(ps.get<fhicl::ParameterSet>(paramName)));
  }
  fHaveSystMetaData = true;

  fhicl::ParameterSet ToolOptions;
  ps.get_if_present("tool_options", ToolOptions);

  fIsFullyConfigured = this->SetupResponseCalculator(ToolOptions);

  std::cout << "[INFO]: Syst provider " << std::quoted(GetFullyQualifiedName())
            << " configured " << fSystMetaData.size() << " parameters."
            << std::endl;

  return fIsFullyConfigured;
}

#ifndef NO_ART

ParamResponses 
responses_for(SystParamHeader const& sph)
{
  if (sph.isCorrection) {
    return {sph.systParamId, std::vector{1.}};
  }
  return {sph.systParamId, std::vector<double>(sph.paramVariations.size(), 1.)};
}

systtools::event_unit_response_t
ISystProviderTool::GetDefaultEventResponse() const
{
  auto const& smd = this->GetSystMetaData();
  systtools::event_unit_response_t resp;
  resp.reserve(smd.size());
  for(auto const& sph : smd) {
    resp.push_back(responses_for(sph));
  }
  return resp;
}


std::unique_ptr<EventAndCVResponse>
ISystProviderTool::GetEventVariationAndCVResponse(art::Event const &evt) {
  std::unique_ptr<EventAndCVResponse> responseandCV =
      std::make_unique<EventAndCVResponse>();

  std::unique_ptr<EventResponse> prov_response = GetEventResponse(evt);

  for (event_unit_response_t eur : (*prov_response)) {
    // Foreach param
    event_unit_response_w_cv_t eur_cv;
    for (ParamResponses &pr : eur) {
      // Get CV resp
      SystParamHeader const &hdr = GetParam(GetSystMetaData(), pr.pid);

      double CVResp = hdr.isWeightSystematicVariation ? 1 : 0;
      size_t NVars = hdr.paramVariations.size();

      double cv_param_val = 0;
      if (hdr.centralParamValue != kDefaultDouble) {
        cv_param_val = hdr.centralParamValue;
      }
      for (size_t idx = 0; idx < NVars; ++idx) {
        if (fabs(cv_param_val - hdr.paramVariations[idx]) <=
            std::numeric_limits<float>::epsilon()) {
          CVResp = pr.responses[idx];
          break;
        }
      }
      // if we didn't find it, the CVResp stays as 1/0 depending on whether it
      // is a weight or not.
      for (size_t idx = 0; idx < NVars; ++idx) {
        if (hdr.isWeightSystematicVariation) {
          pr.responses[idx] /= CVResp;
        } else {
          pr.responses[idx] -= CVResp;
        }
      }

      eur_cv.push_back({pr.pid, CVResp, std::move(pr.responses)});
    } // end for each parameter response
    responseandCV->push_back(std::move(eur_cv));
  } // end for each event unit

  return responseandCV;
}
#endif

void ISystProviderTool::CheckHaveMetaData(paramId_t i) const{
  if (!fHaveSystMetaData) {
    throw ISystProviderTool_metadata_not_generated()
        << "[ERROR]: Requested syst set configuration from syst provider "
        << GetFullyQualifiedName() << ", but it has not been generated yet.";
  }
  if (i != kParamUnhandled<paramId_t>) {
    if (!ParamIsHandled(i)) {
      throw ISystProviderTool_metadata_not_generated()
          << "[ERROR]: SuggestParameterThrows Check failed. Parameter "
             "with id \""
          << i << "\", is not handled by this systematic provider: \""
          << GetFullyQualifiedName() << "\".";
    }
  }
}

} // namespace systtools
