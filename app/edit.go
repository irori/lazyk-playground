package main

import (
	"google.golang.org/appengine"
	"google.golang.org/appengine/datastore"
	"log"
	"net/http"
	"strings"
	"text/template"
)

var editTemplate = template.Must(template.ParseFiles("template/edit.html"))

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
				log.Printf("loading Snippet: %v", err)
			}
			http.Error(w, "Snippet not found", http.StatusNotFound)
			return
		}
	}
	editTemplate.Execute(w, &editData{snip})
}

func main() {
	http.HandleFunc("/", edit)
	http.HandleFunc("/save", save)

	appengine.Main()
}
