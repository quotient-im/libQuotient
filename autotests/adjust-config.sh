#!/bin/bash

CMD=""

$CMD sed -i 's/tls: false/tls: true/g' homeserver.yaml

(
cat <<HEREDOC

rc_message:
  per_second: 10000
  burst_count: 100000
rc_registration:
  per_second: 10000
  burst_count: 30000
rc_login:
  address:
    per_second: 10000
    burst_count: 30000
  account:
    per_second: 10000
    burst_count: 30000
  failed_attempts:
    per_second: 10000
    burst_count: 30000
rc_admin_redaction:
  per_second: 1000
  burst_count: 5000
rc_joins:
  local:
    per_second: 10000
    burst_count: 100000
  remote:
    per_second: 10000
    burst_count: 100000
tls_certificate_path: "/data/localhost.tls.crt"
tls_private_key_path: "/data/localhost.tls.key"
HEREDOC
) | $CMD tee -a homeserver.yaml

$CMD openssl req -x509 -newkey rsa:4096 -keyout localhost.tls.key -out localhost.tls.crt -days 365 -subj '/CN=localhost' -nodes

$CMD chmod 0777 localhost.tls.crt
$CMD chmod 0777 localhost.tls.key

cp ../register-users.sh .
