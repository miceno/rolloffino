
SERVER=${SERVER:-192.168.1.46}
PORT=${PORT:-8888}

function open_dome(){
  (echo -n "(SET:OPEN:0)" && sleep 1) | nc $SERVER $PORT
}

function close_dome(){
  (echo -n "(SET:CLOSE:0)" && sleep 1) | nc $SERVER $PORT
}

function isopen_dome(){
  (echo -n "(GET:OPENED:0)" && sleep 1) | nc $SERVER $PORT
}

function isclosed_dome(){
  (echo -n "(GET:CLOSED:0)" && sleep 1) | nc $SERVER $PORT
}
