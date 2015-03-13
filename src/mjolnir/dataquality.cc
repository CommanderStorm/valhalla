#include "mjolnir/dataquality.h"
#include <fstream>

using namespace valhalla::midgard;
using namespace valhalla::baldr;

namespace valhalla {
namespace mjolnir {

// Constructor
DataQuality::DataQuality()
    : simplerestrictions(0),
      timedrestrictions(0),
      turnchannelcount(0),
      internalcount(0),
      culdesaccount(0),
      node_counts{} {
}

// Add statistics (accumulate from several DataQuality objects)
void DataQuality::AddStatistics(const DataQuality& stats) {
  simplerestrictions += stats.simplerestrictions;
  timedrestrictions  += stats.timedrestrictions;
  turnchannelcount   += stats.turnchannelcount;
  internalcount      += stats.internalcount;
  culdesaccount      += stats.culdesaccount;
  for (uint32_t i = 0; i < 128; i++) {
    node_counts[i] += stats.node_counts[i];
  }
}

// Adds an issue.
void DataQuality::AddIssue(const DataIssueType issuetype, const GraphId& graphid,
            const uint64_t wayid1, const uint64_t wayid2) {
  if (issuetype == kDuplicateWays) {
    std::pair<uint64_t, uint64_t> wayids = std::make_pair(wayid1, wayid2);
    auto it = duplicateways_.find(wayids);
    if (it == duplicateways_.end()) {
      duplicateways_.emplace(wayids, 1);
    } else {
      it->second++;
    }
  } else if (issuetype == kUnconnectedLinkEdge) {
    unconnectedlinks_.insert(wayid1);
  } else if (issuetype == kIncompatibleLinkUse) {
    incompatiblelinkuse_.insert(wayid1);
  }
}

// Logs statistics and issues
void DataQuality::LogStatistics() const {
  LOG_INFO("Turn Channel Count = " + std::to_string(turnchannelcount));
  LOG_INFO("Simple Restriction Count = " + std::to_string(simplerestrictions));
  LOG_INFO("Timed  Restriction Count = " + std::to_string(timedrestrictions));
  LOG_INFO("Internal Intersection Edge Count = " + std::to_string(internalcount));
  LOG_INFO("Cul-de-Sac Count = " + std::to_string(culdesaccount));
  LOG_INFO("Node edge count histogram:");
  for (uint32_t i = 0; i < 128; i++) {
    if (node_counts[i] > 0) {
      LOG_INFO(std::to_string(i) + ": " + std::to_string(node_counts[i]));
    }
  }
}

// Logs issues
void DataQuality::LogIssues() const {
  // Log the duplicate ways - sort by number of duplicate edges

  uint32_t duplicates = 0;
  std::vector<DuplicateWay> dups;
  if (duplicateways_.size() > 0) {
    LOG_WARN("Duplicate Ways: count = " + std::to_string(duplicateways_.size()));
    for (const auto& dup : duplicateways_) {
      dups.emplace_back(DuplicateWay(dup.first.first, dup.first.second,
                                      dup.second));
      duplicates += dup.second;
    }
    LOG_WARN("Duplicate ways " + std::to_string(duplicateways_.size()) +
             " duplicate edges = " + std::to_string(duplicates));
  }

  // Sort by edgecount and write to separate file
  std::ofstream dupfile;
  std::sort(dups.begin(), dups.end());
  dupfile.open("duplicateways.txt", std::ofstream::out | std::ofstream::app);
  dupfile << "WayID1   WayID2    DuplicateEdges" << std::endl;
  for (const auto& dupway : dups) {
    dupfile << dupway.wayid1 << "," << dupway.wayid2 << ","
            << dupway.edgecount << std::endl;
  }
  dupfile.close();

  // Log the unconnected link edges
  if (unconnectedlinks_.size() > 0) {
    LOG_WARN("Link edges that are not connected. OSM Way Ids");
    for (const auto& wayid : unconnectedlinks_) {
      LOG_WARN(std::to_string(wayid));
    }
  }

  // Log the links with incompatible use
  if (incompatiblelinkuse_.size() > 0) {
    LOG_WARN("Link edges that have incompatible use. OSM Way Ids:");
    for (const auto& wayid : incompatiblelinkuse_) {
      LOG_WARN(std::to_string(wayid));
    }
  }
}

}
}
