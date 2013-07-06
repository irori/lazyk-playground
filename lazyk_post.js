var eval_program = Module.cwrap('eval_program', null, ['string', 'string']);
var is_valid_program = Module.cwrap('is_valid_program', 'number', ['string']);
var output_utf8;

addEventListener('message', function(e) {
  try {
    switch (e.data.cmd) {
    case 'eval':
      output_utf8 = new Runtime.UTF8Processor();
      eval_program(e.data.program, e.data.input);
      postMessage({'cmd': 'stopped'});
      break;
    case 'validate':
      var result = is_valid_program(e.data.program);
      postMessage({'cmd': 'validated', 'ok': result});
      break;
    }
  } catch (e) {
    postMessage({'cmd': 'terminate'});
    throw e;
  }
});
