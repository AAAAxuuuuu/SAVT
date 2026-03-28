#pragma once

struct GraphSpec {
    int nodes = 3;
};

class GraphBuilder {
public:
    int build() const {
        GraphSpec spec;
        return spec.nodes;
    }
};
