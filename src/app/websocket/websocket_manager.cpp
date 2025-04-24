#include "websocket_manager.h"
#include <iostream>

namespace app {

std::unique_ptr<WebSocketManager> WebSocketManager::s_instance;
std::mutex WebSocketManager::s_mutex;

WebSocketManager& WebSocketManager::instance() {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (!s_instance) {
    s_instance.reset(new WebSocketManager());
  }
  return *s_instance;
}

WebSocketManager::WebSocketManager()
  : m_initialized(false)
{
  m_client = std::make_unique<WebSocketClient>();
}

WebSocketManager::~WebSocketManager() {
  shutdown();
}

bool WebSocketManager::initialize() {
  if (m_initialized)
    return true;

  std::cout << "Initializing WebSocket connection..." << std::endl;
  
  if (!m_client->connect("ws://localhost:8080")) {
    std::cerr << "Failed to connect to WebSocket server" << std::endl;
    return false;
  }

  m_initialized = true;
  std::cout << "WebSocket connection initialized" << std::endl;
  return true;
}

void WebSocketManager::shutdown() {
  if (!m_initialized)
    return;

  m_client->disconnect();
  m_initialized = false;
  std::cout << "WebSocket connection shut down" << std::endl;
}

bool WebSocketManager::sendPixelUpdate(const std::vector<Pixel>& pixels, int width, int height) {
  if (!m_initialized || !m_client)
    return false;

  return m_client->sendPixelUpdate(pixels, width, height);
}

bool WebSocketManager::isConnected() const {
  return m_initialized && m_client && m_client->isConnected();
}

} // namespace app 