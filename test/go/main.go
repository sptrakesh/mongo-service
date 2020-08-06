package main

import "flag"

func main() {
  p := flag.Int("port", 2000, "Port for the service")
  h := flag.String("host", "127.0.0.1", "Host on which the service runs")
  flag.Parse()

  port = *p
  host = *h

  create()
  retrieve()
  retrieveMultiple()
  update()
  updateByQuery()
  delete()
  ClosePool()
}

var (
  port int
  host string
)
