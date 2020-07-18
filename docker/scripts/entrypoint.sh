#!/bin/sh

LOGDIR=/opt/spt/logs
DBFILE=/opt/spt/data/dbip.mmdb

Decompress()
{
  if [ -f /opt/spt/data/dbip.mmdb.gz ]
  then
    gzip -dc /opt/spt/data/dbip.mmdb.gz > $DBFILE
  else
    echo "DB IP file not found.  Make sure it is mounted for the running container."
    exit 1
  fi
}

Defaults()
{
  if [ -z "$PORT" ]
  then
    PORT=8010
    echo "PORT not set.  Will default to $PORT"
  fi

  if [ -z "$THREADS" ]
  then
    THREADS=8
    echo "THREADS not set.  Will default to $THREADS"
  fi
}

Service()
{
  if [ ! -d $LOGDIR ]
  then
    mkdir -p $LOGDIR
    chown spt:spt $LOGDIR
  fi

  echo "Starting up MaxMind DB websocket server"
  /opt/spt/bin/mmdb-ws -c true -o ${LOGDIR}/ -p $PORT -n $THREADS -f $DBFILE
}

Decompress && Defaults && Service