#include "websocket_client.h"
#include <libwebsockets.h>
#include <sstream>
#include <ctime>
#include <thread>
#include <chrono>
#include <iostream>

namespace app {

static WebSocketClient* g_client = nullptr;

static struct lws_protocols protocols[] = {
  {
    "pixel-protocol",
    WebSocketClient::websocketCallback,
    0,
    64 * 1024,  // RX buffer size: 64KB
  },
  { NULL, NULL, 0, 0 }
};

WebSocketClient::WebSocketClient()
  : m_connected(false)
  , m_ws_context(nullptr)
  , m_ws_client(nullptr)
{
  g_client = this;
}

WebSocketClient::~WebSocketClient()
{
  disconnect();
  g_client = nullptr;
}

bool WebSocketClient::connect(const std::string& url)
{
  if (m_connected)
    return true;

  std::cout << "Attempting to connect to WebSocket server..." << std::endl;

  // Initialize libwebsockets context
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  info.fd_limit_per_thread = 1024;
  info.ka_time = 30;  // Keep-alive timeout in seconds
  info.ka_interval = 10;  // Keep-alive interval in seconds
  info.ka_probes = 3;  // Number of keep-alive probes

  m_ws_context = lws_create_context(&info);
  if (!m_ws_context) {
    std::cerr << "Failed to create WebSocket context" << std::endl;
    return false;
  }

  std::cout << "WebSocket context created successfully" << std::endl;

  // Create client connection
  struct lws_client_connect_info ccinfo;
  memset(&ccinfo, 0, sizeof(ccinfo));
  
  ccinfo.context = static_cast<lws_context*>(m_ws_context);
  ccinfo.address = "localhost";
  ccinfo.port = 8080;
  ccinfo.path = "/";
  ccinfo.host = ccinfo.address;
  ccinfo.origin = ccinfo.address;
  ccinfo.protocol = protocols[0].name;
  ccinfo.userdata = this;
  ccinfo.ssl_connection = 0;

  std::cout << "Connecting to " << ccinfo.address << ":" << ccinfo.port << ccinfo.path << std::endl;

  m_ws_client = lws_client_connect_via_info(&ccinfo);
  if (!m_ws_client) {
    std::cerr << "Failed to connect to WebSocket server" << std::endl;
    lws_context_destroy(static_cast<lws_context*>(m_ws_context));
    m_ws_context = nullptr;
    return false;
  }

  std::cout << "Connection initiated, waiting for establishment..." << std::endl;

  // Run the event loop for a short time to establish the connection
  int retries = 0;
  const int MAX_RETRIES = 20;  // 2 seconds total
  while (!m_connected && retries < MAX_RETRIES) {
    lws_service(static_cast<lws_context*>(m_ws_context), 100);  // 100ms timeout
    retries++;
    if (!m_connected) {
      std::cout << "Waiting for connection... attempt " << retries << "/" << MAX_RETRIES << std::endl;
    }
  }

  if (!m_connected) {
    std::cerr << "Failed to establish connection after " << MAX_RETRIES << " attempts" << std::endl;
    disconnect();
    return false;
  }

  std::cout << "WebSocket connection established successfully" << std::endl;
  return true;
}

void WebSocketClient::disconnect()
{
  if (m_ws_client) {
    lws_callback_on_writable(static_cast<lws*>(m_ws_client));
    m_ws_client = nullptr;
  }

  if (m_ws_context) {
    lws_context_destroy(static_cast<lws_context*>(m_ws_context));
    m_ws_context = nullptr;
  }

  m_connected = false;
}

bool WebSocketClient::sendPixelUpdate(const std::vector<Pixel>& pixels, int width, int height)
{
  if (!m_connected || !m_ws_client)
    return false;

  // Create a JSON object with the required format
  std::stringstream ss;
  ss << "{\"type\":\"pixel_update\",\"data\":{\"pixels\":[";
  
  // Send pixels in chunks to avoid buffer overflow
  const size_t CHUNK_SIZE = 1000;  // Process 1000 pixels at a time
  for (size_t i = 0; i < pixels.size(); ++i) {
    if (i > 0) ss << ",";
    ss << "{\"x\":" << pixels[i].x
       << ",\"y\":" << pixels[i].y
       << ",\"color\":" << pixels[i].color
       << "}";

    // If we've reached a chunk boundary or this is the last pixel,
    // send the current chunk
    if ((i + 1) % CHUNK_SIZE == 0 || i == pixels.size() - 1) {
      if (i == pixels.size() - 1) {
        // Add the closing brackets for the last chunk
        ss << "],\"width\":" << width
           << ",\"height\":" << height
           << "}}";
      }

      std::string message = ss.str();
      std::cout << "Sending chunk of size: " << message.size() << " bytes" << std::endl;

      // Prepare the message with LWS_PRE padding
      unsigned char* buf = (unsigned char*)malloc(LWS_PRE + message.size());
      if (!buf) {
        std::cerr << "Failed to allocate memory for WebSocket message" << std::endl;
        return false;
      }

      memcpy(buf + LWS_PRE, message.c_str(), message.size());

      // Send the message
      int ret = lws_write(static_cast<lws*>(m_ws_client), buf + LWS_PRE, message.size(), LWS_WRITE_TEXT);
      free(buf);

      if (ret < 0) {
        std::cerr << "Failed to send WebSocket message" << std::endl;
        return false;
      }

      std::cout << "Successfully sent " << ret << " bytes" << std::endl;

      // Clear the stringstream for the next chunk
      if (i < pixels.size() - 1) {
        ss.str("");
        ss.clear();
        ss << "{\"type\":\"pixel_update\",\"data\":{\"pixels\":[";
      }

      // Run the event loop to process the send
      lws_service(static_cast<lws_context*>(m_ws_context), 0);

      // Add a small delay between chunks to prevent overwhelming the connection
      if (i < pixels.size() - 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  }

  return true;
}

int WebSocketClient::websocketCallback(struct lws* wsi,
                                     enum lws_callback_reasons reason,
                                     void* user, void* in, size_t len)
{
  std::cout << "WebSocket callback reason: " << reason << std::endl;
  
  switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      std::cout << "WebSocket connection established" << std::endl;
      if (g_client)
        g_client->m_connected = true;
      break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      std::cerr << "WebSocket connection error: " << (in ? (char*)in : "unknown error") << std::endl;
      if (g_client)
        g_client->m_connected = false;
      break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
      if (in && len > 0) {
        std::string message((char*)in, len);
        std::cout << "Received message from server: " << message << std::endl;
      }
      break;

    case LWS_CALLBACK_CLIENT_CLOSED:
      std::cout << "WebSocket connection closed" << std::endl;
      if (g_client)
        g_client->m_connected = false;
      break;
  }

  return 0;
}

} // namespace app 