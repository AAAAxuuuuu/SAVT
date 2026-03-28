#include "ui/GraphWindow.h"

#include "lib/graph/GraphBuilder.h"

int GraphWindow::nodeCount() const {
    GraphBuilder builder;
    return builder.build();
}
