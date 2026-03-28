package com.example.inventory.service;

import com.example.inventory.model.InventoryItem;
import com.example.inventory.repository.InventoryRepository;
import org.springframework.stereotype.Service;

@Service
public class InventoryService {
  private final InventoryRepository inventoryRepository;
  private InventoryItem latestItem;

  public InventoryService(InventoryRepository inventoryRepository) {
    this.inventoryRepository = inventoryRepository;
  }
}
