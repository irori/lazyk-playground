mergeInto(LibraryManager.library, {
  output_char: function(c) {
    ch = output_utf8.processCChar(c);
    if (ch.length > 0)
      postMessage({'cmd': 'putc', 'ch': ch});
  },
  error: function(msg) {
    postMessage({'cmd': 'error', 'message': Pointer_stringify(msg)});
  },
});
