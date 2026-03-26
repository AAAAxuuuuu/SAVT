#pragma once

#include "../../backend/service/TaskService.h"

namespace App::Ui {

class MainWindow {
public:
    int show(App::Backend::TaskService& service);
};

}  // namespace App::Ui
