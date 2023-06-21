FROM alpine:latest as builder

RUN apk --no-cache add build-base bash automake autoconf linux-headers pkgconfig
WORKDIR /app
RUN wget https://github.com/ntop/n2n/archive/refs/tags/3.0.tar.gz
RUN tar -xzvf 3.0.tar.gz
WORKDIR /app/n2n-3.0
RUN ./autogen.sh && ./configure
RUN make -j

FROM alpine:latest
WORKDIR /app
COPY --from=0 /app/n2n-3.0/supernode .
EXPOSE 7654/tcp
EXPOSE 7654/udp
CMD ["./supernode","-f","-c","community.list"]