lazyk.js: lazyk.c library.js lazyk_pre.js lazyk_post.js
	emcc -O2 \
		-s EXPORTED_FUNCTIONS="['_eval_program', '_is_valid_program']" \
		--js-library library.js \
		--pre-js lazyk_pre.js \
		--post-js lazyk_post.js \
		-o $@ lazyk.c
