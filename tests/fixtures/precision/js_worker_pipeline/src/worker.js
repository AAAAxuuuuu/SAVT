const rebuildIndexJob = require('./jobs/rebuildIndexJob');

function bootstrapWorker() {
  return rebuildIndexJob.run();
}

module.exports = { bootstrapWorker };
