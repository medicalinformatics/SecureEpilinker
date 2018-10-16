curl_json_file() {
  url=${3}
  if [[ ${url:0:5} == https ]]; then
    flags=-k
  fi
  if [[ -n "${4}" ]]; then
    auth=-H
    key="Authorization: ${4}"
  fi
  curl -v $flags $auth "$key" -H "Expect:" -H "Content-Type: application/json" --data "@${2}" -X $1 "${url}"
}

local_init() {
  id=${1}
  port=${2:-8161}
  host=${3:-127.0.0.1}
  db_port=${4:-8800}
  db_host=${5:-127.0.0.1}

  curl_json_file PUT <(sed -e "s/{{id}}/${id}/g" \
      -e "s/{{port}}/${db_port}/g" \
      -e "s/{{host}}/${db_host}/g" \
      < configurations/local_init.template.json) \
    "https://${host}:${port}/initLocal/"
}

remote_init() {
  id=${1}
  port=${2:-8161}
  host=${3:-127.0.0.1}
  remote_port=${4:-8161}
  remote_host=${5:-127.0.0.1}

  curl_json_file PUT <(sed \
      -e "s/{{port}}/${remote_port}/g" \
      -e "s/{{host}}/${remote_host}/g" \
      < configurations/remote_init.template.json) \
    "https://${host}:${port}/initRemote/${id}"
}

sel_test_conn() {
  id=${1}
  port=${2:-8161}
  host=${3:-127.0.0.1}

  curl -v -k "https://${host}:${port}/test/${id}"
}

mpc_action() {
  action=${1}
  id=${2}
  port=${3:-8161}
  host=${4:-127.0.0.1}
  callback=${5:-http://localhost:8800/${action}Callback}

  curl_json_file POST <(sed \
      -e "s^{{callback}}^${callback}^g" \
      < configurations/linkRecord.template.json) \
    "https://${host}:${port}/${action}Record/${id}" \
    "apiKey apiKey=\"123abc\""
}

link_record() {
  mpc_action link ${@}
}

match_record() {
  mpc_action match ${@}
}
