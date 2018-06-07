FROM base/devel

RUN pacman --noconfirm -Syu git cmake boost

WORKDIR /app
COPY . .

# Restbed needs some custom built OpenSSL
RUN cd /app/extern/restbed/dependency/openssl && \
  ./config && make -j $(nproc)

RUN cd extern/restbed && \
  git apply ../../cmake/restbed.patch

RUN mkdir -p build && \
  cd /app/build && \
  cmake ..

RUN make -j $(nproc)
