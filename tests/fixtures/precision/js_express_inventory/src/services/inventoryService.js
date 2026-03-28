const inventoryRepository = require('../repositories/inventoryRepository');

function loadInventory() {
  return inventoryRepository.readInventory();
}

module.exports = { loadInventory };
