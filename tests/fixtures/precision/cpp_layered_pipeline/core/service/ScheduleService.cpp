#include "core/service/ScheduleService.h"

#include "core/storage/ScheduleRepository.h"

int ScheduleService::load() const {
    ScheduleRepository repository;
    return repository.fetch();
}
