add_library(websocket_client
  websocket_client.cpp
  websocket_client.h
  websocket_manager.cpp
  websocket_manager.h
)

target_include_directories(websocket_client PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(websocket_client PUBLIC ${LIBWEBSOCKETS_LIBRARIES}) 