#!/bin/sh

METRIC_DATABASE="versionHistory"
METRIC_COLLECTION="metrics"
PORT=2000
THREADS=4
gdb -ex run --args /opt/spt/bin/mongo-service --console true --dir ${LOGDIR}/ \
    --mongo-uri $MONGO_URI \
    --version-history-database $VERSION_HISTORY_DATABASE \
    --version-history-collection $VERSION_HISTORY_COLLECTION \
    --metric-database $METRIC_DATABASE \
    --metric-collection $METRIC_COLLECTION \
    --port $PORT --threads $THREADS