package com.example.inventory.repository;

import com.example.inventory.model.InventoryItem;
import org.springframework.stereotype.Repository;

@Repository
public class InventoryRepository {
  private InventoryItem cachedItem;
}
