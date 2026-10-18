#!/bin/bash
#
# Login leak tests: the login paths that own the email and famTarget
# strings, from the first message to a spawned or rejected player.
#
# Usage: runLoginTest.sh functional|leaks [case ...]
#
#   functional  run the cases against the server
#   leaks       run the cases with the server under valgrind, then stop
#               the server cleanly with connections still waiting, and
#               also fail on "definitely lost" memory
#   case ...    run only these cases (default: all)
#
# Builds OneLifeServer, runs one server in a temporary directory, and
# plays the cases below against it with /dev/tcp clients.  Each case uses
# its own emails and twin codes, and checks the replies its clients get
# and the server log lines it expects.
#
# Cases (name: what is sent -> what is expected):
#   emailCase           LOGIN MiXeD@... -> email lowercased
#   seedOnly            LOGIN |seed -> email becomes blank_email
#   famTargetUnknown    LOGIN email:family, no such family -> ACCEPTED,
#                       then REJECTED
#   twinFamTarget       twin party with unknown family -> all REJECTED
#   eveName             Eve says "I AM name" -> named after the eveName
#                       setting plus a family name
#   famTargetExisting   LOGIN email:family, family has a fertile Eve ->
#                       born into it
#   famTargetOnly       LOGIN :family -> email becomes blank_email, born
#                       into the family (or, if the seedOnly life is still
#                       alive, reconnected to it)
#   twinFamTargetExisting  twin party with email:family -> all born into
#                       it
#
# Exit codes:
#   0  PASS
#   1  FAIL: at least one check failed
#   2  setup error (build, game data, server start, client connection,
#      an error in this script) or bad usage
#
# Environment:
#   ONELIFE_DATA_DIR   OneLifeData7 checkout, default ../OneLifeData7 next
#                      to the OneLife repo
#   KEEP_RUN_DIR=1     keep the server run directory afterwards
#   STARTUP_TIMEOUT    seconds to wait for the server to start (default 600)
#
# Requires: g++, make, and valgrind for leaks


set -u

# the famTarget cases with an existing family go last: they wait for
# their Eves to become fertile, and other logins could be born to them
ALL_CASES="emailCase seedOnly famTargetUnknown twinFamTarget eveName
    famTargetExisting famTargetOnly twinFamTargetExisting"

usage() {
    echo "Usage: $( basename "$0" ) functional|leaks [case ...]"
    echo "Cases: $( echo $ALL_CASES )"
}

case "${1:-}" in
    functional|leaks) MODE=$1; shift ;;
    -h|--help)        usage; exit 0 ;;
    *)                usage >&2; exit 2 ;;
esac

CASES="${*:-$ALL_CASES}"


# Only finish() exits with 0 or 1.  Any other exit (fail_setup, a set -u
# error, a failed command) becomes exit 2.
RESULT=""

finish() {
    RESULT=$1
    exit "$1"
}

fail_setup() {
    echo "SETUP ERROR: $*" >&2
    exit 2
}

# replaced below once there is a server and run directory to clean up
cleanup() {
    :
}

onExit() {
    local status=$?
    cleanup
    if [ -z "$RESULT" ] && [ "$status" -ne 2 ]; then
        echo "SETUP ERROR: test script stopped unexpectedly (exit $status)" >&2
        exit 2
    fi
}
trap onExit EXIT


SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SERVER_DIR="$( cd "$SCRIPT_DIR/../.." && pwd )"
REPO_DIR="$( cd "$SERVER_DIR/.." && pwd )"

STARTUP_TIMEOUT="${STARTUP_TIMEOUT:-600}"
EVE_NAME="TESTEVE"

TOOLS="make g++"
[ "$MODE" = "leaks" ] && TOOLS="$TOOLS valgrind"
for tool in $TOOLS; do
    command -v "$tool" > /dev/null || fail_setup "$tool not found"
done



# ---- build ----------------------------------------------------------------

echo "== Building OneLifeServer"
(
    cd "$SERVER_DIR" || exit 1
    if [ ! -f Makefile ]; then
        ./configure 1 > /dev/null || exit 1
    fi
    make -j"$( nproc )" > /dev/null
) || fail_setup "server build failed"

SERVER_BIN="$SERVER_DIR/OneLifeServer"
[ -x "$SERVER_BIN" ] || fail_setup "no server binary at $SERVER_BIN"



