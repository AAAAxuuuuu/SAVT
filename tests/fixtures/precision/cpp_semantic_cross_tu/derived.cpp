#include "model.h"

int Derived::run() {
    return Base::run();
}

Box<int>* makeBox() {
    return nullptr;
}
