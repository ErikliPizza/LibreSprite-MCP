// Aseprite
// Copyright (C) 2001-2016  David Capello
// Copyright (C) 2021  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/document.h"
#include "doc/sprite.h"
#include "doc/layer.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "app/websocket/websocket_client.h"
#include "app/websocket/websocket_manager.h"
#include "app/ui/status_bar.h"
#include <sstream>
#include <iostream>

namespace app {

class SendToBridgeCommand : public Command {
public:
  SendToBridgeCommand();
  Command* clone() const override { return new SendToBridgeCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* ctx) override;
};

SendToBridgeCommand::SendToBridgeCommand()
  : Command("SendToBridge", "Send to Bridge", CmdUIOnlyFlag)
{
}

bool SendToBridgeCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::HasActiveSprite);
}

void SendToBridgeCommand::onExecute(Context* ctx)
{
  app::Document* doc = ctx->activeDocument();
  if (!doc) {
    StatusBar::instance()->showTip(1000, "No active document");
    return;
  }

  doc::Sprite* sprite = doc->sprite();
  if (!sprite) {
    StatusBar::instance()->showTip(1000, "No sprite in document");
    return;
  }

  // Get the first layer
  doc::Layer* layer = sprite->indexToLayer(sprite->firstLayer());
  if (!layer) {
    StatusBar::instance()->showTip(1000, "No layer found");
    return;
  }

  std::shared_ptr<doc::Cel> cel = layer->cel(0);
  if (!cel) {
    StatusBar::instance()->showTip(1000, "No cel found");
    return;
  }

  doc::Image* image = cel->image();
  if (!image) {
    StatusBar::instance()->showTip(1000, "No image found");
    return;
  }

  // Create a vector of pixels
  std::vector<Pixel> pixels;
  pixels.reserve(sprite->width() * sprite->height());

  std::cout << "Processing image: " << image->width() << "x" << image->height() << std::endl;
  int nonTransparentCount = 0;

  // Only send non-transparent pixels
  for (int y = 0; y < image->height(); ++y) {
    for (int x = 0; x < image->width(); ++x) {
      uint32_t color = image->getPixel(x, y);
      if (color != 0) { // Skip transparent pixels
        // Convert color from LibreSprite format to RGB format
        int r = doc::rgba_getr(color);
        int g = doc::rgba_getg(color);
        int b = doc::rgba_getb(color);
        uint32_t rgbColor = (r << 16) | (g << 8) | b;
        
        Pixel pixel;
        pixel.x = x;
        pixel.y = y;
        pixel.color = rgbColor;
        pixels.push_back(pixel);
        nonTransparentCount++;
      }
    }
  }

  std::cout << "Found " << nonTransparentCount << " non-transparent pixels" << std::endl;

  // Initialize WebSocket connection
  WebSocketManager& manager = WebSocketManager::instance();
  if (!manager.initialize()) {
    StatusBar::instance()->showTip(1000, "Failed to initialize WebSocket connection");
    return;
  }

  // Send the pixel data
  if (manager.sendPixelUpdate(pixels, sprite->width(), sprite->height())) {
    std::string msg = "Sent " + std::to_string(pixels.size()) + " pixels to bridge";
    StatusBar::instance()->showTip(1000, msg.c_str());
  } else {
    StatusBar::instance()->showTip(1000, "Failed to send pixel data to bridge");
    manager.shutdown();
  }

  // Always shutdown the connection when done
  manager.shutdown();
}

Command* CommandFactory::createSendToBridgeCommand()
{
  return new SendToBridgeCommand;
}

} // namespace app 