const searchStore = require('../stores/searchStore');

function refreshIndex() {
  return searchStore.writeIndex();
}

module.exports = { refreshIndex };
