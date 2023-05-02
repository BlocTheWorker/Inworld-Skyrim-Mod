// @ts-check
import {
    InworldClient
} from '@inworld/nodejs-sdk';
import InworldWorkspaceManager from './InworldWorkspaceManager';

const WORKSPACE_NAME = process.env.INWORLD_WORKSPACE;

const defaultConfigurationConnection = {
    autoReconnect: true,
    disconnectTimeout: 3600 * 60
}

export default class InworldClientManager {
    private connection : any;
    private client: InworldClient;
    private IsConnected: boolean;
    private workspaceManager: InworldWorkspaceManager;

    currentCapabilities = {
        audio: true,
        emotions: true
    }

    constructor() {
        // Somewhere here create hand crafted characters from list
        this.SetupWorkspaceAndClient();
    }

    async SetupWorkspaceAndClient(){
        this.workspaceManager = new InworldWorkspaceManager();
        this.CreateClient();
    }

    ConnectToCharacter(characterId:string, playerName:string, callback:any) {
        let id = this.workspaceManager.GetCharacterIdentifier(characterId);
        if(!id) {
            console.log(`Cannot connect to ${id}`);
            return;
        }
        console.log("Requesting connecting to " + id);
        let scene = "workspaces/" + WORKSPACE_NAME + "/characters/{CHARACTER_NAME}".replace("{CHARACTER_NAME}", id);
        this.client.setUser({
            fullName: playerName
        })
        this.client.setScene(scene);
        this.client.setOnMessage(callback);
        this.client.setOnError((err) => {
            if (err.code != 10 && err.code != 1)
                console.error(err);
        });
        this.connection = this.client.build();
        this.IsConnected = true;
        return this.connection;
    }

    Say(message: string){
        if(this.IsConnected) {
            this.connection.sendText(message);
        }
    }

    CreateClient() {
        this.client = new InworldClient();
        this.client.setApiKey({
            key: process.env.INWORLD_KEY as string,
            secret: process.env.INWORLD_SECRET as string,
        });
        this.SetConfiguration();
    }

    SetConfiguration() {
        this.client.setConfiguration({
            connection: defaultConfigurationConnection,
            capabilities: this.currentCapabilities
        });
    }


}