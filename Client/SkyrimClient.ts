// @ts-check
import * as dotenv from 'dotenv'
import websocketPlugin from "@fastify/websocket"
import Fastify from 'fastify'
try {
  dotenv.config()
} catch (e) {
  console.error("Something is not right with your env config!", e)
}
const fastify = Fastify({
  logger: true
});
fastify.register(websocketPlugin);
import InworldClientManager from "./Inworld/InworldManager";

const ClientManager = new InworldClientManager();
var Connection: any;
let CombinedResponse: string;
let IsInteractionEnded:boolean;
let LastConnectedUser: string;

function SocketObserver(msg: any) {
  if (msg.type == 'AUDIO') {
    // dont use for now
  } else if (msg.emotions) {
    // dont use for now
  } else if (msg.isText()) {
    let responseMessage = msg.text.text;
    CombinedResponse += responseMessage;
  } else if (msg.isInteractionEnd()) {
    IsInteractionEnded = true;
  }
}

fastify.get('/ping', (request, reply) => {
  return { 'status': "OK"}
});

// Say
fastify.post('/say', async (request, reply) => {
  let requestBody = request.body as any;
  if (Connection == null) {
    console.log("Connection is empty. Creating a new connection")
    Connection = ClientManager.ConnectToCharacter(requestBody.id, "Dragonborn", SocketObserver);
    LastConnectedUser = requestBody.id;
  } else {
    if(LastConnectedUser != requestBody.id) {
      console.log(`Connection is for ${LastConnectedUser} and we requested it for ${requestBody.id} so we are creating new one`)
      LastConnectedUser = requestBody.id;
      Connection = ClientManager.ConnectToCharacter(requestBody.id, "Dragonborn", SocketObserver);
    }
  }
  
  if(requestBody.message == "") return "What?";
  CombinedResponse = "";
  IsInteractionEnded = false;
  ClientManager.Say(requestBody.message);
  return await new Promise( (resolve, reject) => {
    let interval = setInterval(() => {
      if (IsInteractionEnded) {
        // Clear the interval and resolve the promise with the combined response
        clearInterval(interval);
        resolve({ "message": CombinedResponse});
      }
    }, 100);
  });
});

// Run the server!
const StartEngine = async () => {
  try {
    await fastify.listen({
      port: 3000
    })
  } catch (err) {
    fastify.log.error(err);
    console.error(err);
    process.exit(1)
  }
}

StartEngine();