# ---- run directory --------------------------------------------------------

DATA_ITEMS="objects transitions categories tutorialMaps dataVersionNumber.txt"
ONELIFE_DATA_DIR="${ONELIFE_DATA_DIR:-$REPO_DIR/../OneLifeData7}"

for item in $DATA_ITEMS; do
    [ -e "$ONELIFE_DATA_DIR/$item" ] ||
        fail_setup "no $item in $ONELIFE_DATA_DIR (set ONELIFE_DATA_DIR to an OneLifeData7 checkout)"
done
ONELIFE_DATA_DIR="$( cd "$ONELIFE_DATA_DIR" && pwd )"
echo "== Using game data from $ONELIFE_DATA_DIR"

RUN_DIR="$( mktemp -d "${TMPDIR:-/tmp}/loginTest.XXXXXX" )"
SERVER_PID=""

# client name -> socket fd, reader pid
declare -A CLIENT_FD
declare -A READER_PID

cleanup() {
    local name
    for name in "${!CLIENT_FD[@]}"; do
        closeClient "$name"
    done
    [ -n "$SERVER_PID" ] && kill -KILL "$SERVER_PID" 2> /dev/null
    if [ "${KEEP_RUN_DIR:-0}" = "1" ]; then
        echo "== Run directory kept: $RUN_DIR"
    else
        rm -rf "$RUN_DIR"
    fi
}

cp -r "$SERVER_DIR/settings" "$RUN_DIR/"
cp "$SERVER_DIR/firstNames.txt" "$SERVER_DIR/lastNames.txt" "$RUN_DIR/"
for item in $DATA_ITEMS; do
    ln -s "$ONELIFE_DATA_DIR/$item" "$RUN_DIR/$item"
done

# no outside servers, no passwords
for setting in requireTicketServerCheck requireClientPassword \
               useCurseServer useStatsServer useLifeTokenServer \
               useFitnessServer; do
    echo 0 > "$RUN_DIR/settings/$setting.ini"
done
# a seeded login spawns an Eve
echo 1 > "$RUN_DIR/settings/forceEveOnSeededSpawn.ini"
# not the default EVE, so eveName can tell the setting is used
echo "$EVE_NAME" > "$RUN_DIR/settings/eveName.ini"

# first free port from 18005 up
PORT=18005
while ( exec 3<> "/dev/tcp/127.0.0.1/$PORT" ) 2> /dev/null; do
    PORT=$(( PORT + 1 ))
done
echo "$PORT" > "$RUN_DIR/settings/port.ini"



# ---- start server ---------------------------------------------------------

VG_LOG="$RUN_DIR/valgrind.txt"
RUNNER=()
if [ "$MODE" = "leaks" ]; then
    RUNNER=( valgrind --leak-check=full --show-leak-kinds=definite
             --log-file="$VG_LOG" )
    echo "== Starting server under valgrind on port $PORT (startup is slow)"
else
    echo "== Starting server on port $PORT"
fi
(
    cd "$RUN_DIR" || exit 1
    exec "${RUNNER[@]}" "$SERVER_BIN" > serverOut.txt 2>&1
) &
SERVER_PID=$!

waited=0
until grep -q "Listening for connection on port" "$RUN_DIR/log.txt" 2> /dev/null; do
    if ! kill -0 "$SERVER_PID" 2> /dev/null; then
        tail -20 "$RUN_DIR/log.txt" "$RUN_DIR/serverOut.txt" 2> /dev/null
        fail_setup "server exited during startup"
    fi
    [ "$waited" -ge "$STARTUP_TIMEOUT" ] &&
        fail_setup "server did not start within ${STARTUP_TIMEOUT}s"
    sleep 1
    waited=$(( waited + 1 ))
done
echo "== Server listening after ~${waited}s"



# ---- clients --------------------------------------------------------------

# Clients are /dev/tcp connections, named by the case.  After connecting
# and reading the server greeting, a background cat copies everything
# the server sends to $RUN_DIR/client_<name>.bin.  When the server
# closes the connection, that cat exits.

