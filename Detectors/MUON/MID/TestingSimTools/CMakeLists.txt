o2_add_library(MIDTestingSimTools
               SOURCES src/HitFinder.cxx src/TrackGenerator.cxx
               PUBLIC_LINK_LIBRARIES O2::DataFormatsMID O2::MIDBase)
