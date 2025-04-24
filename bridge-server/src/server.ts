import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { WebSocketServer, WebSocket } from 'ws';
import { ServerMessage, PixelUpdate, Pixel, ClaudeAnalysis } from './types';
import { z } from "zod";
import * as readline from 'readline';
import * as fs from 'fs';
import * as path from 'path';

const WS_PORT = 8080;
const STORAGE_DIR = path.join(__dirname, '..', 'storage');
const UPDATES_FILE = path.join(STORAGE_DIR, 'pixel_updates.json');
const LOG_FILE = path.join(STORAGE_DIR, 'server.log');

// Ensure storage directory exists
if (!fs.existsSync(STORAGE_DIR)) {
  fs.mkdirSync(STORAGE_DIR, { recursive: true });
}

// Initialize storage if it doesn't exist
if (!fs.existsSync(UPDATES_FILE)) {
  fs.writeFileSync(UPDATES_FILE, JSON.stringify({ updates: [] }, null, 2));
}

// Load existing updates
let savedUpdates: PixelUpdate[] = [];
try {
  const data = fs.readFileSync(UPDATES_FILE, 'utf8');
  const parsed = JSON.parse(data);
  savedUpdates = parsed.updates || [];
  console.error(`Loaded ${savedUpdates.length} saved updates from storage`);
} catch (error) {
  console.error('Error loading saved updates:', error);
  savedUpdates = [];
}

// Store the last pixel update and analysis
let lastUpdate: PixelUpdate | null = savedUpdates[savedUpdates.length - 1] || null;
let lastAnalysis: ClaudeAnalysis | null = null;

// Helper function to log both to console and file
function logMessage(message: string) {
  const timestamp = new Date().toISOString();
  const logEntry = `[${timestamp}] ${message}\n`;
  fs.appendFileSync(LOG_FILE, logEntry);
  console.error(message);
}

// Function to save update to storage
function savePixelUpdate(update: PixelUpdate): void {
  try {
    // Add some basic validation
    if (!update || !Array.isArray(update.pixels) || update.pixels.length === 0) {
      logMessage('Invalid update data');
      return;
    }

    // Add timestamp if not present
    if (!update.timestamp) {
      update.timestamp = Date.now();
    }

    // Create storage directory if it doesn't exist
    if (!fs.existsSync(STORAGE_DIR)) {
      logMessage('Creating storage directory');
      fs.mkdirSync(STORAGE_DIR, { recursive: true });
    }

    // If file doesn't exist, create it with initial structure
    if (!fs.existsSync(UPDATES_FILE)) {
      logMessage('Creating new updates file');
      fs.writeFileSync(UPDATES_FILE, JSON.stringify({ updates: [update] }, null, 2));
      savedUpdates = [update]; // Update in-memory array
      return;
    }

    // Read the file content
    logMessage('Reading existing updates file');
    const fileContent = fs.readFileSync(UPDATES_FILE, 'utf8');
    
    try {
      // Parse existing content
      const data = JSON.parse(fileContent);
      
      // Add new update
      data.updates = data.updates || [];
      data.updates.push(update);
      
      // Write back to file
      logMessage(`Writing ${data.updates.length} updates to file`);
      fs.writeFileSync(UPDATES_FILE, JSON.stringify(data, null, 2));
      savedUpdates = data.updates; // Update in-memory array
      logMessage(`Successfully saved update with ${update.pixels.length} pixels`);
      
    } catch (parseError: any) {
      logMessage(`Error parsing existing updates file: ${parseError.message}`);
      // If parsing fails, create new file with just this update
      fs.writeFileSync(UPDATES_FILE, JSON.stringify({ updates: [update] }, null, 2));
      savedUpdates = [update]; // Update in-memory array
      logMessage('Created new updates file after parse error');
    }
    
  } catch (error: any) {
    logMessage(`Error in savePixelUpdate: ${error.message}`);
    if (error.stack) {
      logMessage(`Error stack: ${error.stack}`);
    }
    throw error;
  }
}

