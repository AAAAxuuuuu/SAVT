#include "MarkdownRenderer.h"

#include <QTextDocument>

MarkdownRenderer::MarkdownRenderer(QObject* parent)
    : QObject(parent) {}

QString MarkdownRenderer::toHtml(const QString& markdown) const {
    QTextDocument document;
    document.setMarkdown(markdown);
    return document.toHtml();
}
