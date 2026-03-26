const { scheduleController } = require('../controllers/scheduleController');

function buildScheduleRoutes() {
  return scheduleController();
}

module.exports = { buildScheduleRoutes };
