var eval_program = Module.cwrap('eval_program', null, ['string', 'string']);
var output_utf8;

addEventListener('message', function(e) {
  try {
    output_utf8 = new Runtime.UTF8Processor();
    eval_program(e.data.program, e.data.input);
    postMessage({'cmd': 'stopped'});
  } catch (e) {
    postMessage({'cmd': 'terminate'});
    throw e;
  }
});
