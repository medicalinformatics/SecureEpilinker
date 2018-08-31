curl_json_file() {
  url=${3}
  if [[ ${url:0:5} == https ]]; then
    flags=-k
  fi
  curl -v $flags -H "Expect:" -H "Content-Type: application/json" --data "@${2}" -X $1 "${3}"
}

local_init() {
  id=${1}
  port=${2:-8161}
  host=${3:-localhost}
  db_port=${4:-8800}
  db_host=${5:-localhost}

  curl_json_file PUT <(sed -e "s/{{id}}/${id}/g" \
      -e "s/{{port}}/${db_port}/g" \
      -e "s/{{host}}/${db_host}/g" \
      < configurations/local_init.template.json) \
    "https://${host}:${port}/initLocal/"

  #curl -v -k -H "Expect:" -H "Content-Type: application/json" \
    #--data @<(sed -e "s/{{id}}/${local_id}/g" \
      #-e "s/{{port}}/${local_port}/g" \
      #-e "s/{{host}}/${local_host}/g" \
      #< configurations/local_init.template.json) \
    #-X PUT "https://${local_host}:${local_port}/initLocal/"
}

remote_init() {
  id=${1}
  port=${2:-8161}
  host=${3:-localhost}
  remote_port=${4:-8161}
  remote_host=${5:-localhost}

  curl_json_file PUT <(sed \
      -e "s/{{port}}/${remote_port}/g" \
      -e "s/{{host}}/${remote_host}/g" \
      < configurations/remote_init.template.json) \
    "https://${host}:${port}/initRemote/${id}"
}

sel_test_conn() {
  id=${1}
  port=${2:-8161}
  host=${3:-localhost}

  curl -v -k "https://${host}:${port}/test/${id}"
}

link_record() {
  id=${1}
  port=${2:-8161}
  host=${3:-localhost}
  callback=${4:-no_callback_defined}

  curl_json_file POST <(sed \
      -e "s/{{callback}}/${callback}/g" \
      < configurations/linkRecord.template.json) \
    "https://${host}:${port}/linkRecord/${id}"
}
