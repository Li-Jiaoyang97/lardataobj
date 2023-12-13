/**
 * \class ParticleAncestryMap
 *
 * \brief Product to store the mapping of track IDs that were
 *        dropped in largeant back to their ancestor that wasn't
 *        dropped.
 *
 * \author Henry Lay (h.lay@lancaster.ac.uk)
 *
 */

#ifndef PARTICLEANCESTRYMAP_H
#define PARTICLEANCESTRYMAP_H

#include <map>
#include <set>

namespace sim {

  class ParticleAncestryMap {

    std::map<int, std::set<int>> fParticleMap;

  public:
    // Default constructor
    ParticleAncestryMap() {}

    // Construction from a pre filled map
    ParticleAncestryMap(const std::map<int, std::set<int>>& particleMap) : fParticleMap(particleMap)
    {}

    // Set the map contents
    void SetMap(const std::map<int, std::set<int>>& map);

    // Return the whole ancestry map
    std::map<int, std::set<int>> const& GetMap() const;

    // Whether there are any dropped descendants for a given track id
    bool HasDroppedDescendants(const int trackid) const;

    // Return the set of dropped descendants from a given ancestor's track id
    std::set<int> const& GetAllDroppedDescendants(const int trackid) const;

    // Get the stored ancestor from a given dropped track id
    int GetAncestor(const int trackid) const;

    // Whether or not the returned track id from GetAncestor is valid or set to the default value
    bool Exists(const int trackid) const;
  };
}
#endif