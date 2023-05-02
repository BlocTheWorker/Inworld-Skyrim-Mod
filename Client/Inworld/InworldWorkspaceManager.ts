import axios from "axios";
import InworldLoginSystem from "./InworldLoginSystem";

import * as ALL_CHARACTERS from "../Templates/WorldBuilding/SkyrimCharacters.json";
import * as SKYRIM_KNOWLEDGE from "../Templates/WorldBuilding/SkyrimKnowledge.json";

const WORKSPACE_NAME = process.env.INWORLD_WORKSPACE;
const SHARED_KNOWLEDGE_URL : string = "https://studio.inworld.ai/v1alpha/workspaces/" + WORKSPACE_NAME + "/common-knowledges?pageSize=500"
const CREATE_URI = "https://studio.inworld.ai/v1alpha/workspaces/" + WORKSPACE_NAME + "/characters?skipAutoCreate=false";
const GET_CHARACTERS = "https://studio.inworld.ai/v1alpha/workspaces/" + WORKSPACE_NAME + "/characters";

const INITIAL_TEMPLATE = {"languageCode":"en","defaultCharacterAssets":{"rpmImageUri":"","rpmModelUri":"","voice":{"baseName":"Masculine - US - Alex","gender":"VOICE_GENDER_MALE","pitch":0,"speakingRate":1,"ttsType":"TTS_TYPE_ADVANCED","roboticVoiceFilterLevel":0}},"defaultCharacterDescription":{"givenName":"Test","description":"","narrativeActionsEnabled":true},"brainSettings":{"conversationConfig":{"emotionsDisabled":false,"fillerSpec":{"fillersDisabled":true},"behavioralContextsDisabled":false}},"initialMood":{"joy":0,"fear":0,"trust":0,"surprise":0},"personality":{"positive":0,"peaceful":0,"open":0,"extravert":0},"emotionalFluidity":0.5,"isAutoGenerate":true,"brainTriggers":[]}

export default class InworldWorkspaceManager {
    private loginManager;
    private characterList;
    private SharedKnowledge;

    constructor(){
        this.loginManager = new InworldLoginSystem();
        this.SetupWorkplace();
    }

    private async SetupWorkplace(){
        await this.SetupCommonKnowledge();
        await this.PopulateCharacters();
        await this.CreateMissingCharacters();
        // refresh
        await this.PopulateCharacters();
    }

    private async CreateMissingCharacters(){
        let expectedList = ALL_CHARACTERS.characters;
        for(let i = 0; i < expectedList.length; i++) {
            let data = expectedList[i];
            let isExist = false;
            for(let k = 0; k < this.characterList.length; k++){
                let charData = this.characterList[k];
                let expectedName = data.name.replace("{WORKSPACE}", WORKSPACE_NAME);
                if(expectedName == charData.name){
                    isExist = true;
                }

                if (isExist) break;
            }

            if(!isExist){
                console.log(`${data.defaultCharacterDescription.givenName} does not exist. I'm requesting to create it.`)
                await this.CreateNewCharacter(data);
            } else {
                console.log(`${data.defaultCharacterDescription.givenName} exists, not updating.`)
            }
        }
        console.log(`All characters are ready to use.`)
    }

    private async PopulateCharacters(){
        let headers = await this.GetHeader();
        let response = await axios.get(GET_CHARACTERS, {
            headers: headers
        });
        this.characterList = response.data.characters;
    }

    private internalDelay(ms: number): Promise<void> {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    private async CreateNewCharacter(characterData){
        try {
            let headers = await this.GetHeader();

            let response = await axios.post(CREATE_URI, JSON.stringify(characterData), {
                headers: headers
            });
    
            let checkUri = "https://studio.inworld.ai/v1alpha/" + response.data.name
    
            let isDone = false; 
            let nameFetched= ""        
            while(!isDone){
                headers = await this.GetHeader();
                let checkData = await axios.get(checkUri, {
                    headers: headers
                });
                isDone = checkData.data.done;
    
                if(isDone){
                    nameFetched = checkData.data.response.name;
                } else {
                    await this.internalDelay(1000);
                }
            }
            
            characterData.name = nameFetched;
            this.SharedKnowledge.commonKnowledges.forEach(knowledge => {
                characterData.commonKnowledges.push(knowledge.name)
            });

            if(characterData.defaultCharacterDescription.givenName.includes(" Guard")){
                characterData.defaultCharacterDescription.givenName = "Rolf"
            }
            headers = await this.GetHeader();
            await axios.patch("https://studio.inworld.ai/v1alpha/" + characterData.name, JSON.stringify(characterData), {
                headers: headers
            });
        } catch (e){
            console.error(e)
        }
    }

    async SetupCommonKnowledge() {
        let commonknowledgeList = await this.GetAllCurrentCommonKnowledges();
        let expectedList = SKYRIM_KNOWLEDGE.list;
        for (let i = 0; i < expectedList.length; i++) {
            let singleKnowledge = expectedList[i];
            let filtered = commonknowledgeList.commonKnowledges.filter(el => el.displayName == singleKnowledge.displayName);
            if (filtered.length == 0) {
                console.log("Creating a new common knowledge");
                await this.CreateNewCommonKnowledge(singleKnowledge);
            }
        }
        this.SharedKnowledge = await this.GetAllCurrentCommonKnowledges();
        console.log("All common knowledge has been processed.");
        return;
    }

    async CreateNewCommonKnowledge(commonKnowledge) {
        let normalizedUri = SHARED_KNOWLEDGE_URL.replace("?pageSize=500", "");
        let header = await this.GetHeader(true);
        await axios.post(normalizedUri, commonKnowledge, {
            headers: header
        });
    }

    async GetAllCurrentCommonKnowledges(){
        let header = await this.GetHeader(true);
        let response = await axios.get(SHARED_KNOWLEDGE_URL, {
            headers: header
        });
        return response.data;
    }
    
    GetCharacterIdentifier(name){
        for(let i = 0; i < this.characterList.length; i++) {
            let character = this.characterList[i];
            let nameNormalized = character.name.toLowerCase().replace("_", " ");
            if(nameNormalized.includes(name.toLowerCase()) || character.defaultCharacterDescription.givenName.toLowerCase().includes(name.toLowerCase())){
                let name = character.name;
                let id = name.replace("workspaces/" + WORKSPACE_NAME + "/characters/", "")
                return id;
            }
        }
    }

    GetAllCharacterName(){
        let names = [];
        for(let i = 0; i < this.characterList.length; i++) {
            let character = this.characterList[i];
            names.push(character.defaultCharacterDescription.givenName);
        }
        return names;
    }

    private async GetHeader(isKnowledge: boolean = false){
        let token = await this.loginManager.GetTokenDirectly();
        let headerConfig = {
            'authorization': 'Bearer ' + token,
            'content-type': 'text/plain;charset=UTF-8',
            'origin': 'https://studio.inworld.ai',
            'referer': 'https://studio.inworld.ai/workspaces/' + WORKSPACE_NAME + (!isKnowledge ? '/characters':  '/knowledge'),
            'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.0.0 Safari/537.36'
        }
        return headerConfig;
    }

    
}