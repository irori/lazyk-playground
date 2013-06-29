var eval_program = Module.cwrap('eval_program', null, ['string', 'string']);

addEventListener('message', function(e) {
  eval_program(e.data.program, e.data.input);
  postMessage({'cmd': 'terminate'});
});
