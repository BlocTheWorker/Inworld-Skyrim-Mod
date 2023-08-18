import axios from "axios";
import InworldLoginSystem from "./InworldLoginSystem.js";
import path from "path";
import * as fs from 'fs';
const ALL_CHARACTERS = JSON.parse(fs.readFileSync(path.resolve("./World/SkyrimCharacters.json"), 'utf-8'));
const SKYRIM_KNOWLEDGE = JSON.parse(fs.readFileSync(path.resolve("./World/SkyrimKnowledge.json"), 'utf-8'));

const WORKSPACE_NAME = process.env.INWORLD_WORKSPACE;
const SHARED_KNOWLEDGE_URL: string = "https://studio.inworld.ai/v1alpha/workspaces/" + WORKSPACE_NAME + "/common-knowledges?pageSize=500"
const CREATE_URI = "https://studio.inworld.ai/v1alpha/workspaces/" + WORKSPACE_NAME + "/characters?skipAutoCreate=true";
const GET_CHARACTERS = "https://studio.inworld.ai/v1alpha/workspaces/" + WORKSPACE_NAME + "/characters";

export default class InworldWorkspaceManager {
    private loginManager;
    private characterList;
    private SharedKnowledge;
    private waitingCharacters : Array <string>;

    constructor() {
        this.loginManager = new InworldLoginSystem();
        this.SetupWorkplace();
    }

    private async SetupWorkplace() {
        await this.SetupCommonKnowledge();
        await this.PopulateCharacters();
        await this.CreateMissingCharacters();
        // refresh
        await this.PopulateCharacters();
    }

    private GetNameFromPath(path : string){
        let arr = path.split("/")
        return arr[arr.length-1]
    }

    private async CreateMissingCharacters() {
        this.waitingCharacters = [];
        let expectedList = (ALL_CHARACTERS as any).characters;
        for (let i = 0; i < expectedList.length; i++) {
            let data = expectedList[i];
            let isExist = false;
            for (let k = 0; k < this.characterList.length; k++) {
                let charData = this.characterList[k];
                let charName = this.GetNameFromPath(charData.name);
                let expectedName =  this.GetNameFromPath(data.name); // data.name.replace("{WORKSPACE}", WORKSPACE_NAME);
                if (expectedName.toLowerCase() == charName.toLowerCase()) {
                    isExist = true;
                }

                if (isExist) 
                    break;
            }

            if (!isExist) {
                console.log(`${data.defaultCharacterDescription.givenName} does not exist. I'm requesting to create it.`);
                (console as any).logToLog(`${data.defaultCharacterDescription.givenName} does not exist. I'm requesting to create it.`)
                this.CreateCharacter(data.name, data);
            } else {
                console.log(`${
                    data.defaultCharacterDescription.givenName
                } exists, not updating.`);
                (console as any).logToLog(`${data.defaultCharacterDescription.givenName} exists, not updating.`)
            }
        }

    }

    private removeItem<T>(arr : Array < T >, value : T): Array < T > {
        const index = arr.indexOf(value);
        if (index > -1) {
            arr.splice(index, 1);
        }
        return arr;
    }

    private async CreateCharacter(name, data) {
        await this.CreateNewCharacter(data);
        this.waitingCharacters = this.removeItem(this.waitingCharacters, name);
        console.log(`${data.defaultCharacterDescription.givenName} is now ready to use.`);
        (console as any).logToLog(`${data.defaultCharacterDescription.givenName} is now ready to use.`)
    }

    private async PopulateCharacters() {
        let headers = await this.GetHeader();
        let response = await axios.get(GET_CHARACTERS, {headers: headers});
        this.characterList = response.data.characters;
    }

