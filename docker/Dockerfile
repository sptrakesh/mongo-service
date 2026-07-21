FROM sptrakesh/config-db:alpine as config
FROM sptrakesh/encrypter as encrypter
FROM sptrakesh/mongocxx-boost as base

RUN apk add --no-cache readline-dev
COPY CMakeLists.txt /opt/spt/mongo-service/
COPY MongoServiceConfig.cmake /opt/spt/mongo-service/
COPY src /opt/spt/mongo-service/src
COPY docker/env.sh /opt/spt/mongo-service/docker/

WORKDIR /opt/spt/mongo-service/build
RUN rm -rf * \
  && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/local -DBUILD_TESTING=OFF .. \
  && make -j8 \
  && make install

FROM sptrakesh/cppruntime

COPY --from=config /opt/spt/bin /opt/spt/bin
COPY --from=config /opt/spt/certs/ca.crt /opt/spt/certs/ca.crt
COPY --from=config /opt/spt/certs/client.crt /opt/spt/certs/client.crt
COPY --from=config /opt/spt/certs/client.key /opt/spt/certs/client.key
COPY --from=encrypter /opt/spt/bin /opt/spt/bin
COPY --from=base /opt/spt/bin /opt/spt/bin
COPY --from=base /opt/spt/lib /opt/spt/lib
COPY --from=base /opt/spt/include /opt/spt/include

RUN apk add --no-cache readline \
  && addgroup spt \
  && adduser -DS -g "SPT User" -G spt -s /bin/sh -h /home/spt spt \
  && mkdir -p /opt/spt/logs /opt/spt/data \
  && chown spt:spt /opt/spt/logs /opt/spt/data

COPY docker/scripts/entrypoint.sh /opt/spt/bin
COPY docker/scripts/gdb.sh /tmp

WORKDIR /opt/spt
ENV LD_LIBRARY_PATH=/usr/lib
USER spt
ENTRYPOINT [ "/opt/spt/bin/entrypoint.sh" ]