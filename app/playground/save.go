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
	"regexp"
)

const salt = "[replace this with something unique]"

type Snippet struct {
	Code []byte
	Input []byte
}

func (s *Snippet) Id() string {
	h := sha1.New()
	h.Write(s.Code)
	io.WriteString(h, salt)
	h.Write(s.Input)
	sum := h.Sum(nil)
	b := make([]byte, base64.URLEncoding.EncodedLen(len(sum)))
	base64.URLEncoding.Encode(b, sum)
	return string(b)[:10]
}

type PostData struct {
	Program string
	Input string
}

func (data *PostData) ToSnippet() *Snippet {
	return &Snippet{
		Code: []byte(data.Program),
		Input: []byte(data.Input),
	}
}

func isValidCode(code []byte) bool {
	stripped := regexp.MustCompile(`\s|#.*\n`).ReplaceAll(code, []byte{})
	return regexp.MustCompile("^[`*()skiSKI01]+$").Match(stripped)
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

	var data PostData
	decoder := json.NewDecoder(r.Body)
	err := decoder.Decode(&data)
	if err != nil {
		c.Errorf("reading Body: %v", err)
		http.Error(w, "Server Error", http.StatusInternalServerError)
		return
	}
	r.Body.Close()

	snip := data.ToSnippet()

	if !isValidCode(snip.Code) {
		http.Error(w, "Invalid Lazy K code", http.StatusBadRequest)
		return
	}

	id := snip.Id()
	key := datastore.NewKey(c, "Snippet", id, 0, nil)
	_, err = datastore.Put(c, key, snip)
	if err != nil {
		c.Errorf("putting Snippet: %v", err)
		http.Error(w, "Server Error", http.StatusInternalServerError)
		return
	}

	fmt.Fprint(w, id)
}
