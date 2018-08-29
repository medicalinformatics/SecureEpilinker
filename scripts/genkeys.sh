#!/bin/sh

target_dir=${1:-../data}

openssl req -new -newkey rsa:1024 -days 3650 -nodes -x509 \
    -subj "/C=DE/ST=Hess/L=Darmstadt/O=TUDA/CN=sel.local" \
    -keyout "${target_dir}/server.key" \
    -out "${target_dir}/server.crt" \
&& openssl dhparam -out "${target_dir}/dh768.pem" 768
