var eval_program = Module.cwrap('eval_program', null, ['string', 'string']);

addEventListener('message', function(e) {
  try {
    eval_program(e.data.program, e.data.input);
  } catch (e) {
    if (e.name != "ExitStatus")
      throw e;
  }
  postMessage({'cmd': 'terminate'});
});
