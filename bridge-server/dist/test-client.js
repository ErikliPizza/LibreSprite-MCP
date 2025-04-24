"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const ws_1 = __importDefault(require("ws"));
const ws = new ws_1.default('ws://localhost:8080');
ws.on('open', () => {
    console.log('Connected to server');
    // Send a test pixel update
    const update = {
        pixels: [
            { x: 12, y: 8, color: 16711680 }, // Blue pixel at (12,8)
            { x: 13, y: 8, color: 16711680 }, // Another blue pixel
            { x: 14, y: 8, color: 16711680 } // And another one
        ],
        timestamp: Date.now(),
        width: 32,
        height: 32
    };
    ws.send(JSON.stringify(update));
    console.log('Sent test update:', update);
});
ws.on('message', (data) => {
    const message = JSON.parse(data.toString());
    console.log('Received message:', message);
});
ws.on('error', (error) => {
    console.error('WebSocket error:', error);
});
ws.on('close', () => {
    console.log('Disconnected from server');
});