    private internalDelay(ms : number): Promise < void > {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    private async CreateNewCharacter(characterData) {
        try {
            let headers = await this.GetHeader();
            delete characterData.safetyConfig;
            characterData.commonKnowledges = [];
            characterData.name = characterData.name.replace("{WORKSPACE}", WORKSPACE_NAME);
            let response = await axios.post(CREATE_URI, JSON.stringify(characterData), {headers: headers});

            let checkUri = "https://studio.inworld.ai/v1alpha/" + response.data.name

            let isDone = false;
            let nameFetched = ""
            while (!isDone) {
                headers = await this.GetHeader();
                let checkData = await axios.get(checkUri, {headers: headers});
                isDone = checkData.data.done;

                if (isDone && !!checkData.data.response && !!checkData.data.response.name) {
                    nameFetched = checkData.data.response.name;
                    await this.internalDelay(500);
                } else {
                    isDone = false;
                    console.log(checkData)
                    await this.internalDelay(1000);
                }
            }

            characterData.name = nameFetched;
            this.SharedKnowledge.commonKnowledges.forEach(knowledge => {
                characterData.commonKnowledges.push(knowledge.name)
            });

            headers = await this.GetHeader();
            await axios.patch("https://studio.inworld.ai/v1alpha/" + characterData.name, JSON.stringify(characterData), {headers: headers});
        } catch (e) {
            console.error(e)
        }
    }

    async SetupCommonKnowledge() {
        let commonknowledgeList = await this.GetAllCurrentCommonKnowledges();
        let expectedList = (SKYRIM_KNOWLEDGE as any).list;
        for (let i = 0; i < expectedList.length; i++) {
            let singleKnowledge = expectedList[i];
            let filtered = commonknowledgeList.commonKnowledges.filter(el => el.displayName == singleKnowledge.displayName);
            if (filtered.length == 0) {
                console.log("Creating a new common knowledge");
                (console as any).logToLog("Creating a new common knowledge.")
                await this.CreateNewCommonKnowledge(singleKnowledge);
            }
        }
        this.SharedKnowledge = await this.GetAllCurrentCommonKnowledges();
        console.log("All common knowledge has been processed.");
        (console as any).logToLog("All common knowledge has been processed.")
        return;
    }

    async CreateNewCommonKnowledge(commonKnowledge) {
        let normalizedUri = SHARED_KNOWLEDGE_URL.replace("?pageSize=500", "");
        let header = await this.GetHeader(true);
        await axios.post(normalizedUri, commonKnowledge, {headers: header});
    }

    async GetAllCurrentCommonKnowledges() {
        let header = await this.GetHeader(true);
        let response = await axios.get(SHARED_KNOWLEDGE_URL, {headers: header});
        return response.data;
    }

    GetCharacterIdentifier(name: string) {
        try {
            if (name.toLowerCase().includes("guard")) 
                name = "guard";
            for (let i = 0; i < this.characterList.length; i++) {
                let character = this.characterList[i];
                let nameNormalized = character.name.toLowerCase().replace("_", " ");
                if (nameNormalized.includes(name.toLowerCase()) || character.defaultCharacterDescription.givenName.toLowerCase().includes(name.toLowerCase())) {
                    let name = character.name;
                    let id = name.replace("workspaces/" + WORKSPACE_NAME + "/characters/", "")
                    return id;
                }
            }
        } catch {
            
        }
        return null;
}

GetAllCharacterName() {
    let names = [];
    for (let i = 0; i < this.characterList.length; i++) {
        let character = this.characterList[i];
        names.push(character.defaultCharacterDescription.givenName as never);
    }
    return names;
}

private async GetHeader(isKnowledge : boolean = false) {
    let token = await this.loginManager.GetTokenDirectly();
    let headerConfig = {
        'authorization': 'Bearer ' + token,
        'content-type': 'text/plain;charset=UTF-8',
        'origin': 'https://studio.inworld.ai',
        'referer': 'https://studio.inworld.ai/workspaces/' + WORKSPACE_NAME + (!isKnowledge ? '/characters' : '/knowledge'),
        'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.0.0 Safari/537.36'
    }
    return headerConfig;
}}
