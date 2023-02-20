#ifndef SYSTTOOLS_INTERFACE_SYSTPARAMHEADER_SEEN
#define SYSTTOOLS_INTERFACE_SYSTPARAMHEADER_SEEN

#include "systematicstools/utility/exceptions.hh"

#include <array>
#include <limits>
#include <string>
#include <vector>

namespace systtools {

/// Parameter indentifier.
///
/// Unique for a given parameter set configuration, but value--parameter
/// associations should never be hard coded by consumers as they may change for
/// different sets of systematic parameters.
typedef unsigned paramId_t;

/// Magic values for signalling that a value is defaulted
constexpr double kDefaultDouble = 0xdeadbeef;

/// Magic value for signalling that a parameter is not configured.
///
/// Often specialized with paramId_t when requesting the Id of a named
/// parameter, or with size_t when requesting the index of a parameter.
template <typename T>
constexpr T kParamUnhandled = std::numeric_limits<T>::max();
template <> inline constexpr double kParamUnhandled<double> = kDefaultDouble;

/// Exception to be thrown when a SystParamHeader fails Validate
/// N.B. It is not thrown by the validate method upon failure, but should be
/// thrown by calling methods that cannot handle invalid SystParamHeaders.
NEW_SYSTTOOLS_EXCEPT(invalid_SystParamHeader);

/// Systematic parameter metadata class
///
/// Instances are used to inform systematic response consumers how to interpret
/// responses. A number of the most commonly used features are explicitly
/// exposed as data members, but extensibility is provided by the opts data
/// member which can hold an arbitrary vector of strings.
///
/// This class is currently only serialized to and from FHiCL and as such,
/// adding new/removing/altering members will break usage, but it is fairly easy
/// to fix in pre-generated parameter sets.
///
/// \note Changes to this class *must* be reflected in
/// systematicstools/interpreters/load_parameter_headers.xx and
/// systematicstools/utility/build_parameter_set_from_header.xx
///
/// Usually analyzers/users will interact with instances through
/// systematicstools/interpreters/ParamHeaderHelper.xx
struct SystParamHeader {
  SystParamHeader()
      : prettyName{""}, systParamId{kParamUnhandled<paramId_t>},
        isWeightSystematicVariation{true}, unitsAreNatural{false},
        differsEventByEvent{true}, centralParamValue{kDefaultDouble},
        isCorrection{false}, oneSigmaShifts{{kDefaultDouble, kDefaultDouble}},
        paramValidityRange{{kDefaultDouble, kDefaultDouble}},
        isSplineable{false}, isRandomlyThrown{false}, paramVariations{},
        isResponselessParam{false},
        responseParamId{kParamUnhandled<paramId_t>}, responses{}, opts{} {}

  ///\brief Human readable systematic parameter name
  std::string prettyName;
  ///\brief Unique identifier for this systematic parameter
  ///
  /// Used to `key` the per-event systematic response data product.
  ///
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
  /// centralParamValue are in 'natural' units or units of 'one sigma'
  /// uncertainty.
  bool unitsAreNatural;
  ///\brief Whether the the response of this parameter is fully described by
  /// this meta-data.
  ///
  /// Equivalent to `bool(Responses.size())`;
  ///
  /// This is used for variations that do not depend on the event properties of
  /// events that variations of this parameter effects, such as normalization
  /// weights for classes of events.
  bool differsEventByEvent;
  ///\brief The parameter value to be considered as the central value when
  /// evaluating variations of this parameter.
  ///
  /// \note This may not be the value generated with in the case of
  /// `isCorrection == true` or when the CV tune changes post-MC production.
  ///
  /// Respects unitsAreNatural value.
  double centralParamValue;
  ///\brief Whether to only expect a single response that should always be
  /// applied by consumers.
  ///
  /// Uses centralParamValue to generate a single response, respects
  /// differsEventByEvent.
  bool isCorrection;
  ///\brief The 'one sigma' shifts of this parameter, if present, always defined
  /// in nautral units.
  ///
  /// Can be used by a downstream consumer to convert centralParamValue and
  /// paramVariations to and from natural units.
  std::array<double, 2> oneSigmaShifts;
  ///\brief The range of valid parameter values.
  ///
  /// If either end of the range is set to `kDefaultDouble`, that 'side' is
  /// unbounded.
  ///
  /// Respects unitsAreNatural
  std::array<double, 2> paramValidityRange;
  ///\brief Whether the paramVariations were chosen to facilitate a downstream
  /// consumer to interpolate between the calculated responses.
  ///
  /// When `isSplineable == false`, this parameter has likely been run in
  /// 'multi-universe' mode.
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
  ///\note responseParamId holds the parameter Id that contains R(p1,p2,...).
  bool isResponselessParam;
  ///\brief The parameter Id of where responses to parameters with
  /// `isResponselessParam == true` can be found.
  paramId_t responseParamId;
  ///\brief The parameter responses for 'parameter-level' systematics.
  ///
  /// Empty for event-by-event parameters, contains universe or spline knot
  /// responses for dials that affect all events in the same way.
  ///
  /// These will most often be used for overall event-class re-normalisations,
  /// which do not need to be stored event-by-event.
  std::vector<double> responses;

  ///\brief Arbitrary string options stored in the metadata for further
  /// `ISystProviderTool` configuration.
  std::vector<std::string> opts;
};

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
bool Validate(SystParamHeader const &hdr, bool quiet = true);

} // namespace systtools
#endif
