
art_dictionary()

cet_build_plugin(SystToolsEventResponse art::EDProducer
  LIBRARIES PRIVATE
  systematicstools::utility
  systematicstools::interface
  art::Persistency_Common
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  ROOT::Core
)

