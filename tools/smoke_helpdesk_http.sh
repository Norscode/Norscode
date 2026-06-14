#!/usr/bin/env sh

set -eu

BASE_URL="${1:-http://127.0.0.1:4173}"

headers_common='X-API-Key: secret-helpdesk'

request() {
    navn="$1"
    metode="$2"
    url="$3"
    ekstra_hode="$4"
    body="${5:-}"

    status_code=$(curl -sS -o /tmp/helpdesk_http_body.txt -w '%{http_code}' \
        -X "$metode" "$url" \
        -H "$headers_common" \
        ${ekstra_hode:+-H "$ekstra_hode"} \
        ${body:+-d "$body"} )

    echo "$status_code $navn $url"
}

assert_status() {
    status_line="$1"
    expected="$2"
    if [ "$status_line" != "$expected" ]; then
        printf 'FEIL: forventet status %s, fikk %s\n' "$expected" "$status_line" >&2
        if [ -f /tmp/helpdesk_http_body.txt ]; then
            printf 'Svarkropp: %s\n' "$(cat /tmp/helpdesk_http_body.txt)" >&2
        fi
        exit 1
    fi
}

if ! command -v curl >/dev/null 2>&1; then
    printf 'FEIL: curl mangler på maskinen. Trengs for runtime-smoke.\n' >&2
    exit 1
fi

printf '=== Helpdesk HTTP smoke ===\n'
printf 'Base-url: %s\n' "$BASE_URL"

if ! curl -sS --max-time 2 "$BASE_URL/health" >/dev/null; then
    printf 'FEIL: klarte ikkje nå %s/health. Start server på 4173 først.\n' "$BASE_URL" >&2
    exit 1
fi

# Health/metadata checks
status=$(request "health" GET "$BASE_URL/health" "")
status_code="${status%% *}"
assert_status "$status_code" "200"

status=$(request "tickets" GET "$BASE_URL/tickets" "")
assert_status "${status%% *}" "200"

status=$(request "tickets-invalid-sort" GET "$BASE_URL/tickets?sort=bad-sort" "-H x-Role: admin")
assert_status "${status%% *}" "400"

status=$(request "tickets-invalid-limit" GET "$BASE_URL/tickets?limit=abc" "-H x-Role: admin")
assert_status "${status%% *}" "400"

status=$(request "tickets-id-invalid" GET "$BASE_URL/tickets/bad-id" "-H x-Role: agent" "-H x-user: lena")
assert_status "${status%% *}" "404"

# Admin checks
status=$(request "admin-users" GET "$BASE_URL/admin/users" "")
assert_status "${status%% *}" "200"

status=$(curl -sS -o /tmp/helpdesk_http_body.txt -w '%{http_code}' -X GET "$BASE_URL/admin/roles" -H "X-Role: user")
assert_status "$status" "403"

status=$(curl -sS -o /tmp/helpdesk_http_body.txt -w '%{http_code}' -X GET "$BASE_URL/admin/roles")
assert_status "$status" "401"

status=$(request "admin-roles" GET "$BASE_URL/admin/roles" "")
assert_status "${status%% *}" "200"

status=$(curl -sS -o /tmp/helpdesk_http_body.txt -w '%{http_code}' -X PATCH "$BASE_URL/admin/tickets/1" \
    -H "$headers_common" -H "X-Role: agent" -H "X-user: lena" -H "Content-Type: application/json" \
    -d '{"status":"closed"}')
assert_status "$status" "403"

status=$(request "admin-metrics" GET "$BASE_URL/admin/metrics" "")
assert_status "${status%% *}" "200"

status=$(request "admin-panel" GET "$BASE_URL/admin/panel" "")
assert_status "${status%% *}" "200"

# Negative/admin-blocked
status=$(curl -sS -o /tmp/helpdesk_http_body.txt -w '%{http_code}' -X GET "$BASE_URL/admin/roles" -H "X-Role: user")
assert_status "$status" "403"

# Agent cannot hit admin override
status=$(curl -sS -o /tmp/helpdesk_http_body.txt -w '%{http_code}' -X PATCH "$BASE_URL/admin/tickets/1" \
    -H "$headers_common" -H "X-Role: agent" -H "Content-Type: application/json" \
    -d '{"status":"closed"}')
assert_status "$status" "403"

printf 'OK: Helpdesk HTTP smoke klar\n'
