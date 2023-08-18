import * as dotenv from 'dotenv'
import * as fs from 'fs';
import websocketPlugin, {SocketStream} from "@fastify/websocket"
import Fastify, {FastifyRequest} from 'fastify'
import InworldClientManager from "./Inworld/InworldManager.js";
import path from "path";

const resolved = path.resolve(".env");
console.log("Reading .env from location: ", resolved);
try {
    dotenv.config({path: resolved})
} catch (e) {
    console.error("Something is not right with your env config!", e)
}

const fastify = Fastify({logger: true});
fastify.register(websocketPlugin);

const ClientManager = new InworldClientManager();


process.on('uncaughtException', function  (err, origin) {
    console.error('Caught exception: ', err, origin);
    logToLog(JSON.stringify({err, origin}));
});

process.on('unhandledRejection', function (err, origin) {
    console.error('Caught rejection: ', err, origin);
    logToLog(JSON.stringify({err, origin}));
});

RunInformation();
OverrideConsole();
logToLog("=============================S=T=A=R=T=I=N=G===T=H=E===M=O=D=============================");

fastify.get('/ping', (request, reply) => {
    return {'status': "OK"}
});

// Socket connection for better communication channel
fastify.register(async function (fastify) {
    fastify.get('/chat', {
        websocket: true
    }, (connection : SocketStream, req : FastifyRequest) => {
        connection.socket.on('message', msg => {
            let message = JSON.parse(msg.toString());
            console.log("Message received", message);
            if (message.type == "connect") {
                ClientManager.ConnectToCharacterViaSocket(message.id, "Dragonborn", connection.socket);
            } else if (message.type == "start_listen") {
                ClientManager.StartTalking();
            } else if (message.type == "stop_listen") {
                ClientManager.StopTalking();
            } else if (message.type == "message") {
                ClientManager.Say(message.message);
            }
        })
    })
});

// Run the server!
const StartEngine = async () => {
    try {
        let portOnConfig = parseInt(process.env.CLIENT_PORT);
        await fastify.listen({port: portOnConfig})
    } catch (err) {
        logToLog(JSON.stringify(err));
        fastify.log.error(err);
        console.error(err);
        process.exit(1)
    }
}; 

StartEngine();


function RunInformation(){
    console.log("\x1b[32m",`                                                                                                    
                                                                                                    
                                                                                                    
                                             @        ,                                             
                                            @@@       @@#                                           
                                           @@           @@                                          
                                          @@    (       &@@                                         
                                         @@@   &@@@@     @@@                                        
                                       *@@@    @@  (@%    @&@                                       
                                      @@,@*   (      #@   @@ @                                      
                                     @@ @@@        @@,    @@@ @,                                    
                                    @@ @@      @@,          @@ @@                                   
                                   @@ @@&      @@@          @@@ @@                                  
                                  @@.@@@    @#@ @@@*  .@&   %@@@.@@                                 
                                  @@ @@@&&, &@@@@@@@@@ @   &@@@ @@/                                 
                                   @@#@@@ @@@@@@@@ @@@@@@@@@@@,@@                                   
                                    (@@@@@@@% @@@@@@@@# /@@@@&@@                                    
                                      @@@@@  @  @@@.@ &   #@@@@                                     
                                       @@@,       @@.     @@@@                                      
                                        @@@       @@@     @@@                                       
                                         @@@@@    &@@  @@@@&                                        
                                          @@@@   @@    @@@                                          
                                           @@@  @&     &@                                           
                                            ,@  %@                                                  
                                                &@                                                  
                                               @                                                    
                                                @, #@                                               
                                                 @@&                                                
                                                  ,                                                 
                                                                                                   `);
    console.log("\x1b[34m", "****************************************************");
    console.log("\x1b[32m", "Don't worry, you are suppose to see this!");
    console.log("\x1b[34m", "\n****************************************************\n\n");
    console.log("\x1b[31m", "DONT close this window or other window that opens if you want to use the mod. Close both only once you are done with the game. (Especially audio one because it really hits CPU)");
    console.log("\x1b[32m", "Errors will shown here.");
}

function OverrideConsole() {
    const originalLog = console.log;
    console.log = function () {
        const timestamp: string = new Date().toISOString();
        const args = Array.prototype.slice.call(arguments);
        args.unshift(`[${timestamp}]`);
        originalLog.apply(console, args);
    };
    const originalError = console.error;
    console.error = function () {
        const timestamp: string = new Date().toISOString();
        const args = Array.prototype.slice.call(arguments);
        args.unshift(`[${timestamp}]`);
        originalError.apply(console, args);
        logToLog(JSON.stringify(args));
    };

    (console as any).logToLog =   function () {
        logToLog(JSON.stringify(Array.prototype.slice.call(arguments)));
    }
}

function logToLog(message: string): void {
    const timestamp: string = new Date().toISOString();
    const logMessage: string = `${timestamp} - ${message}`;
    const logFileName = "InworldClient.log"
    if (fs.existsSync(logFileName)) { // File exists, append to it
        fs.appendFileSync(logFileName, logMessage + '\n', 'utf8');
    } else { // File does not exist, create it and write the log message
        fs.writeFileSync(logFileName, logMessage + '\n', 'utf8');
    }
}
