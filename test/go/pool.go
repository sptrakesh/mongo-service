package main

import (
  "fmt"
  "log"
  "net"
  "sync"

  "github.com/fatih/pool"
)

func connectionPool() *mongoClientHolder {
  const fn = "connectionPool"

  /*
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
   */

  ponce.Do(func() {
    factory := func() (net.Conn, error) { return net.Dial("tcp", fmt.Sprintf("%v:%v", host, port)) }
    p, err := pool.NewChannelPool(5, 200, factory)
    if err != nil {log.Printf("%v - Error creating pool %v\n", fn, err)}
    mongoClient = &mongoClientHolder{p: p}
  })

  return mongoClient
}

func ClosePool() {
  connectionPool().p.Close()
}

var (
  ponce sync.Once
  mongoClient *mongoClientHolder
)
