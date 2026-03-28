const indexService = require('../services/indexService');

function run() {
  return indexService.refreshIndex();
}

module.exports = { run };
