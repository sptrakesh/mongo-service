package main

import (
  "log"
  "time"

  "github.com/globalsign/mgo/bson"
)

type result struct {
  id *bson.ObjectId
  Error string `bson:"error"`
  Result bson.M `bson:"result"`
  Results []bson.M `bson:"results"`
  err error
}

func create() {
  const fn = "crud.create"

  insert := func(c chan result) {
    r := result{}
    oid := bson.NewObjectId()
    r.id = &oid
    doc := &payload{Database: database, Collection: collection, Document: bson.M{"_id": r.id, "time": time.Now()}}

    b, err := connectionPool().create(doc)
    if err != nil {
      r.err = err
      c <- r
      return
    }

    if err := bson.Unmarshal(b, &r); err != nil {
      r.err = err
    }

    c <- r
  }

  c := make(chan result)
  for i := 0; i < 100; i++ {
    go insert(c)
  }

  oids := make([]*bson.ObjectId, 100)
  for i := 0; i < 100; i++ {
    r := <-c
    if r.err != nil {
      log.Printf("%v - Error creating document %d %v\n", fn, i, r.err)
    } else {
      oids[i] = r.id
    }
  }

  ids = oids[:]
}

func retrieve() {
  const fn = "crud.retrieve"

  get := func(i int, c chan result) {
    r := result{}
    b, err := connectionPool().query(database, collection, bson.M{"_id": ids[i]})
    if err != nil {
      r.err = err
      c <- r
      return
    }

    if err := bson.Unmarshal(b, &r); err != nil {
      r.err = err
    }

    c <- r
  }

  c := make(chan result)
  for i := 0; i < 100; i++ {
    go get(i, c)
  }

  for i := 0; i < 100; i++ {
    r := <-c
    if r.err != nil {
      log.Printf("%v - Error retrieving document %d %v\n", fn, i, r.err)
    }
  }
}

func delete() {
  const fn = "crud.delete"

  remove := func(i int, c chan result) {
    r := result{}
    b, err := connectionPool().delete(database, collection, bson.M{"_id": ids[i]})
    if err != nil {
      r.err = err
      c <- r
      return
    }

    if err := bson.Unmarshal(b, &r); err != nil {
      r.err = err
    }

    c <- r
  }

  c := make(chan result)
  for i := 0; i < 100; i++ {
    go remove(i, c)
  }

  for i := 0; i < 100; i++ {
    r := <-c
    if r.err != nil {
      log.Printf("%v - Error deleting document %d %v\n", fn, i, r.err)
    }
  }
}

const (
  database = "itest"
  collection = "test"
)

var ids []*bson.ObjectId
