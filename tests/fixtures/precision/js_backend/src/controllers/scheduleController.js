const { scheduleService } = require('../services/scheduleService');

function scheduleController() {
  return scheduleService();
}

module.exports = { scheduleController };
