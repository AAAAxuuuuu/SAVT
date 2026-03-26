#include "TrafficBackend.h"

namespace App::Backend {

Station TrafficBackend::currentStation() const {
    return Station{refresh()};
}

int TrafficBackend::refresh() const {
    return 12;
}

}  // namespace App::Backend
