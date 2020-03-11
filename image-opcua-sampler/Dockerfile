FROM intersystemsdc/irisdemo-base-gcc:version-1.0.0 as gcc

ADD ./src /tmp/src/

WORKDIR /tmp/src

RUN mkdir ./build && \
    cd ./build && \
    cmake ../ && \
    make
 
FROM library/ubuntu

WORKDIR /app

COPY --from=gcc /tmp/src/build/opcua-sampler /app

ENTRYPOINT [ "/app/opcua-sampler" ]