const { readSchedule } = require('../stores/scheduleStore');

function scheduleService() {
  return readSchedule();
}

module.exports = { scheduleService };
