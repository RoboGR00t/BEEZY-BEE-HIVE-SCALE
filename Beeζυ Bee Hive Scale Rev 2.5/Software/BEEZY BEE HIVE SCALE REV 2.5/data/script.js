/* SOCKET GATEWAY --> */ 
var gateway = 'ws://'+window.location.hostname+'/ws';
/* <-- SOCKET GATEWAY */ 

var websocket;

/* INITIALIZING WEB SOCKET --> */ 
function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
}
/* <-- INITIALIZING WEB SOCKET */ 

 
function onOpen(event) {
  console.log('Connection opened');
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

/* WEB PAGE EVENT LISTENER --> */ 
window.addEventListener('load', onLoad);
/* <-- WEB PAGE EVENT LISTENER */ 

/* CALL initWebSocket() --> */ 
function onLoad(event) {
  initWebSocket();
}
/* <-- CALL initWebSocket() --> */

/* SHOW NECESSARY FIELDS PER CHOICE --> */ 
function showFields(){
  var fields = document.getElementById("pref").value;
  if(fields == 'sms'){
    document.getElementById("thingspeak").style.display = "none";
    document.getElementById("tl").style.display = "none";
    document.getElementById("phone_telecom").style.display = "none";
    document.getElementById("phone").style.display = "block";
    document.getElementById("pl").style.display = "block";
  }
  else if(fields == 'thingspeak'){
    document.getElementById("thingspeak").style.display = "block";
    document.getElementById("tl").style.display = "block";
    document.getElementById("phone_telecom").style.display = "block";
    document.getElementById("phone").style.display = "none";
    document.getElementById("pl").style.display = "none";
  }
  else{
    document.getElementById("thingspeak").style.display = "block";
    document.getElementById("tl").style.display = "block";
    document.getElementById("phone_telecom").style.display = "block";
    document.getElementById("phone").style.display = "block";
    document.getElementById("pl").style.display = "block";
  }
}
/* <-- SHOW NECESSARY FIELDS PER CHOICE */ 

/* SEND DATA TO ESP32 VIA WEB SOCKET --> */ 
function send(){

  place = document.getElementById("place").value;
  thingspeak = document.getElementById("thingspeak").value;
  phone = document.getElementById("phone").value;
  sms = document.getElementById("sms").value;
  apn = document.getElementById("phone_telecom").value;
  prefix = document.getElementById("pref").value;

  document.getElementById("savem").style.fontSize = "0px";

  document.getElementById("place").value = "";
  document.getElementById("thingspeak").value = "";
  document.getElementById("phone").value = "";
  document.getElementById("sms").value = "";

  /* CHECK IF ALL NECESSERY FIELDS ARE COMPLETE --> */ 
  if(prefix == "sms_thingspeak"){
    if(place != "" && thingspeak != "" && phone != "" && sms != "" ){
      msg = place+"|"+thingspeak+"|"+phone+"|"+sms+"|"+apn+"|"+prefix+"|";
      console.log(msg);
      websocket.send(msg);
      document.getElementById("savem").style.fontSize = "10px";
    }
  }
  else if (prefix == "sms") {
    if(place != "" && phone != "" && sms != "" ){
      msg = place+"|"+"0"+"|"+phone+"|"+sms+"|"+"0"+"|"+prefix+"|";
      console.log(msg);
      websocket.send(msg);
      document.getElementById("savem").style.fontSize = "10px";
    }
  }
  else if (prefix == "thingspeak") {
    if(place != "" && thingspeak != "" && sms != "" ){
      msg = place+"|"+thingspeak+"|"+"0"+"|"+sms+"|"+apn+"|"+prefix+"|";
      console.log(msg);
      websocket.send(msg);
      document.getElementById("savem").style.fontSize = "10px";
    }
  }
  /* CHECK IF ALL NECESSERY FIELDS ARE COMPLETE --> */ 
}
/* <-- SEND DATA TO ESP32 VIA WEB SOCKET */ 
