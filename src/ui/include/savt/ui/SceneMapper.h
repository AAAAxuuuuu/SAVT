#pragma once

#include "savt/core/CapabilityGraph.h"
#include "savt/core/ComponentGraph.h"
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

struct ComponentSceneData {
    QVariantList nodeItems;
    QVariantList edgeItems;
    QVariantList groupItems;
    QVariantList diagnosticItems;
    QString title;
    QString summary;
    double sceneWidth = 0.0;
    double sceneHeight = 0.0;
};

class SceneMapper {
public:
    static QVariantMap toVariantMap(const CapabilitySceneData& scene);
    static QVariantMap toVariantMap(const ComponentSceneData& scene);
    static CapabilitySceneData buildCapabilitySceneData(
        const savt::core::CapabilityGraph& graph,
        const savt::layout::CapabilitySceneLayoutResult& layout);
    static ComponentSceneData buildComponentSceneData(
        const savt::core::ComponentGraph& graph,
        const savt::layout::ComponentSceneLayoutResult& layout);
};

}  // namespace savt::ui
