o2_add_library(MIDWorkflow
               SOURCES src/ClusterizerSpec.cxx
                       src/ClusterLabelerSpec.cxx
                       src/DigitReaderSpec.cxx
                       src/RecoWorkflow.cxx
                       src/TrackerSpec.cxx
                       src/TrackLabelerSpec.cxx
               PUBLIC_LINK_LIBRARIES O2::Framework
                                     ms_gsl::ms_gsl
                                     O2::DataFormatsMID
                                     O2::SimulationDataFormat
                                     O2::MIDSimulation
                                     O2::MIDTracking
                                     O2::DPLUtils)

o2_add_executable(reco-workflow
                  SOURCES src/mid-reco-workflow.cxx
                  COMPONENT_NAME mid
                  PUBLIC_LINK_LIBRARIES O2::MIDWorkflow)
