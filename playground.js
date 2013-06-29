var outputElement;
var worker;

function handleWorkerMessage(e) {
  switch (e.data.cmd) {
  case 'putc':
    outputElement.textContent += String.fromCharCode(e.data.ch);
    break;
  case 'error':
    document.getElementById('error').textContent = "error: " + e.data.message;
    setRunningState(true);
    break;
  case 'terminate':
    setRunningState(true);
    break;
  }
}

function setRunningState(running) {
  document.getElementById('run').disabled = !running;
  document.getElementById('stop').disabled = running;
  document.getElementById('program').readOnly = !running;
  document.getElementById('input').readOnly = !running;
}

function run() {
  setRunningState(false);

  if (worker)
    worker.terminate();
  worker = new Worker('lazyk.js');
  worker.addEventListener('message', handleWorkerMessage);

  var program = document.getElementById('program').value;
  var input = document.getElementById('input').value;
  outputElement = document.getElementById('output');
  outputElement.textContent = null;
  document.getElementById('error').textContent = null;

  worker.postMessage({'program': program, 'input': input});
}

function stop() {
  if (worker) {
    worker.terminate();
    worker = null;
  }
  setRunningState(true);
}
