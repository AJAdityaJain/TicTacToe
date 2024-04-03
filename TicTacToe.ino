// #include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// REPLACE WITH YOUR DETAILS
const char* ssid = "SSID";
const char* password = "PWD";

uint8_t board[] = {0,0,0,0,0,0,0,0,0};
uint32_t u1 = 0;
uint32_t u2 = 0;
uint32_t turn = 1;
bool gameend = false;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html>
<head>
  <style>
    body{
      background-color: black;
      text-align: center;
      font-family:system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
    }
    h2{
      color: white;
    }
    h3{
      color: red;
      border: none;
    }
    td:not(#wltc){
      width: 50px;
      height: 50px;
      border: 1px solid white;
      color: white;
    }
  </style>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">    
</head>
<body>
  <div>

  <table align="center">
      <tr><td colspan="3"><h2>Tic-Tac-Toe</h2></td></tr>
      <tr>
          <td id="0" onclick="play(0)"> </td>
          <td id="1" onclick="play(1)"> </td>
          <td id="2" onclick="play(2)"> </td></tr><tr>
          <td id="3" onclick="play(3)"> </td>
          <td id="4" onclick="play(4)"> </td>
          <td id="5" onclick="play(5)"> </td></tr><tr>
          <td id="6" onclick="play(6)"> </td>
          <td id="7" onclick="play(7)"> </td>
          <td id="8" onclick="play(8)"> </td>
      </tr>
      <tr><td id="wltc" colspan="3" onclick="reset()"><h3 id="wlt"></h3></td></tr>
  </table>
</div>

<script>
  // REPLACE WITH YOUR IP;
  var gateway = `ws://192.168.29.220/ws`;
  var websocket;
  var dwin = false;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
  }
  function onOpen(event) {console.log('Connection opened');}
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    console.log( event.data);        
    if(event.data == 'win'){win();}
    else if (event.data == 'end'){end();}
    else if (event.data == 'draw'){draw();}
    else if(event.data[0] == "{"){
      let obj = JSON.parse(event.data);
      if(obj.code == "board"){
        if(obj.bb == "000000000")document.getElementById('wlt').innerHTML = "";
        for(let i = 0; i < 9; i++){
          switch(obj.bb[i]){
            case '0': document.getElementById(i.toString()).innerHTML = ' ';break;
            case '1': document.getElementById(i.toString()).innerHTML = 'x';break;
            case '2': document.getElementById(i.toString()).innerHTML = 'o';break;
          }
        }
      }
    }
  }
  function onLoad(event) {initWebSocket();}
  function play(i){websocket.send("play"+i);}
  function win(){
    dwin = true;
    document.getElementById('wlt').innerHTML = "You WON the match!";
  }
  function end(){
    if(!dwin){
      document.getElementById('wlt').innerHTML = "You lost...";
    }
    dwin = false;
  }
  function draw(){
    document.getElementById('wlt').innerHTML = "Its a draw...";
    dwin = false;
  }
  function reset(){websocket.send("reset");}
</script>
</body>
</html>
)rawliteral";

void updateBoard(){
  String boarddata = String("{\"code\":\"board\",\"bb\":\"");
      for(uint8_t n : board)
        boarddata += String(n);
      boarddata += "\"}";
      ws.textAll(boarddata);
}

bool checkWinner(uint8_t player) {
    if (board[0] == player && board[1] == player && board[2] == player)
        return true;
    if (board[3] == player && board[4] == player && board[5] == player)
        return true;
    if (board[6] == player && board[7] == player && board[8] == player)
        return true;
    
    if (board[0] == player && board[3] == player && board[6] == player)
        return true;
    if (board[1] == player && board[4] == player && board[7] == player)
        return true;
    if (board[2] == player && board[5] == player && board[8] == player)
        return true;
    
    if (board[0] == player && board[4] == player && board[8] == player)
        return true;
    if (board[2] == player && board[4] == player && board[6] == player)
        return true;

    return false;
}

void winLose() {
  bool ret = true;
  for(auto c : board)
    if(c != 0){
      ret = false;
      break;
    } 
  if(ret){
    return;
    updateBoard();
  }

  if (u2 == 0 || checkWinner(1)) {
    ws.text(u1, "win");
    ws.textAll("end");
    gameend = true; 
  } else if (u1 == 0 || checkWinner(2)) {
    ws.text(u2, "win");
    ws.textAll("end");
    gameend = true; 
  }
  else{
    bool draw = true;
    for(int i = 0; i < 9; i ++){
      if(board[i] == 0){
        draw = false;
        break;
      } 
    }
    if(draw){
      ws.textAll("draw");
      gameend = true;
    }
  }

  updateBoard(); 
}

void resetBoard(){
  gameend = false; 
  for(int i = 0; i < 9; i++)board[i] = 0;
  turn = 1;
  if(u1 != 0 && u2 != 0){
    uint32_t u0 = u1;
    u1 = u2;
    u2 = u0;
  }
  updateBoard();
}

void handleWebSocketMessage(void *arg, char *data, size_t len, uint8_t user) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp(data, "update") == 0) {
      updateBoard();
    }
    else if(gameend && strcmp(data, "reset") == 0){
      resetBoard();
    }
    else if (!gameend && strlen(data) == 5 && data[0] == 'p' && data[1] == 'l' && data[2] == 'a' && data[3] == 'y') {
      if(u1 != 0 || u2 != 0){
        int i = data[4] - '0';
        if(board[i] == 0){
          if(user == u1 && turn == 1){
            board[i] = 1;
            turn = 2;
            winLose();
          }
          else if (user == u2 && turn == 2){
            turn = 1;
            board[i] = 2;
            winLose();
          }
        }
      }
      else{
        winLose();
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
             if(u1 == 0) u1 = client->id();
        else if(u2 == 0) u2 = client->id();
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        updateBoard();
        break;
      case WS_EVT_DISCONNECT:{
        uint8_t clientID = client->id();
             if(clientID == u1) u1 = 0;
        else if(clientID == u2) u2 = 0;
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        winLose();
        break;
      }
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, (char*)data, len, client->id());
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void setup(){  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
}

void loop() {
  ws.cleanupClients(10);
}
