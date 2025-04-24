#include "websocket_client.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
  app::WebSocketClient client;
  
  std::cout << "Connecting to WebSocket server..." << std::endl;
  if (!client.connect("ws://localhost:8080")) {
    std::cerr << "Failed to connect to WebSocket server" << std::endl;
    return 1;
  }
  
  std::cout << "Connected successfully!" << std::endl;
  
  // Create some test pixels
  std::vector<app::Pixel> pixels = {
    {100, 100, 0xFF0000},  // Red pixel
    {101, 100, 0xFF0000},  // Red pixel
    {102, 100, 0xFF0000},  // Red pixel
    {100, 101, 0x00FF00},  // Green pixel
    {101, 101, 0x00FF00},  // Green pixel
    {102, 101, 0x00FF00}   // Green pixel
  };
  
  std::cout << "Sending test pixels..." << std::endl;
  if (!client.sendPixelUpdate(pixels, 200, 200)) {
    std::cerr << "Failed to send pixel update" << std::endl;
    return 1;
  }
  
  std::cout << "Pixel update sent successfully!" << std::endl;
  
  // Keep the connection alive for a few seconds
  std::this_thread::sleep_for(std::chrono::seconds(5));
  
  client.disconnect();
  std::cout << "Disconnected from server" << std::endl;
  
  return 0;
} 