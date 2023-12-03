// @ts-check
import {InworldClient, InworldConnectionService} from '@inworld/nodejs-sdk';
import InworldWorkspaceManager from './InworldWorkspaceManager.js';
import {BLCRecorder} from './Audio/BLCRecorder.js';
import {SkyrimInworldSocketController,GetSocketResponse} from './SkyrimInworldSocketController.js';

const WORKSPACE_NAME = process.env.INWORLD_WORKSPACE;

const defaultConfigurationConnection = {
    autoReconnect: true,
    disconnectTimeout: 3600 * 60
}

export default class InworldClientManager {
    private connection : InworldConnectionService;
    private client : InworldClient;
    private IsConnected : boolean;
    private workspaceManager : InworldWorkspaceManager;
    private blcRecorder : BLCRecorder;
    private socketController : SkyrimInworldSocketController;
    private isVoiceConnected = false;
    private isAudioSessionStarted = false;
    currentCapabilities = {
        audio: true,
        emotions: true,
        phonemes: true
    }

    constructor() { 
        this.SetupWorkspaceAndClient();
    }

    async SetupWorkspaceAndClient() {
        this.workspaceManager = new InworldWorkspaceManager();
        this.CreateClient();
    }

    // Socket version of connection
    async ConnectToCharacterViaSocket(characterId : string, playerName : string, socket : WebSocket) {
        try {
            let id = this.workspaceManager.GetCharacterIdentifier(characterId);
            console.log(`Requested to talk with ${characterId} which corresponds to ${id} on database.`);
            (console as any).logToLog(`Requested to talk with ${characterId} which corresponds to ${id} on database.`)
            if (!id) {
                let errorResult = `Cannot connect to ${id} or ${characterId}`;
                throw errorResult
            }
            console.log("Requesting connecting to " + id);
            let scene = "workspaces/" + WORKSPACE_NAME + "/characters/{CHARACTER_NAME}".replace("{CHARACTER_NAME}", id);
            this.client.setUser({fullName: playerName});
            this.client.setScene(scene);

            this.socketController = new SkyrimInworldSocketController(socket);
            this.client.setOnMessage((dat : any) => this.socketController.ProcessMessage(dat));

            this.client.setOnError((err) => {
                if (err.code != 10 && err.code != 1) 
                    console.error(err);
            });
            this.connection = this.client.build();
            this.IsConnected = true;
            if (!this.isVoiceConnected) {
                console.log("Creating voice listener connection");
                let port = parseInt(process.env.AUDIO_PORT);
                this.blcRecorder = new BLCRecorder("127.0.0.1", port);
                this.blcRecorder.connect(this.connection);
            }
            this.socketController.SetRecorder(this.blcRecorder);

            let verifyConnection = GetSocketResponse("connection established", "1-1", "established");
            console.log("Connection to " + id + " is succesfull");
            (console as any).logToLog(`Connection to ${id} is succesfull.`)
            await this.connection.sendAudioSessionStart();
            this.isAudioSessionStarted = true;
            socket.send(JSON.stringify(verifyConnection));
        } catch(err) {
            console.error(err);
            let returnDoesNotExist = GetSocketResponse("This soul lacks the divine blessing of conversational endowment bestowed by the gods.", "1-1", "doesntexist");
            socket.send(JSON.stringify(returnDoesNotExist));
        }
    }

    Say(message : string) {
        if (this.IsConnected) {
            this.connection.sendText(message);
        }
    }

    async StartTalking() {
        if (!this.isAudioSessionStarted) {
            await this.connection.sendAudioSessionStart();
            this.isAudioSessionStarted = true;
        }
        await this.blcRecorder.start();
    }

    async StopTalking() {
        setTimeout(async () => {
            await this.blcRecorder.stop();
            await this.connection.sendAudioSessionEnd();
            this.isAudioSessionStarted = false;
            this.socketController.SendUserVoiceInput();
        }, 500)
    }

    CreateClient() {
        this.client = new InworldClient();
        this.client.setApiKey({key: process.env.INWORLD_KEY as string, secret: process.env.INWORLD_SECRET as string});
        this.SetConfiguration();
    }

    SetConfiguration() {
        this.client.setConfiguration({connection: defaultConfigurationConnection, capabilities: this.currentCapabilities});
    }
}
