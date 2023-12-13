//
// Build a dictionary.
//
// $Author:  $
// $Date: 2010/04/12 18:12:28 $
//
// Original author Rob Kutschke, modified by klg and Steven Gardiner

#include "canvas/Persistency/Common/Wrapper.h"
#include "canvas/Persistency/Common/Assns.h"

// nusimdata includes
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"

// Simulation includes
#include "lardataobj/Simulation/SimEnergyDeposit.h"
#include "lardataobj/Simulation/AuxDetHit.h"
#include "lardataobj/Simulation/SimChannel.h"
#include "lardataobj/Simulation/OpDetBacktrackerRecord.h"
#include "lardataobj/Simulation/ParticleAncestryMap.h"
#include "lardataobj/Simulation/SimPhotons.h"
#include "lardataobj/Simulation/BeamGateInfo.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "lardataobj/Simulation/SupernovaTruth.h"
#include "lardataobj/Simulation/GeneratedParticleInfo.h"
#include "lardataobj/Simulation/SimDriftedElectronCluster.h"
