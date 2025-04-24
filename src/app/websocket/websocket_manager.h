#pragma once

#include "websocket_client.h"
#include <memory>
#include <mutex>

namespace app {

class WebSocketManager {
public:
  static WebSocketManager& instance();

  // Initialize the WebSocket connection
  bool initialize();

  // Clean up the WebSocket connection
  void shutdown();

  // Send pixel updates
  bool sendPixelUpdate(const std::vector<Pixel>& pixels, int width, int height);

  // Check connection status
  bool isConnected() const;

  ~WebSocketManager();

private:
  WebSocketManager();

  // Prevent copying
  WebSocketManager(const WebSocketManager&) = delete;
  WebSocketManager& operator=(const WebSocketManager&) = delete;

  static std::unique_ptr<WebSocketManager> s_instance;
  static std::mutex s_mutex;

  std::unique_ptr<WebSocketClient> m_client;
  bool m_initialized;
};

} // namespace app 