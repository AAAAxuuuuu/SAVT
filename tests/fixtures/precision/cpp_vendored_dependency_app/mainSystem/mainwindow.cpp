#include "mainwindow.h"

#include "QXlsx-1.5.0/QXlsx/header/xlsxdocument.h"

void MainWindow::show() {
    QXlsxDocument document;
    document.save();
}
