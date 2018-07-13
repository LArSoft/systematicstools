#ifndef LARSYST_INTERFACE_SYSTPARAMHEADER_SEEN
#define LARSYST_INTERFACE_SYSTPARAMHEADER_SEEN

#include "larsyst/interface/types.hh"

#include "larsyst/utility/exceptions.hh"

#include <array>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

namespace larsyst {

struct SystParamHeader {
  SystParamHeader()
      : prettyName{""}, systParamId{kParamUnhandled<paramId_t>},
        isWeightSystematicVariation{true}, unitsAreNatural{false},
        differsEventByEvent{true}, centralParamValue{0xdeadb33f},
        isCorrection{false}, oneSigmaShifts{{0xdeadb33f, 0xdeadb33f}},
        paramValidityRange{{0xdeadb33f, 0xdeadb33f}}, isSplineable{false},
        isRandomlyThrown{false}, paramVariations{}, isResponselessParam{false},
        responseParamId{kParamUnhandled<paramId_t>}, responses{}, opts{} {}
  ///\brief Human readable systematic parameter name
  std::string prettyName;
  ///\brief Unique identifier for this systematic parameter
  ///
  /// Used to key `std::map`-based event data product.
  ///\note Not guaranteed to persist between different configurations. i.e.
  ///`systParamId == 0` might be used for some physics model parameter in one
  /// data product and a calibration parameter in another.
  paramId_t systParamId;
  ///\brief Whether this systematic corresponds to a weight or property shift.
  ///
  ///\note Non-weight systematics will always need custom code on the part of
  /// a downstream consumer.
  bool isWeightSystematicVariation;
  ///\brief Whether the quantities stored in paramVariations and
  /// centralParamValue are in 'natural' units
  bool unitsAreNatural;
  ///\brief Whether the the response of this parameter is fully described by
  /// this meta-data
  ///
  /// Equivalent to `bool(Responses.size())`;
  bool differsEventByEvent;
  ///\brief The central parameter value used in this systematic evaluation.
  ///
  /// Respects unitsAreNatural value.
  double centralParamValue;
  ///\brief Whether to only expect a single response that should always be
  /// applied by consumers.
  ///
  /// Uses centralParamValue to generate a single response, respects
  /// differsEventByEvent.
  bool isCorrection;
  ///\brief The 'one sigma' shifts of this parameter, always defined in nautral
  /// units.
  ///
  /// Can be used by a downstream consumer to convert centralParamValue and
  /// paramVariations to and from natural units.
  std::array<double, 2> oneSigmaShifts;
  ///\brief The range of valid parameter values.
  ///
  /// If either end of the range is set to 0xdeadb33f, that 'side' is unbounded.
  ///
  /// Respects unitsAreNatural
  std::array<double, 2> paramValidityRange;
  ///\brief Whether the paramVariations were chosen to facilitate a downstream
  /// consumer to spline the parameter response.
  ///
  /// When `isSplineable == false`, this parameter has likely been run in
  /// 'multisim' mode.
  bool isSplineable;
  ///\brief Whether the non-splineable variations have been hand-picked to
  /// randomly distributed according to some prior (like gaussian).
  bool isRandomlyThrown;
  ///\brief The shifted values that were calculated for this parameter.
  ///
  /// Contains the parameter values (either in sigma-shift units or natural
  /// units, see `oneSigmaShifts`) that were used to determine responses. The
  /// responses can either be event-level or parameter-level, parameter-level
  /// responses are stored in `responses`.
  std::vector<double> paramVariations;
  ///\brief Whether variations of this parameter produce responses via this
  /// header.
  ///
  /// This is used for multi-dimensional responses, e.g. R(p1,p2), where
  /// R(p1,nominal2) * R(nominal1,p2) !=  R(p1,p2). In this instance, two
  /// parameter headers would be used, one describing variations in p1 and one
  /// in p2. All of the response to variations in both will be included on p1
  ///
  /// As multi-dimensional responses cannot be effectively splined (yet), this
  /// should always be used with numberOfVariations > 0 or isCorrection ==
  /// true.
  ///
  ///\note responseParamId holds the parameter Id that contains R(p1,p2,...).
  bool isResponselessParam;
  ///\brief The parameter Id of where responses to parameters with
  /// isResponselessParam == true can be found.
  paramId_t responseParamId;
  ///\brief The parameter responses for 'parameter-level' systematics.
  ///
  /// Empty for event-by-event parameters, contains universe or spline knot
  /// responses for dials that affect all events in the same way.
  ///
  /// These will most often be used for overall event-class re-normalisations,
  /// which do not need to be stored event-by-event.
  std::vector<double> responses;

