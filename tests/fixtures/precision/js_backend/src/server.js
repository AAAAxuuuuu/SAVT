const express = require('express');
const scheduleRoutes = require('./routes/schedule');

function bootstrapServer() {
  return express();
}

module.exports = { bootstrapServer, scheduleRoutes };
