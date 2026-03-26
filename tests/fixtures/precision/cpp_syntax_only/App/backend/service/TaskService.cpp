#include "TaskService.h"

namespace App::Backend {

Task TaskService::current() const {
    return Task{nextId()};
}

int TaskService::nextId() const {
    return 7;
}

}  // namespace App::Backend
