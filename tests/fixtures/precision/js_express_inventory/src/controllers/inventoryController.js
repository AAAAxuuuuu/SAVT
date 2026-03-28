const inventoryService = require('../services/inventoryService');

function listInventory() {
  return inventoryService.loadInventory();
}

module.exports = { listInventory };
