var eval_program = Module.cwrap('eval_program', null, ['string', 'string']);

addEventListener('message', function(e) {
  try {
    eval_program(e.data.program, e.data.input);
    postMessage({'cmd': 'stopped'});
  } catch (e) {
    postMessage({'cmd': 'terminate'});
    throw e;
  }
});
