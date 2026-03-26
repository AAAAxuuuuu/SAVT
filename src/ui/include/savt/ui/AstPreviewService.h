#pragma once

#include "savt/core/ArchitectureGraph.h"

#include <QString>
#include <QVariantList>

namespace savt::ui {

struct AstPreviewData {
    QString title;
    QString summary;
    QString text;
};

class AstPreviewService {
public:
    static QVariantList buildAstFileItems(const savt::core::AnalysisReport& report);
    static QString chooseDefaultAstFilePath(const QVariantList& items);
    static AstPreviewData buildEmptyPreview();
    static AstPreviewData buildPreview(const QString& projectRootPath, const QString& relativePath);
};

}  // namespace savt::ui
