import {BLCRecorder} from './Audio/BLCRecorder.js';

export function GetSocketResponse(message: string, phoneme: string, type: string) {
    return {"message": message, "phoneme": phoneme, "type": type}
}

export class SkyrimInworldSocketController {
    private CombinedResponse : string = "";
    private CombinedPhoneme : string = "";
    private CombinedUserInput : string = "";
    private Recorder : BLCRecorder;
    private DisableAudio : boolean;
    constructor(private socket : WebSocket) {}

    ProcessMessage(msg : any) {
        if (msg.type == 'AUDIO') { 
            let arr = msg.audio.additionalPhonemeInfo;
            arr.forEach(ph => {
                if (ph.phoneme != "<INTERSPERSE_CHARACTER>") 
                    this.CombinedPhoneme += ph.phoneme
                
            });
            if (this.Recorder != null && !this.DisableAudio) 
                this.Recorder.playChunk(msg.audio.chunk);
        } else if (msg.emotions) { // dont use for now
        } else if (msg.phonemes) {
            console.log(msg.phonemes)
        } else if (msg.isText()) {
            if (msg.routing.target.isCharacter) { // Always overwrite user input
                this.CombinedUserInput = msg.text.text;
            } else {
                let responseMessage = msg.text.text;
                this.CombinedResponse += responseMessage;
            }
        } else if (msg.isInteractionEnd()) {
            if (this.Recorder != null && !this.DisableAudio) {
                console.log("Releasing voice chunks");
                this.Recorder.releaseChunks();
            }
            let result = GetSocketResponse(this.CombinedResponse, this.CombinedPhoneme, "chat");
            console.log("Character said: " + this.CombinedResponse);
            (console as any).logToLog(`Character said: ${this.CombinedResponse}`)
            this.CombinedResponse = "";
            this.socket.send(JSON.stringify(result));
        }
    }

    SetRecorder(recorder : BLCRecorder) {
        this.DisableAudio = (process.env.DISABLE_AUDIO).toLowerCase() === "true"; 
        console.log("Will it play audio from characters? " + !this.DisableAudio);
        (console as any).logToLog("Will it play audio from characters? " + !this.DisableAudio)
        this.Recorder = recorder;
    }

    SendUserVoiceInput() {
        let userData = GetSocketResponse(this.CombinedUserInput, "", "user_voice");
        this.socket.send(JSON.stringify(userData));
    }
}