# openClient name
openClient() {
    local name=$1 fd reply
    exec {fd}<>"/dev/tcp/127.0.0.1/$PORT" ||
        fail_setup "$name: can't connect to port $PORT"
    CLIENT_FD[$name]=$fd

    IFS= read -r -d '#' -t 60 -u "$fd" reply
    [[ "$reply" == SN* ]] || fail_setup "$name: unexpected greeting '$reply'"

    # the reader must not hold other clients' sockets, or closing those
    # clients would not close their connections
    (
        for other in "${CLIENT_FD[@]}"; do
            [ "$other" = "$fd" ] || eval "exec $other>&-"
        done
        exec cat <&"$fd"
    ) > "$RUN_DIR/client_$name.bin" &
    READER_PID[$name]=$!
}

# the reader holds a copy of the socket, so it has to go too
closeClient() {
    local name=$1
    [ -n "${CLIENT_FD[$name]:-}" ] || return 0
    kill "${READER_PID[$name]}" 2> /dev/null
    wait "${READER_PID[$name]}" 2> /dev/null
    eval "exec ${CLIENT_FD[$name]}>&-"
    unset "CLIENT_FD[$name]" "READER_PID[$name]"
}

# send name text
send() {
    echo "  $1 sends '${2//$'\n'/\\n}'"
    echo -n "$2" >&"${CLIENT_FD[$1]}"
}

