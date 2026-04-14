#pragma once

#include <QString>
#include <QVariantMap>

namespace savt::ui {

class FileInsightService {
public:
    static bool isSingleFileNode(const QVariantMap& nodeData);
    static QString primaryFilePath(const QVariantMap& nodeData);
    static QVariantMap buildDetail(
        const QString& projectRootPath,
        const QVariantMap& nodeData);
};

}  // namespace savt::ui
