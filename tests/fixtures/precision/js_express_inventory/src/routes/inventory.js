const inventoryController = require('../controllers/inventoryController');

function buildInventoryRoutes(app) {
  return inventoryController.listInventory(app);
}

module.exports = { buildInventoryRoutes };