# login name email [tutorial [twinCode twinCount]]
# email may carry |seed or :famTarget
login() {
    local name=$1 msg="LOGIN $2 aaaa aaaa"
    [ $# -ge 3 ] && msg="$msg $3"
    [ $# -ge 5 ] && msg="$msg $4 $5"
    openClient "$name"
    send "$name" "$msg#"
}



# ---- checks ---------------------------------------------------------------

# log lines of the current case only
CASE_LOG_START=0
caseLog() {
    tail -n +$(( CASE_LOG_START + 1 )) "$RUN_DIR/log.txt"
}

logHas() {
    caseLog | grep -q -F -- "$1"
}

replyStartsWith() {
    [ "$( head -c "${#2}" "$RUN_DIR/client_$1.bin" 2> /dev/null )" = "$2" ]
}

replyHas() {
    grep -a -q -F -- "$2" "$RUN_DIR/client_$1.bin" 2> /dev/null
}

# game data = anything after the ACCEPTED message
ACCEPTED=$'ACCEPTED\n#'
hasGameData() {
    replyStartsWith "$1" "$ACCEPTED" &&
        [ "$( stat -c %s "$RUN_DIR/client_$1.bin" )" -gt "${#ACCEPTED}" ]
}

# true once the server has closed the connection
isClosed() {
    ! kill -0 "${READER_PID[$1]:-}" 2> /dev/null
}

not() {
    ! "$@"
}

# player id from "New player <email> connected as player <id>"
playerId() {
    caseLog | grep -F "New player $1 connected as player " | tail -1 |
        sed 's/.* connected as player \([0-9]*\).*/\1/'
}

CASE_FAILED=0
FAILED_CASES=""

# check what timeout command...
# runs command every 0.1s until it succeeds; after timeout seconds (0:
# at once) the check fails
check() {
    local what=$1 limit=$(( $2 * 10 )) n=0
    shift 2
    until "$@"; do
        n=$(( n + 1 ))
        if [ "$n" -ge "$limit" ]; then
            echo "  FAIL  $what"
            CASE_FAILED=1
            return 1
        fi
        sleep 0.1
    done
    echo "  ok    $what"
}

# shown after a failed check
showLog() {
    echo "        last log lines of this case:"
    caseLog | tail -n 5 | cut -c 1-120 | sed 's/^/        | /'
}

showClient() {
    local state="open"
    isClosed "$1" && state="closed by server"
    echo "        $1 received $( stat -c %s "$RUN_DIR/client_$1.bin" )" \
         "bytes, connection $state:"
    head -c 200 "$RUN_DIR/client_$1.bin" | tr -d '\000' | head -n 3 |
        sed 's/^/        | /'
}

expectLog() {
    check "log: $1" "${2:-15}" logHas "$1" || showLog
}

expectNoLog() {
    check "no log: $1" 0 not logHas "$1"
}

expectNewLife() {
    expectLog "New player $1 connected as player"
}

expectAccepted() {
    check "$1 got ACCEPTED" 15 replyStartsWith "$1" "$ACCEPTED" ||
        showClient "$1"
}

expectRejected() {
    check "$1 got REJECTED" 15 replyHas "$1" "REJECTED" || showClient "$1"
    check "$1 connection closed by server" 15 isClosed "$1"
}

expectGameData() {
    check "$1 got game data" 15 hasGameData "$1" || showClient "$1"
}



# ---- cases ----------------------------------------------------------------

case_emailCase() {
    login emailCase MiXeD@Test.COM
    expectAccepted emailCase
    expectNewLife mixed@test.com
}

case_seedOnly() {
    login seedOnly "|otherSeed"
    expectAccepted seedOnly
    expectNewLife blank_email
}

case_famTargetUnknown() {
    login famTarget "famtarget@test.com:noSuchFamily"
    expectAccepted famTarget
    expectRejected famTarget
    expectLog "cause: Target family is not found"
    expectNoLog "New player famtarget@test.com"
}

case_twinFamTarget() {
    login twinFamA "twinfama@test.com:noSuchFamily" 0 twinFamCode 2
    login twinFamB "twinfamb@test.com:noSuchFamily" 0 twinFamCode 2
    expectRejected twinFamA
    expectRejected twinFamB
    expectNoLog "New player twinfam"
}


# ---- cases with an existing family ----------------------------------------

# namedEve client email word: client logs in as an Eve (a seeded login)
# and says "I AM word"; sets EVE_ID to her player id and EVE_FAMILY to
# her family name, empty if she got no name
EVE_ID=""
EVE_FAMILY=""

namedEve() {
    local name=$1 email=$2 word=$3 nameRe
    login "$name" "$email"
    expectNewLife "${email%%|*}"
    expectGameData "$name"
    EVE_ID="$( playerId "${email%%|*}" )"
    # a SAY within minSayGapInSeconds (1s) of the spawn is ignored
    sleep 2
    send "$name" "SAY 0 0 I AM $word#"

    # the NM message has a line "<id> <eveName> <family>"
    nameRe="^$EVE_ID $EVE_NAME [^ ]+"
    check "$name is named '$EVE_NAME <family>'" 15 \
          grep -a -q -E "$nameRe" "$RUN_DIR/client_$name.bin"
    EVE_FAMILY="$( grep -a -o -E "$nameRe" "$RUN_DIR/client_$name.bin" |
                   tail -1 | sed 's/.* //' )"
}

case_eveName() {
    namedEve eveNameEve "evenameeve@test.com|eveNameSeed" SOMENAME
}

# Eves spawn at 14 and are fertile at 15, one year is 60 seconds, but an
# idle player starves after about 90 seconds.  So these Eves spawn at
# 14.9 (forceEveAge, which the server reads again for every Eve) and are
# fertile 6 seconds later.  The first case that needs a family spawns all
# three Eves, so there is only one wait.  Case name -> family name, and
# player id of its Eve.
declare -A FAMILY
declare -A FAMILY_EVE
FAMILIES_BORN=0

fertileFamily() {
    local c ageFile="$RUN_DIR/settings/forceEveAge.ini" oldAge
    if [ "$FAMILIES_BORN" -eq 0 ]; then
        echo "  (spawning a named Eve, age 14.9, for each famTarget case)"
        oldAge="$( cat "$ageFile" )"
        echo 14.9 > "$ageFile"
        for c in famTargetExisting famTargetOnly twinFamTargetExisting; do
            namedEve "${c}Eve" "${c,,}eve@test.com|${c}Seed" "${c^^}"
            FAMILY[$c]=$EVE_FAMILY
            FAMILY_EVE[$c]=$EVE_ID
        done
        echo "$oldAge" > "$ageFile"
        FAMILIES_BORN=$SECONDS
    fi
    local left=$(( FAMILIES_BORN + 8 - SECONDS ))
    if [ "$left" -gt 0 ]; then
        echo "  (waiting ${left}s: the Eves become fertile)"
        sleep "$left"
    fi
    [ -n "${FAMILY[$CASE]}" ] || fail_setup "$CASE: its Eve has no family name"
}

# expectMother email: the life log has a birth of email to the Eve of
# this case
bornTo() {
    cat "$RUN_DIR"/lifeLog/*.txt 2> /dev/null |
        grep -a -q -E "^B [0-9]+ [0-9]+ $1 .* parent=$2,"
}

expectMother() {
    check "lifeLog: $1 born to player ${FAMILY_EVE[$CASE]}" 15 \
          bornTo "$1" "${FAMILY_EVE[$CASE]}"
}

case_famTargetExisting() {
    fertileFamily
    login famKid "famkid@test.com:${FAMILY[$CASE],,}"
    expectAccepted famKid
    expectNewLife famkid@test.com
    expectGameData famKid
    expectMother famkid@test.com
    expectNoLog "Target family is not found"
}

# blank_email is one account for every login without an email: if the
# seedOnly life is still alive, this login reconnects to it
blankBornOrReconnected() {
    logHas "New player blank_email connected as player" ||
        logHas "(blank_email) has reconnected."
}

case_famTargetOnly() {
    fertileFamily
    login famOnly ":${FAMILY[$CASE]}"
    expectAccepted famOnly
    check "log: blank_email born or reconnected" 15 blankBornOrReconnected ||
        showLog
    expectGameData famOnly
    logHas "New player blank_email" && expectMother blank_email
    expectNoLog "Target family is not found"
}

case_twinFamTargetExisting() {
    fertileFamily
    local fam="${FAMILY[$CASE]}"
    login twinFamExA "twinfamexa@test.com:$fam" 0 twinFamExCode 2
    login twinFamExB "twinfamexb@test.com:$fam" 0 twinFamExCode 2
    expectLog "Found 2 other people waiting for twin party of"
    expectNewLife twinfamexa@test.com
    expectNewLife twinfamexb@test.com
    expectGameData twinFamExA
    expectGameData twinFamExB
    expectMother twinfamexa@test.com
    expectMother twinfamexb@test.com
    expectNoLog "Target family is not found"
}



# ---- run ------------------------------------------------------------------

for c in $CASES; do
    declare -F "case_$c" > /dev/null || { usage >&2; exit 2; }
done

# endCase: records the result of the current case
endCase() {
    if [ "$CASE_FAILED" -eq 1 ]; then
        FAILED_CASES="$FAILED_CASES $CASE"
        echo "== $CASE: FAIL"
    else
        echo "== $CASE: PASS"
    fi
}

sleep 2

for CASE in $CASES; do
    echo
    echo "== $CASE"
    CASE_FAILED=0
    CASE_LOG_START="$( wc -l < "$RUN_DIR/log.txt" )"
    "case_$CASE"
    endCase
done



# ---- leaks: shutdown and valgrind report ----------------------------------

# With connections still waiting, the server is stopped cleanly, so that
# valgrind can tell memory the server forgot to free from memory still
# in use.  Any "definitely lost" memory fails.

if [ "$MODE" = "leaks" ]; then
    CASE=leaks
    echo
    echo "== $CASE"
    CASE_FAILED=0

    # a twin with a famTarget waiting for its party, a login waiting for
    # its message
    login pendingTwin "pendingtwin@test.com:pendingFamily" 0 pendingCode 2
    expectAccepted pendingTwin
    openClient pendingLogin

    echo "  server gets SIGTSTP, its clean quit signal"
    kill -TSTP "$SERVER_PID"
    check "server quit" 300 not kill -0 "$SERVER_PID" 2> /dev/null
    wait "$SERVER_PID" 2> /dev/null
    SERVER_PID=""

    if check "valgrind wrote a leak report" 0 grep -q "HEAP SUMMARY" "$VG_LOG"
    then
        # no LEAK SUMMARY at all when every block was freed
        lost="$( sed -n 's/.*definitely lost: \([0-9,]*\) bytes.*/\1/p' \
                 "$VG_LOG" | tr -d , )"
        if ! check "valgrind: no definitely lost bytes (${lost:-0})" 0 \
                   [ "${lost:-0}" -eq 0 ]; then
            sed -e 's/^==[0-9]*== \{0,1\}//' "$VG_LOG" |
                sed -n '/are definitely lost in loss record/,/^$/p' |
                sed 's/^/        /'
        fi
    fi
    endCase
fi



# ---- result ---------------------------------------------------------------

echo
if [ -n "$FAILED_CASES" ]; then
    echo "FAIL:$FAILED_CASES"
    [ "${KEEP_RUN_DIR:-0}" = "1" ] ||
        echo "Rerun with KEEP_RUN_DIR=1 to keep the server log."
    finish 1
fi
echo "PASS"
finish 0
