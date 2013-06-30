package playground

import (
	"appengine"
	"appengine/datastore"
	"crypto/sha1"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
)

const salt = "[replace this with something unique]"

type Snippet struct {
	Program string
	Input string
}

func (s *Snippet) Id() string {
	h := sha1.New()
	io.WriteString(h, s.Program)
	io.WriteString(h, salt)
	io.WriteString(h, s.Input)
	sum := h.Sum(nil)
	b := make([]byte, base64.URLEncoding.EncodedLen(len(sum)))
	base64.URLEncoding.Encode(b, sum)
	return string(b)[:10]
}

func init() {
	http.HandleFunc("/save", save)
}

func save(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Error(w, "Forbidden", http.StatusForbidden)
		return
	}
	c := appengine.NewContext(r)

	var snip Snippet
	decoder := json.NewDecoder(r.Body)
	err := decoder.Decode(&snip)
	if err != nil {
		c.Errorf("reading Body: %v", err)
		http.Error(w, "Server Error", http.StatusInternalServerError)
		return
	}
	r.Body.Close()

	id := snip.Id()
	key := datastore.NewKey(c, "Snippet", id, 0, nil)
	_, err = datastore.Put(c, key, &snip)
	if err != nil {
		c.Errorf("putting Snippet: %v", err)
		http.Error(w, "Server Error", http.StatusInternalServerError)
		return
	}

	fmt.Fprint(w, id)
}
