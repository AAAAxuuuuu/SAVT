#include "QXlsx-1.5.0/QXlsx/header/xlsxdocument.h"

int main() {
    QXlsxDocument document;
    return document.save() ? 0 : 1;
}
