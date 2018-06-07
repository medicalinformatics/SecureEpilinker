FROM base/devel:latest as build

RUN pacman --noconfirm -Syu git cmake boost

WORKDIR /app
COPY cmake cmake
COPY extern extern
COPY include include
COPY test test
COPY test_scripts test_scripts
COPY .git .git
COPY .gitmodules CMakeLists.txt sepilinker.cpp /app/

# Restbed needs some custom built OpenSSL
RUN cd /app/extern/restbed/dependency/openssl && \
  ./config && make -j $(nproc)

RUN cd extern/restbed && \
  git apply ../../cmake/restbed.patch || true

# remove cmake files if build context already called cmake
RUN rm -rf build && mkdir -p build && \
  cd build && \
  cmake ..

# build!
RUN cd build && \
  make -j $(nproc)

# copy dependencies
RUN mkdir /deps; \
  echo build/sel | while read bin; do \
    ldd $bin | cut -d' ' -f 3 | grep /usr/lib | while read lib; do \
      cp $lib /deps; \
    done; \
  done

# copy everything into minimal image
FROM gcr.io/distroless/base
# debug
#FROM base/devel
COPY --from=build /app/build/sel /app/sel
COPY --from=build /deps /deps

# copy config and circuit files
COPY extern/ABY/bin/circ/int_div_16.aby /app/circ/
COPY data /data

# let the minimal image find libraries in the correct order
ENV LD_LIBRARY_PATH=/lib/x86_64-linux-gnu/:/deps
# expose default ports
EXPOSE 8080

ENTRYPOINT ["/app/sel"]
