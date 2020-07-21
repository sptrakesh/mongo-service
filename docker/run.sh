#!/bin/sh

STACK=spt
DATA_DIR=$SPT_DATA_DIR
if [ -z "$DATA_DIR" ]
then
  DATA_DIR='/private/var/tmp/spt'
fi

Directory() {
  if [ ! -d $1 ]
  then
    mkdir -p $1
  fi
  chmod 777 $1
}

Swarm()
{
  running=`docker system info | grep Swarm | awk '{print $2}'`
  if [ "$running" == "inactive" ]
  then
    docker swarm init
  fi
}

Stack()
{
  Swarm
  docker pull mongo
  Directory $DATA_DIR/mongo
  Directory $DATA_DIR/mongo-service
  (cd `dirname $0`;
    DATA_DIR=$DATA_DIR docker stack deploy -c stack.yml $STACK)
}

Logs()
{
  if [ -z "$2" ]
  then
    for i in `docker stack ps $STACK | grep $1 | grep Running | awk '{print $1}'`
    do
      echo "Logs for $i"
      echo ""
      echo ""
      docker service logs "$i"
      echo ""
      echo ""
      echo ""
    done
  else
    for i in `docker stack ps $STACK | grep $1 | grep Running | awk '{print $1}'`
    do
      echo "Logs for $i"
      echo ""
      echo ""
      docker service logs "$i" | tail "-$2"
      echo ""
      echo ""
      echo ""
    done
  fi
}

case "$1" in
  'start')
    Stack
    ;;
  'logs')
    if [ -z "$2" ]
    then
      echo "Usage: $0 logs <service> [number of lines]"
      exit 2
    else
      Logs $2 $3
    fi
    ;;
  'status')
    docker service ls
    docker stack ps $STACK
    ;;
  'update')
    if [ -z "$2" ]
    then
      docker service update --force $STACK"_"mongo-service
    else
      docker service update --force $STACK"_$2"
    fi
    ;;
  'stop')
    docker stack rm $STACK
    ;;
  *)
    echo "Usage: $0 <start|logs|status|update|stop>"
    ;;
esac
