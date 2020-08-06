package main

import (
  "fmt"
  "log"
  "net"
  "sync"
  "time"

  "github.com/sabey/lagoon"
)

func connectionPool() *mongoClientHolder {
  const fn = "connectionPool"

  config := func() *lagoon.Config {
    buffer := lagoon.CreateBuffer(200, time.Second * 5)
    return &lagoon.Config{
      Dial: func() (net.Conn, error) {
        return net.DialTimeout("tcp", fmt.Sprintf("%v:%v", host, port), time.Second * 5)
      },
      DialInitial: 1,
      IdleTimeout: time.Second * time.Duration(600),
      Buffer: buffer,
    }
  }

  ponce.Do(func() {
    l, err := lagoon.CreateLagoon(config())
    if err != nil {log.Printf("%v - Error creating lagoon pool %v\n", fn, err)}
    mongoClient = &mongoClientHolder{l: l}
  })

  return mongoClient
}

func ClosePool() {
  connectionPool().l.Close()
}

var (
  ponce sync.Once
  mongoClient *mongoClientHolder
)
