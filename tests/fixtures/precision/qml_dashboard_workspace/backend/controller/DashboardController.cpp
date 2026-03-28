#include "backend/controller/DashboardController.h"

#include "backend/service/MetricService.h"

int DashboardController::refresh() const {
    MetricService service;
    return service.load();
}