// Function to clear all saved drawings
function clearAllDrawings(): void {
  try {
    savedUpdates = [];
    fs.writeFileSync(UPDATES_FILE, JSON.stringify({ updates: [] }, null, 2));
    lastUpdate = null;
    lastAnalysis = null;
    console.error('Cleared all saved drawings from storage');
  } catch (error) {
    console.error('Error clearing drawings from storage:', error);
  }
}

// Create MCP server instance
const server = new McpServer({
  name: "drawing-analyzer",
  version: "1.0.0",
});

// Register the analyze-drawing tool
server.tool(
  "analyze-drawing",
  "Analyze pixel art drawings using a standard 2D coordinate system where (0,0) is at the top-left corner, with x increasing rightward and y increasing downward. Provide specific pattern recognition (e.g., 'stickman', 'chess piece', 'heart') rather than generic descriptions. The input comes from a sprite/pixel art editor where each pixel is deliberately placed for precise artwork creation.",
  {
    pixels: z.array(
      z.object({
        x: z.number().int(),
        y: z.number().int(),
        color: z.number()
      })
    ).describe("Array of non-transparent pixels with their exact integer coordinates (origin at top-left, x rightward, y downward) and RGB colors from the sprite"),
    width: z.number().min(1).describe("Total width of the sprite canvas in pixels (common sizes: 16, 32, 64, 128)"),
    height: z.number().min(1).describe("Total height of the sprite canvas in pixels (common sizes: 16, 32, 64, 128)")
  },
  async ({ pixels, width, height }) => {
    // Check if the provided pixel data is valid
    if (!pixels || pixels.length === 0) {
      return {
        content: [
          {
            type: "text",
            text: "No pixel data provided to analyze."
          }
        ]
      };
    }

    console.error(`Analyzing pixel art with ${pixels.length} pixels on ${width}x${height} canvas`);

    // Create an empty 2D array representing the canvas
    const canvas: string[][] = Array(height).fill(null).map(() => Array(width).fill(' ')); // Use space for empty cells

    // Fill the canvas array with a character for each pixel
    // Using '#' for any colored pixel, could be enhanced to use different characters for different colors
    pixels.forEach(pixel => {
      if (pixel.x >= 0 && pixel.x < width && pixel.y >= 0 && pixel.y < height) {
        // Basic check: is the pixel non-transparent (assuming alpha isn't stored/relevant)?
        // We might need a more sophisticated check based on the color format if 0 represents transparency.
        // For now, let's assume any listed pixel is visible.
        // A more robust approach might map colors to different characters or use block characters.
        canvas[pixel.y][pixel.x] = '#'; // Represent any pixel with '#'
      }
    });

    // Convert the 2D array canvas to a multi-line string
    const canvasString = canvas.map(row => row.join('')).join('\n');

    return {
      content: [
        {
          type: "text",
          text: `Drawing Analysis (${width}x${height} canvas):

` +
                `Visual Representation:
${canvasString}`
        }
      ]
    };
  }
);

// Register the list-saved-drawings tool
server.tool(
  "list-saved-drawings",
  "List all saved drawings in storage",
  {},
  async () => {
    const drawingsList = savedUpdates.map((update, index) => {
      return `Drawing ${index + 1}:\n` +
             `  Timestamp: ${new Date(update.timestamp).toLocaleString()}\n` +
             `  Pixels: ${update.pixels.length}\n` +
             `  Canvas: ${update.width}x${update.height}\n` +
             '  ---';
    }).join('\n');

    return {
      content: [
        {
          type: "text",
          text: savedUpdates.length > 0 
            ? `Found ${savedUpdates.length} saved drawings:\n\n${drawingsList}`            : "No saved drawings found."
        }
      ]
    };
  }
);

// Register the clear-all-drawings tool
server.tool(
  "clear-all-drawings",
  "Delete all saved drawings from storage",
  {},
  async () => {
    const count = savedUpdates.length;
    clearAllDrawings();
    
    return {
      content: [
        {
          type: "text",
          text: `Successfully deleted ${count} drawings from storage.`
        }
      ]
    };
  }
);

