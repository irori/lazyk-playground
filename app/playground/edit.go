package playground

import (
	"appengine"
	"appengine/datastore"
	"net/http"
	"strings"
	"text/template"
)

func init() {
	http.HandleFunc("/", edit)
}

var editTemplate = template.Must(template.ParseFiles("playground/edit.html"))

type editData struct {
	Snippet *Snippet;
}

func edit(w http.ResponseWriter, r *http.Request) {
	snip := &Snippet{}
	if strings.HasPrefix(r.URL.Path, "/p/") {
		c := appengine.NewContext(r)
		id := r.URL.Path[3:]
		key := datastore.NewKey(c, "Snippet", id, 0, nil)
		err := datastore.Get(c, key, snip)
		if err != nil {
			if err != datastore.ErrNoSuchEntity {
				c.Errorf("loading Snippet: %v", err)
			}
			http.Error(w, "Snippet not found", http.StatusNotFound)
			return
		}
	}
	editTemplate.Execute(w, &editData{snip})
}
