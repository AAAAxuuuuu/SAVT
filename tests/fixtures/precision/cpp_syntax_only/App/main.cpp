#include "backend/service/TaskService.h"
#include "frontend/view/MainWindow.h"

int main() {
    App::Backend::TaskService service;
    App::Ui::MainWindow window;
    return window.show(service);
}
