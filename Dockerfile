FROM ubuntu:20.04
ENV TZ=Russia/Moscow

RUN apt-get -y update && apt-get install -y sudo gcc make cmake libev-dev g++

ADD . .
COPY httptest /var/www/html/httptest

RUN chmod +x ./build.sh && ./build.sh

EXPOSE 80
WORKDIR /
ENTRYPOINT [ "./build/server" ]
