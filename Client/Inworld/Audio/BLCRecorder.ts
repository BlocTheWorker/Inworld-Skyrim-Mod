import { InworldConnectionService } from '@inworld/nodejs-sdk';
import WebSocket from 'ws';

interface Console {
  justLog: any
}

export class BLCRecorder {
  private socket: WebSocket;
  constructor(private host: string, private port: number) {
  }

  connect(connection: InworldConnectionService): Promise<void> {
    return new Promise((resolve, reject) => {
      try{
        this.socket = new WebSocket(`ws://${this.host}:${this.port}`, {  perMessageDeflate: false});

        this.socket.on('open', () => {
          console.log(`(Audio Bus) Connected to ${this.host}:${this.port}`);
          (console as any).logToLog(`(Audio Bus) Connected to ${this.host}:${this.port}`)
          resolve();
        });

        this.socket.on('message', async function message(data) {
          console.log("Getting voice data...")
          await connection.sendAudio(data);
        });

        this.socket.on('error',(err) => {
          console.error(`Error connecting to ${this.host}:${this.port}: ${err}`);
          reject(err);
        });
      } catch(e) {
        reject("Ops! " + e);
      }
    });
  }

  start(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.socket.send("start");
      resolve();
    });
  }

  stop(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.socket.send("stop");
      resolve();
    });
  }

  exit(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.socket.send("exit");
      resolve();
    });
  }

  playChunk(data: string): Promise<void> {
    return new Promise((resolve, reject) => {
      this.socket.send('play_audio;;' + data);
      resolve();
    });
  }

  releaseChunks(){
    this.socket.send('release_audio');
  }

  stopAudio(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.socket.send('stop_playing');resolve();
    });
  }
}