// Create WebSocket server for LibreSprite communication
const wss = new WebSocketServer({ 
  port: WS_PORT,
  maxPayload: 1024 * 1024 * 1024, // 1GB max payload
  perMessageDeflate: {
    zlibDeflateOptions: {
      chunkSize: 64 * 1024, // 64KB chunks
      memLevel: 9,
      level: 1  // Fastest compression
    },
    zlibInflateOptions: {
      chunkSize: 64 * 1024  // 64KB chunks
    },
    clientNoContextTakeover: false,  // Allow context reuse
    serverNoContextTakeover: false,  // Allow context reuse
    serverMaxWindowBits: 15,
    concurrencyLimit: 20,
    threshold: 8192 // Only compress messages larger than 8KB
  }
});

// Start both servers
async function startServers() {
  try {
    // Start MCP server
    const transport = new StdioServerTransport();
    await server.connect(transport);
    logMessage('MCP server started');

    // Start WebSocket server
    wss.on('connection', (ws) => {
      logMessage('New client connected');
      
      // Configure WebSocket
      ws.binaryType = 'arraybuffer';
      ws.setMaxListeners(20);

      ws.on('error', (error) => {
        logMessage(`WebSocket error: ${error.message}`);
      });

      ws.on('close', () => {
        logMessage('Client disconnected');
      });

      ws.on('message', async (data) => {
        try {
          // Debug logging for incoming data
          const rawMessage = data.toString();
          logMessage(`Received raw data length: ${rawMessage.length}`);
          logMessage(`First 200 chars of message: ${rawMessage.substring(0, 200)}...`);
          
          let message;
          try {
            message = JSON.parse(rawMessage);
            logMessage('Successfully parsed JSON message');
          } catch (parseError: any) {
            logMessage(`Parse error details: ${parseError.message}`);
            logMessage(`Failed message content: ${rawMessage}`);
            throw new Error(`Failed to parse message data: ${parseError.message}`);
          }
          
          // Validate the message format
          if (!message || typeof message !== 'object') {
            logMessage('Message is not an object');
            throw new Error('Invalid message format: not an object');
          }

          if (message.type !== 'pixel_update') {
            logMessage(`Invalid message type: ${message.type}`);
            throw new Error('Invalid message type');
          }

          if (!message.data) {
            logMessage('Message missing data field');
            throw new Error('Invalid message format: missing data');
          }

          const update = message.data;
          
          // Debug logging for update data
          logMessage(`Update data: pixels=${update.pixels?.length}, width=${update.width}, height=${update.height}`);
          
          // Validate the update data
          if (!Array.isArray(update.pixels)) {
            logMessage('Invalid pixels array');
            throw new Error('Invalid pixels array');
          }
          
          if (typeof update.width !== 'number' || typeof update.height !== 'number') {
            logMessage('Invalid canvas dimensions');
            throw new Error('Invalid canvas dimensions');
          }

          // Store the update first
          lastUpdate = update;
          logMessage(`Stored update with ${update.pixels.length} pixels`);

          // Then try to save it
          try {
            await new Promise((resolve) => setTimeout(resolve, 0)); // Give event loop a chance to breathe
            const beforeSave = Date.now();
            savePixelUpdate(update);
            const afterSave = Date.now();
            logMessage(`Save operation took ${afterSave - beforeSave}ms for ${update.pixels.length} pixels`);
          } catch (saveError: any) {
            logMessage(`Error saving update: ${saveError.message}`);
            logMessage(`Error stack: ${saveError.stack}`);
            throw saveError;
          }

          // Send acknowledgment back to client
          const ackMessage: ServerMessage = {
            type: 'info',
            data: `Successfully received and saved ${update.pixels.length} pixels`
          };
          ws.send(JSON.stringify(ackMessage));
          logMessage('Sent acknowledgment to client');

        } catch (error: any) {
          logMessage(`Error processing message: ${error.message}`);
          if (error.stack) {
            logMessage(`Error stack: ${error.stack}`);
          }
          const errorMessage: ServerMessage = {
            type: 'error',
            data: error.message
          };
          ws.send(JSON.stringify(errorMessage));
        }
      });
    });

    logMessage('WebSocket server started on port ' + WS_PORT);
  } catch (error: any) {
    logMessage(`Error starting servers: ${error.message}`);
    if (error.stack) {
      logMessage(`Error stack: ${error.stack}`);
    }
    process.exit(1);
  }
}

// Start both servers
startServers(); 
