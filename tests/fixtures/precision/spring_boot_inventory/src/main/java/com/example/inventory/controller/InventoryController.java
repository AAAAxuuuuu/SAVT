package com.example.inventory.controller;

import com.example.inventory.service.InventoryService;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class InventoryController {
  private final InventoryService inventoryService;

  public InventoryController(InventoryService inventoryService) {
    this.inventoryService = inventoryService;
  }
}
