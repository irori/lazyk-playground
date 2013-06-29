mergeInto(LibraryManager.library, {
  output_char: function(c) {
    postMessage({'cmd': 'putc', 'ch': c});
  },
  error: function(msg) {
    postMessage({'cmd': 'error', 'message': Pointer_stringify(msg)});
  },
});
