package main

import (
  "io"
  "log"

  "github.com/globalsign/mgo/bson"
  "github.com/sabey/lagoon"
)

type mongoClientHolder struct {
  l *lagoon.Lagoon
}

type payload struct {
  Action string `bson:"action"`
  Database string `bson:"database"`
  Collection string `bson:"collection"`
  Document bson.M `bson:"document"`
  Options bson.M `bson:"options,omitempty"`
  Metadata bson.M `bson:"metadata,omitempty"`
  Application string `bson:"application,omitempty"`
  SkipVersion bool `bson:"skipVersion"`
}

func (c *mongoClientHolder) create(e *payload) ([]byte, error) {
  e.Action = "create"
  e.Application = "go-test"
  return c.send(e)
}

func (c *mongoClientHolder) update(e *payload) ([]byte, error) {
  e.Action = "update"
  e.Application = "go-test"
  return c.send(e)
}

func (c *mongoClientHolder) count(db, coll string, query bson.M) (int64, error) {
  return c.countWithOptions(db, coll, query, nil)
}

func (c *mongoClientHolder) countWithOptions(db, coll string, query, options bson.M) (int64, error) {
  const fn = "countWithOptions"

  r := bson.M{"action": "count", "database": db, "collection": coll, "document": query, "application": "go-test"}
  if options != nil {r["options"] = options}
  b, err := c.send(r)

  if err != nil {return -1, err}
  m := bson.M{}
  if err := bson.Unmarshal(b, &m); err != nil {return -1, err}

  count, ok := m["count"]
  if !ok {
    return 0, nil
  }

  switch count.(type) {
  case int:
    return int64(count.(int)), nil
  case int32:
    return int64(count.(int32)), nil
  case int64:
    return count.(int64), nil
  default:
    log.Printf("%v - Unsupported data type %v\n", fn, count)
    return 0, nil
  }
}

func (c *mongoClientHolder) query(db, coll string, query interface{}) ([]byte, error) {
  return c.queryWithOptions(db, coll, query, nil)
}

func (c *mongoClientHolder) queryWithOptions(db, coll string, query interface{}, options bson.M) ([]byte, error) {
  r := bson.M{"action": "retrieve", "database": db, "collection": coll, "document": query, "application": "go-test"}
  if options != nil {r["options"] = options}
  return c.send(r)
}

func (c *mongoClientHolder) delete(db, coll string, query interface{}) ([]byte, error) {
  return c.deleteWithOptions(db, coll, query, nil)
}

func (c *mongoClientHolder) deleteWithOptions(db, coll string, query interface{}, options bson.M) ([]byte, error) {
  r := bson.M{"action": "delete", "database": db, "collection": coll, "document": query, "application": "go-test"}
  if options != nil {r["options"] = options}
  return c.send(r)
}

func (c *mongoClientHolder) send(m interface{}) ([]byte, error) {
  const fn = "mongoClient.send"

  b, err := bson.Marshal(m)
  if err != nil {
    log.Printf("%v - Error marshalling payload\n%v\n%v\n", fn, m, err)
    return nil, err
  }

  con, err := c.l.Dial()
  if err != nil {
    log.Printf("%v - Error connecting to mongo service %v\n", fn, err)
    return nil, err
  }
  defer con.Close()

  n, err := con.Write(b)
  if err != nil {
    log.Printf("%v - Error sending payload %v\n", fn, err)
    return nil, err
  }

  if n != len(b) {
    log.Printf("%v - Transmitted %d out of %d bytes\n", fn, n, len(b))
  }

  const bufSize = 8192
  buf := make([]byte, bufSize)
  data := make([]byte, 0)
  length := 0

  for {
    n, err := con.Read(buf)
    if err != nil {
      if err != io.EOF {
        log.Printf("%v - Read error %v\n", fn, err)
        return nil, err
      }
      break
    }

    data = append(data, buf[:n]...)
    length += n
    if n < bufSize {break}
  }
  log.Printf("%v - Response size: %d\n", fn, length)
  return data[:], nil
}
