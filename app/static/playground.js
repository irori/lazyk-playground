$(function() {
  var worker;
  var outputElem = $('#output');
  var dataToSave = null;

  $('#shareURL').hide();

  function handleWorkerMessage(e) {
    switch (e.data.cmd) {
    case 'putc':
      outputElem.val(outputElem.val() + e.data.ch);
      break;
    case 'error':
      $('#error').text(e.data.message);
      break;
    case 'stopped':
      setRunningState(true);
      break;
    case 'terminate':
      setRunningState(true);
      worker.terminate();
      worker = null;
      break;
    case 'validated':
      if (e.data.ok && !dataToSave.program.match(/^(\s|#.*\n)*$/))
	submitCode();
      else {
	alert("Enter a non-empty, valid Lazy K program.");
	dataToSave = null;
	$('#save').attr('disabled', false);
      }
      break;
    }
  }

  function initWorker() {
    if (!worker) {
      if (typeof(Worker) == "undefined")
	alert('Sorry, this browser is not supported.');
      worker = new Worker('/static/lazyk.js');
      worker.addEventListener('message', handleWorkerMessage);
    }
  }

  function setRunningState(running) {
    $('#run').attr('disabled', !running);
    $('#stop').attr('disabled', running);
  }

  function formData() {
    return {'program': $('#program').val(), 'input': $('#input').val()};
  }

  function origin(href) {
    return (""+href).split("/").slice(0, 3).join("/");
  }

  function submitCode() {
    $.ajax("/save", {
      processData: false,
      contentType: 'application/json',
      data: JSON.stringify(dataToSave),
      type: "POST",
    }).done(function(body) {
      var path = "/p/" + body;
      var url = origin(window.location) + path;
      $('#shareURL').show().val(url).focus().select();
    }).fail(function() {
      alert("Server error; try again.");
    }).always(function() {
      dataToSave = null;
      $('#save').attr('disabled', false);
    });
  }

  $('#run').click(function() {
    setRunningState(false);
    initWorker();

    $('#output').val('');
    $('#error').text('');

    msg = formData();
    msg['cmd'] = 'eval';
    worker.postMessage(msg);
  });

  $('#stop').click(function() {
    if (worker) {
      worker.terminate();
      worker = null;
    }
    setRunningState(true);
  });

  $('#save').click(function() {
    if (dataToSave)
      return;
    dataToSave = formData();
    $('#shareURL').hide();
    $('#save').attr('disabled', true);

    initWorker();
    worker.postMessage({'cmd': 'validate', 'program': dataToSave.program});
  });
});
