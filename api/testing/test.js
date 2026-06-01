const WebSocket = require("ws");
const ws = new WebSocket("ws://localhost:8080");

ws.onopen = () => {
  console.log("connected");

  ws.send("step");

  setTimeout(() => {
    ws.send("cycle");
  }, 1000);
};

ws.onmessage = (e) => {
  console.log(e.data);
};