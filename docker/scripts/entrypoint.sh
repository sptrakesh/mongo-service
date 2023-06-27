#!/bin/sh

LOGDIR=/opt/spt/logs

FromDb()
{
  cmd="/opt/spt/bin/configctl -s $1 -p $2"
  if [ -n "$3" ]
  then
    cmd="$cmd $3"
  fi

  status=1
  count=0
  echo "Checking if $1 is available"
  while [ $status -ne 0 ]
  do
    echo "[$count] Config-db Service $1:$2 not available ($status).  Sleeping 1s..."
    count=$(($count + 1 ))
    sleep 1
    nc -z $1 $2
    status=$?
  done

  status=1
  count=0
  echo "Checking if $1 has been bootstrapped"
  while [ $status -ne 0 ]
  do
    echo "[$count] Config-db Service $1:$2 not bootstrapped ($status).  Sleeping 1s..."
    count=$(($count + 1 ))
    sleep 1
    $cmd -a get -k /database/mongo/uri
    status=$?
  done

  MONGO_URI=`$cmd -a get -k /database/mongo/uri`
  MONGO_URI=`/opt/spt/bin/encrypter -d $MONGO_URI`

  VERSION_HISTORY_DATABASE=`$cmd -a get -k /database/mongo/database/versionHistory`
  echo $VERSION_HISTORY_DATABASE | grep Error
  if [ $? -eq 0 ]; then VERSION_HISTORY_DATABASE=''; fi

  VERSION_HISTORY_COLLECTION=`$cmd -a get -k /database/mongo/collection/versionHistory`
  echo $VERSION_HISTORY_COLLECTION | grep Error
  if [ $? -eq 0 ]; then VERSION_HISTORY_COLLECTION=''; fi

  METRIC_DATABASE=`$cmd -a get -k /database/mongo/database/metric`
  echo $METRIC_DATABASE | grep Error
  if [ $? -eq 0 ]; then METRIC_DATABASE=''; fi

  METRIC_COLLECTION=`$cmd -a get -k /database/mongo/collection/metric`
  echo $METRIC_COLLECTION | grep Error
  if [ $? -eq 0 ]; then METRIC_COLLECTION=''; fi

  LOG_LEVEL=`$cmd -a get -k /database/mongo/service/log/level`
  echo $LOG_LEVEL | grep Error
  if [ $? -eq 0 ]; then LOG_LEVEL=''; fi

  LOG_ASYNC=`$cmd -a get -k /database/mongo/service/log/async`
  echo $LOG_ASYNC | grep Error
  if [ $? -eq 0 ]; then LOG_ASYNC=''; fi
}

ConfigDb()
{
  if [ -z "$CONFIG_DB" ]
  then
    return
  fi

  server=`echo $CONFIG_DB | cut -d ':' -f1`
  port=`echo $CONFIG_DB | cut -d ':' -f2`
  FromDb $server $port
}

SecureConfigDb()
{
  if [ -z "$SECURE_CONFIG_DB" ]
  then
    return
  fi

  server=`echo $SECURE_CONFIG_DB | cut -d ':' -f1`
  port=`echo $SECURE_CONFIG_DB | cut -d ':' -f2`
  FromDb $server $port "--with-ssl"
}

Check()
{
  if [ -z "$MONGO_URI" ]
  then
    echo "MONGO_URI must be set."
    exit 1
  fi

  if [ -n "$ILP_SERVER" ]
  then
    status=1
    count=0
    echo "Checking if $ILP_SERVER is available"
    while [ $status -ne 0 ]
    do
      echo "[$count] ILP Service $ILP_SERVER:$ILP_PORT not available ($status).  Sleeping 1s..."
      count=$(($count + 1 ))
      sleep 1
      nc -z $ILP_SERVER $ILP_PORT
      status=$?
    done
  fi
}

Defaults()
{
  if [ -z "$PORT" ]
  then
    PORT=2000
    echo "PORT not set.  Will default to $PORT"
  fi

  if [ -z "$THREADS" ]
  then
    np=`nproc --all`
    THREADS=$((np * 2))
    echo "THREADS not set.  Will default to $THREADS"
  fi

  if [ -z "$VERSION_HISTORY_DATABASE" ]
  then
    VERSION_HISTORY_DATABASE="versionHistory"
    echo "VERSION_HISTORY_DATABASE not set.  Will default to $VERSION_HISTORY_DATABASE"
  fi

  if [ -z "$VERSION_HISTORY_COLLECTION" ]
  then
    VERSION_HISTORY_COLLECTION="entities"
    echo "VERSION_HISTORY_COLLECTION not set.  Will default to $VERSION_HISTORY_COLLECTION"
  fi

  if [ -z "$METRIC_DATABASE" ]
  then
    METRIC_DATABASE="versionHistory"
    echo "METRIC_DATABASE not set.  Will default to $METRIC_DATABASE"
  fi

  if [ -z "$METRIC_COLLECTION" ]
  then
    METRIC_COLLECTION="metrics"
    echo "METRIC_COLLECTION not set.  Will default to $METRIC_COLLECTION"
  fi

  if [ -z "$LOG_LEVEL" ]
  then
    LOG_LEVEL="info"
    echo "LOG_LEVEL not set.  Will default to $LOG_LEVEL"
  fi

  if [ -z "$LOG_ASYNC" ]
  then
    LOG_ASYNC="true"
    echo "LOG_ASYNC not set.  Will default to $LOG_ASYNC"
  fi
}

Ilp()
{
  ILP=""

  if [ -n "$ILP_SERVER" ]
  then
    ILP=" --ilp-server $ILP_SERVER"
  fi

  if [ -n "$ILP_PORT" ]
  then
    ILP="$ILP --ilp-port $ILP_PORT"
  fi

  if [ -n "$ILP_NAME" ]
  then
    ILP="$ILP --ilp-series-name $ILP_NAME"
  fi
}

Service()
{
  if [ ! -d $LOGDIR ]
  then
    mkdir -p $LOGDIR
    chown spt:spt $LOGDIR
  fi

  echo "Starting up Mongo socket server"
  /opt/spt/bin/mongo-service --console true --dir ${LOGDIR}/ \
    --mongo-uri $MONGO_URI \
    --version-history-database $VERSION_HISTORY_DATABASE \
    --version-history-collection $VERSION_HISTORY_COLLECTION \
    --metric-database $METRIC_DATABASE \
    --metric-collection $METRIC_COLLECTION \
    --port $PORT --threads $THREADS --log-level $LOG_LEVEL --log-async $LOG_ASYNC \
    $ILP
}

ConfigDb && SecureConfigDb && Check && Defaults && Ilp && Service
