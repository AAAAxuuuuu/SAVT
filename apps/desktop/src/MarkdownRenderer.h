#pragma once

#include <QObject>
#include <QString>

class MarkdownRenderer : public QObject {
    Q_OBJECT

public:
    explicit MarkdownRenderer(QObject* parent = nullptr);

    Q_INVOKABLE QString toHtml(const QString& markdown) const;
};
