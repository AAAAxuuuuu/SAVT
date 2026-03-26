#pragma once

#include "../model/Station.h"

namespace App::Backend {

class TrafficBackend {
public:
    Station currentStation() const;
    int refresh() const;
};

}  // namespace App::Backend
