export interface Pixel {
  x: number;
  y: number;
  color: number;
}

export interface PixelUpdate {
  pixels: Pixel[];
  timestamp: number;
  width: number;
  height: number;
}

export interface ClaudeAnalysis {
  description: string;
  confidence: number;
  timestamp: number;
}

export type ServerMessageType = 'pixel_update' | 'error' | 'info' | 'analysis';

export interface ServerMessage {
  type: ServerMessageType;
  data: PixelUpdate | ClaudeAnalysis | string;
}

export interface MCPMessage {
  role: 'user' | 'assistant';
  content: string;
  timestamp: number;
} 