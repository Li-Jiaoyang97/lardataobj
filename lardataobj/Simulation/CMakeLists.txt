cet_make_library(LIBRARY_NAME sim INTERFACE
  SOURCE sim.h
  LIBRARIES INTERFACE
  ROOT::MathCore
)

cet_make_library(LIBRARY_NAME GeneratedParticleInfo INTERFACE
  SOURCE GeneratedParticleInfo.h
  LIBRARIES INTERFACE
  nusimdata::SimulationBase
)

cet_make_library(SOURCE
  AuxDetSimChannel.cxx
  SimDriftedElectronCluster.h
  SimEnergyDeposit.h
  OpDetBacktrackerRecord.cxx
  SimChannel.cxx
  SimPhotons.cxx
  SupernovaTruth.cxx
  ParticleAncestryMap.cxx
  LIBRARIES
  PUBLIC
  larcoreobj::geo_vectors
  PRIVATE
  lardataobj::sim
  messagefacility::MF_MessageLogger
)

build_dictionary(DICTIONARY_LIBRARIES
  lardataobj::Simulation
  lardataobj::GeneratedParticleInfo
  canvas::canvas
)

install_headers()
install_source()
