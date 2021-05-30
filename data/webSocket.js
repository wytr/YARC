var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
    console.log('Connection opened');    
}

function sendMessage(message) {
    websocket.send(message);
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {    
    var data = event.data.split(';');
    console.log(data);
    addData(data[0], data[1], data[2]);    
}

window.addEventListener('load', _ => {  
    if(!websocket){
        initWebSocket();
    }    
});