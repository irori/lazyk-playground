$(function() {
  var worker;
  var outputElem = $('#output');

  $('#shareURL').hide();

  function handleWorkerMessage(e) {
    switch (e.data.cmd) {
    case 'putc':
      outputElem.val(outputElem.val() + e.data.ch);
      break;
    case 'error':
      $('#error').text("error: " + e.data.message);
      break;
    case 'stopped':
      setRunningState(true);
      break;
    case 'terminate':
      setRunningState(true);
      worker.terminate();
      worker = null;
      break;
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

  $('#run').click(function() {
    setRunningState(false);
    if (!worker) {
      worker = new Worker('/static/lazyk.js');
      worker.addEventListener('message', handleWorkerMessage);
    }

    $('#output').val('');
    $('#error').text('');

    worker.postMessage(formData());
  });

  $('#stop').click(function() {
    if (worker) {
      worker.terminate();
      worker = null;
    }
    setRunningState(true);
  });

  var saving = false;
  $('#save').click(function() {
    if (saving)
      return;
    saving = true;
    $('#shareURL').hide();
    var data = formData();
    $.ajax("/save", {
      processData: false,
      contentType: 'application/json',
      data: JSON.stringify(data),
      type: "POST",
    }).done(function(data) {
      var path = "/p/" + data;
      var url = origin(window.location) + path;
      $('#shareURL').show().val(url).focus().select();
    }).fail(function() {
      alert("Server error; try again.");
    }).always(function() {
      saving = false;
    });
  });
});
