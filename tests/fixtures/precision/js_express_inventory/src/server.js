const inventoryRoutes = require('./routes/inventory');

function bootstrapServer() {
  return inventoryRoutes;
}

module.exports = { bootstrapServer };
