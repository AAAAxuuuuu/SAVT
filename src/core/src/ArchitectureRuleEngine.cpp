#include "savt/core/ArchitectureRuleEngine.h"

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace savt::core {

namespace {

int severityRank(const ArchitectureRuleSeverity severity) {
    switch (severity) {
    case ArchitectureRuleSeverity::Critical:
        return 3;
    case ArchitectureRuleSeverity::Warning:
        return 2;
    case ArchitectureRuleSeverity::Info:
    default:
        return 1;
    }
}

std::size_t hubDegreeThreshold(const CapabilityGraph& graph) {
    const std::size_t visibleNodeCount = std::count_if(
        graph.nodes.begin(),
        graph.nodes.end(),
        [](const CapabilityNode& node) { return node.defaultVisible; });

    if (visibleNodeCount >= 12) {
        return 5;
    }
    if (visibleNodeCount >= 7) {
        return 4;
    }
    return 3;
}

void sortAndUniqueNodeIds(std::vector<std::size_t>& nodeIds) {
    std::sort(nodeIds.begin(), nodeIds.end());
    nodeIds.erase(std::unique(nodeIds.begin(), nodeIds.end()), nodeIds.end());
}

ArchitectureRuleFinding makeFinding(
    const ArchitectureRuleKind kind,
    const ArchitectureRuleSeverity severity,
    std::vector<std::size_t> nodeIds,
    std::vector<std::string> reasonCodes) {
    sortAndUniqueNodeIds(nodeIds);
    std::sort(reasonCodes.begin(), reasonCodes.end());
    reasonCodes.erase(std::unique(reasonCodes.begin(), reasonCodes.end()), reasonCodes.end());

    ArchitectureRuleFinding finding;
    finding.kind = kind;
    finding.severity = severity;
    finding.nodeIds = std::move(nodeIds);
    finding.reasonCodes = std::move(reasonCodes);
    return finding;
}

std::vector<ArchitectureRuleFinding> evaluateCycleRules(
    const CapabilityGraph& graph,
    const ArchitectureOverview& overview) {
    struct CycleBucket {
        std::unordered_set<std::size_t> nodeIdSet;
        bool containsEntryNode = false;
        bool fromOverviewGroup = false;
    };

    std::unordered_map<std::size_t, const OverviewNode*> overviewIndex;
    std::size_t syntheticGroupSeed = 1;
    for (const OverviewNode& node : overview.nodes) {
        overviewIndex.emplace(node.id, &node);
        if (node.cycleGroupId >= syntheticGroupSeed) {
            syntheticGroupSeed = node.cycleGroupId + 1;
        }
    }

    std::unordered_map<std::size_t, CycleBucket> buckets;
    for (const CapabilityNode& node : graph.nodes) {
        if (!node.cycleParticipant) {
            continue;
        }

        std::unordered_set<std::size_t> cycleGroupIds;
        bool hasOverviewCycleGroup = false;
        for (const std::size_t overviewNodeId : node.overviewNodeIds) {
            const auto overviewIt = overviewIndex.find(overviewNodeId);
            if (overviewIt == overviewIndex.end()) {
                continue;
            }
            const OverviewNode* overviewNode = overviewIt->second;
            if (!overviewNode->cycleParticipant) {
                continue;
            }
            if (overviewNode->cycleGroupId != 0) {
                cycleGroupIds.insert(overviewNode->cycleGroupId);
                hasOverviewCycleGroup = true;
            }
        }

        if (cycleGroupIds.empty()) {
            cycleGroupIds.insert(syntheticGroupSeed++);
        }

        for (const std::size_t cycleGroupId : cycleGroupIds) {
            CycleBucket& bucket = buckets[cycleGroupId];
            bucket.nodeIdSet.insert(node.id);
            bucket.containsEntryNode =
                bucket.containsEntryNode || node.kind == CapabilityNodeKind::Entry;
            bucket.fromOverviewGroup = bucket.fromOverviewGroup || hasOverviewCycleGroup;
        }
    }

    std::vector<ArchitectureRuleFinding> findings;
    findings.reserve(buckets.size());
    for (auto& [bucketId, bucket] : buckets) {
        (void)bucketId;
        std::vector<std::size_t> nodeIds(bucket.nodeIdSet.begin(), bucket.nodeIdSet.end());
        if (nodeIds.empty()) {
            continue;
        }

        std::vector<std::string> reasonCodes = {"cycle_participant"};
        if (bucket.fromOverviewGroup) {
            reasonCodes.push_back("shared_overview_cycle_group");
        }
        if (bucket.containsEntryNode) {
            reasonCodes.push_back("contains_entry_node");
        }

        const ArchitectureRuleSeverity severity =
            (bucket.containsEntryNode || nodeIds.size() >= 3)
                ? ArchitectureRuleSeverity::Critical
                : ArchitectureRuleSeverity::Warning;

        findings.push_back(makeFinding(
            ArchitectureRuleKind::Cycle,
            severity,
            std::move(nodeIds),
            std::move(reasonCodes)));
    }

    return findings;
}

std::vector<ArchitectureRuleFinding> evaluateHubRules(const CapabilityGraph& graph) {
    const std::size_t degreeFloor = hubDegreeThreshold(graph);
    std::vector<ArchitectureRuleFinding> findings;

    for (const CapabilityNode& node : graph.nodes) {
        const std::size_t totalDegree = node.incomingEdgeCount + node.outgoingEdgeCount;
        const bool highInDegree = node.incomingEdgeCount >= degreeFloor;
        const bool highOutDegree = node.outgoingEdgeCount >= degreeFloor;
        const bool balancedHub =
            totalDegree >= degreeFloor && node.incomingEdgeCount >= 2 && node.outgoingEdgeCount >= 2;

        if (!highInDegree && !highOutDegree && !balancedHub) {
            continue;
        }

        std::vector<std::string> reasonCodes = {"high_total_degree"};
        if (highInDegree) {
            reasonCodes.push_back("high_in_degree");
        }
        if (highOutDegree) {
            reasonCodes.push_back("high_out_degree");
        }
        if (node.reachableFromEntry) {
            reasonCodes.push_back("reachable_from_entry");
        }

        const ArchitectureRuleSeverity severity =
            (totalDegree >= degreeFloor + 3 || (highInDegree && highOutDegree))
                ? ArchitectureRuleSeverity::Critical
                : ArchitectureRuleSeverity::Warning;

        findings.push_back(makeFinding(
            ArchitectureRuleKind::Hub,
            severity,
            {node.id},
            std::move(reasonCodes)));
    }

    return findings;
}

std::vector<ArchitectureRuleFinding> evaluateIsolatedRules(const CapabilityGraph& graph) {
    std::vector<ArchitectureRuleFinding> findings;

    for (const CapabilityNode& node : graph.nodes) {
        if (node.kind == CapabilityNodeKind::Entry) {
            continue;
        }

        const std::size_t totalDegree = node.incomingEdgeCount + node.outgoingEdgeCount;
        const bool fullyIsolated = totalDegree == 0;
        const bool weaklyConnected = totalDegree == 1 && !node.reachableFromEntry;

        if (!fullyIsolated && !weaklyConnected) {
            continue;
        }

        std::vector<std::string> reasonCodes;
        if (fullyIsolated) {
            reasonCodes.push_back("no_relationships");
        } else {
            reasonCodes.push_back("single_relationship");
        }
        if (!node.reachableFromEntry) {
            reasonCodes.push_back("not_reachable_from_entry");
        }
        if (!node.defaultVisible) {
            reasonCodes.push_back("hidden_in_default_view");
        }

        const ArchitectureRuleSeverity severity =
            fullyIsolated ? ArchitectureRuleSeverity::Warning : ArchitectureRuleSeverity::Info;

        findings.push_back(makeFinding(
            ArchitectureRuleKind::Isolated,
            severity,
            {node.id},
            std::move(reasonCodes)));
    }

    return findings;
}

}  // namespace

const char* toString(const ArchitectureRuleKind kind) {
    switch (kind) {
    case ArchitectureRuleKind::Cycle:
        return "cycle";
    case ArchitectureRuleKind::Hub:
        return "hub";
    case ArchitectureRuleKind::Isolated:
        return "isolated";
    default:
        return "unknown";
    }
}

const char* toString(const ArchitectureRuleSeverity severity) {
    switch (severity) {
    case ArchitectureRuleSeverity::Info:
        return "info";
    case ArchitectureRuleSeverity::Warning:
        return "warning";
    case ArchitectureRuleSeverity::Critical:
        return "critical";
    default:
        return "unknown";
    }
}

ArchitectureRuleReport evaluateArchitectureRules(
    const CapabilityGraph& graph,
    const ArchitectureOverview& overview) {
    ArchitectureRuleReport report;
    report.findings = evaluateCycleRules(graph, overview);

    auto hubFindings = evaluateHubRules(graph);
    report.findings.insert(
        report.findings.end(),
        std::make_move_iterator(hubFindings.begin()),
        std::make_move_iterator(hubFindings.end()));

    auto isolatedFindings = evaluateIsolatedRules(graph);
    report.findings.insert(
        report.findings.end(),
        std::make_move_iterator(isolatedFindings.begin()),
        std::make_move_iterator(isolatedFindings.end()));

    std::sort(
        report.findings.begin(),
        report.findings.end(),
        [](const ArchitectureRuleFinding& left, const ArchitectureRuleFinding& right) {
            const int leftSeverity = severityRank(left.severity);
            const int rightSeverity = severityRank(right.severity);
            if (leftSeverity != rightSeverity) {
                return leftSeverity > rightSeverity;
            }
            if (left.kind != right.kind) {
                return static_cast<int>(left.kind) < static_cast<int>(right.kind);
            }
            if (left.nodeIds.size() != right.nodeIds.size()) {
                return left.nodeIds.size() > right.nodeIds.size();
            }
            return left.nodeIds < right.nodeIds;
        });

    report.diagnostics.push_back(
        "ArchitectureRuleEngine evaluated cycle, hub, and isolated rules for capability nodes.");
    return report;
}

}  // namespace savt::core