  ///\brief Arbitrary string options stored in the meta-data for further
  /// syst-provider configuration.
  std::vector<std::string> opts;
};

NEW_LARSYST_EXCEPT(invalid_SystParamHeader);

///\brief Checks interface validity of a SystParamHeader
///
/// Checks performed:
/// * Has valid Id
/// * Has non-empty pretty name
/// * If it is a correction:
///  * Does it have a specified central value? (should)
///  * Does it have any responses or parameter variations defined? (shouldn't)
/// * If it is not a correction, does it have at least one parameter variation
/// specified?
/// * If it is marked as splineable:
///  * Is it also marked as randomly thrown? (shouldn't)
///  * Is it also marked as responseless? (shouldn't)
/// * If it is marked as responseless:
///  * Does it have a corresponding response parameter? (should)
///  * Does it have any responses defined? (shouldn't)
/// * If it is marked as not differing event-by-event:
///  * Does it have header-level responses defined? (should)
///  * Does it have parameter variations specified? (should unless marked as a
///  correction)
/// * If it is marked as differing event-by-event, does it have header-level
/// responses defined? (shouldn't)
inline bool Validate(SystParamHeader const &hdr, bool quiet = true) {

  if (hdr.systParamId == kParamUnhandled<paramId_t>) {
    if (!quiet) {
      std::cout << "[ERROR]: SystParamHeader has the default systParamId."
                << std::endl;
    }
    return false;
  }
  if (!hdr.prettyName.size()) {
    if (!quiet) {
      std::cout << "[ERROR]: SystParamHeader doesn't have a prettyName."
                << std::endl;
    }
    return false;
  }
  if (hdr.isCorrection) {
    if (hdr.centralParamValue == 0xdeadb33f) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") is marked as a correction but the centralParamValue is "
                     "defaulted."
                  << std::endl;
      }
      return false;
    }
    if (hdr.paramVariations.size() || hdr.responses.size()) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") is marked as a correction but has variations ("
                  << hdr.paramVariations.size() << ") or responses ("
                  << hdr.responses.size() << ")" << std::endl;
      }
      return false;
    }
  } else {
    if (!hdr.paramVariations.size()) {
      if (!quiet) {
        std::cout
            << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
            << std::quoted(hdr.prettyName)
            << ") is not marked as a correction, but contains no variations."
            << std::endl;
      }
      return false;
    }
  }

  if (hdr.isSplineable) { // Splineable
    if (hdr.isRandomlyThrown) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") marked as splineable is also set as randomly thrown."
                  << std::endl;
      }
      return false;
    }
    if (hdr.isResponselessParam) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") marked as splineable is also set as expressing "
                     "response through another parameter ("
                  << hdr.responseParamId << ")." << std::endl;
      }
      return false;
    }
  }
  if (hdr.isResponselessParam) {
    if (hdr.responses.size()) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") marked as responseless, but also has "
                     "header-level responses."
                  << std::endl;
      }
      return false;
    }
    if (hdr.responseParamId == kParamUnhandled<paramId_t>) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") marked as responseless, but it doesn't have a valid, "
                     "associated response parameter."
                  << std::endl;
      }
      return false;
    }
  }
  if (hdr.differsEventByEvent) { // differs event by event
    if (hdr.responses.size()) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") marked as differing event by event, but also has "
                     "header-level responses."
                  << std::endl;
      }
      return false;
    }
  } else {
    if (!hdr.responses.size()) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") marked as not differing event by event, but has no "
                     "header-level responses."
                  << std::endl;
      }
      return false;
    }
    if (!hdr.isCorrection &&
        (hdr.responses.size() != hdr.paramVariations.size())) {
      if (!quiet) {
        std::cout << "[ERROR]: SystParamHeader(" << hdr.systParamId << ":"
                  << std::quoted(hdr.prettyName)
                  << ") marked as differing event by event, but also has "
                     "header-level responses."
                  << std::endl;
      }
      return false;
    }
  }
  return true;
}

} // namespace larsyst
#endif