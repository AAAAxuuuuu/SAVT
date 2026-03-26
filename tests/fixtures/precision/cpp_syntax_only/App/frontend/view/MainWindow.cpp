#include "MainWindow.h"

namespace App::Ui {

int MainWindow::show(App::Backend::TaskService& service) {
    return service.current().id;
}

}  // namespace App::Ui
