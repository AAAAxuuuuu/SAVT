#include "interaction/http/ApiController.h"

#include "core/service/ScheduleService.h"

int ApiController::handle() const {
    ScheduleService service;
    return service.load();
}
