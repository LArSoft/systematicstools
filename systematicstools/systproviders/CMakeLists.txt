
cet_build_plugin(ExampleISystProvider art::tool
  LIBRARIES PRIVATE
  systematicstools::interface
  systematicstools::utility
  art::Framework_Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art::Framework_Services_Optional_RandomNumberGenerator_service
  CLHEP::Random
)

# cet_build_plugin(CorrelatedMultisimProvider art::tool
#   LIBRARIES PRIVATE
#   systematicstools_utility
#   art::Framework_Core
#   art::Framework_Principal
#   art::Framework_Services_Registry
#   art::Framework_Services_Optional_RandomNumberGenerator_service
#   CLHEP::Random
# )
