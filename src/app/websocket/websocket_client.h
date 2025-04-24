#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <libwebsockets.h>

struct lws;
enum lws_callback_reasons;

namespace app {

struct Pixel {
  int x;
  int y;
  uint32_t color;
};

class WebSocketClient {
public:
  WebSocketClient();
  ~WebSocketClient();

  // Connect to the WebSocket server
  bool connect(const std::string& url);

  // Disconnect from the server
  void disconnect();

  // Send pixel updates to the server
  bool sendPixelUpdate(const std::vector<Pixel>& pixels, int width, int height);

  // Check if connected
  bool isConnected() const { return m_connected; }

  // Internal WebSocket callback
  static int websocketCallback(struct lws* wsi,
                             enum lws_callback_reasons reason,
                             void* user, void* in, size_t len);

private:
  bool m_connected;
  void* m_ws_context;  // Will be cast to lws_context*
  void* m_ws_client;   // Will be cast to lws*

  friend int websocketCallback(struct lws* wsi,
                             enum lws_callback_reasons reason,
                             void* user, void* in, size_t len);
};

} // namespace app 