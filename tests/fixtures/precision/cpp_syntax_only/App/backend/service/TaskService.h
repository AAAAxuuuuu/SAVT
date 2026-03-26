#pragma once

#include "../model/Task.h"

namespace App::Backend {

class TaskService {
public:
    Task current() const;
    int nextId() const;
};

}  // namespace App::Backend
