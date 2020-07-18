#!/bin/sh

case "$1" in
  'start')
    (cd `dirname $0`;
    docker run -d --rm \
      -p 2000:2000 \
      --name mongo-service mongo-service)
    ;;
  'logs')
    docker logs mongo-service
    ;;
  'stats')
    docker stats mongo-service
    ;;
  'stop')
    docker stop mongo-service
    ;;
  *)
    echo "Usage: $0 <start|logs|stats|stop>"
    exit 1
    ;;
esac
