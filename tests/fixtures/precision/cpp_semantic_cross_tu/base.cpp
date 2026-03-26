#include "model.h"

int Base::run() {
    return 1;
}

int callBase(Base& base) {
    return base.run();
}
