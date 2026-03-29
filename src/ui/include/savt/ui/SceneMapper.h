#pragma once

#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"

#include <QVariantList>
#include <QVariantMap>

namespace savt::ui {

struct CapabilitySceneData {
    QVariantList nodeItems;
    QVariantList edgeItems;
    QVariantList groupItems;
    double sceneWidth = 0.0;
    double sceneHeight = 0.0;
};

class SceneMapper {
public:
    static QVariantMap toVariantMap(const CapabilitySceneData& scene);
    static CapabilitySceneData buildCapabilitySceneData(
        const savt::core::CapabilityGraph& graph,
        const savt::layout::CapabilitySceneLayoutResult& layout);
};

}  // namespace savt::ui
