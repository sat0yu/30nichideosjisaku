FROM alpine

RUN apk update && \
    apk --no-cache add \
        build-base \
        nasm \
        mtools

WORKDIR /haribos

CMD ["/bin/ash"]
