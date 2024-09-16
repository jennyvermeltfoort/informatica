#!/bin/bash
COUNTER=0
TEST=1

if [ "$#" -eq  "0" ]; then
    TEST=0
fi;

do_test () {
    let COUNTER++

    if [ $TEST -eq 1 ]; then
        echo -e "$1" | ./main.o
    fi;

    echo -e "$1" | ./main.o | grep -q "$2" && echo "Test ${COUNTER} Ok" || (echo "Test ${COUNTER} Failed" && echo -e "$1" | ./main.o)
}

echo ""
echo "Test happy flow exact study."
do_test "1994 \n 2 \n 28 \n m \n 91823 \n 91823 \n 91823" "geschikt voor een exacte studie"

echo ""
echo "Test happy flow beta study."
do_test "1994 \n 2 \n 28 \n m \n 1 \n b" "u bent geschikt voor een beta studie"

echo ""
echo "Test happy flow: 31 december 2000 is zondag."
do_test "2000 \n 12 \n 31 \n z \n o \n 1 \n b" "je bent geschikt voor een beta studie"

echo ""
echo "Test happy flow: 1 jan 2000 is zaterday."
do_test "2000 \n 1 \n 1 \n z \n a \n 1 \n b" "je bent geschikt voor een beta studie"

echo ""
echo "Test happy flow: 1 jan 1877 is zondag."
do_test "1877 \n 1 \n 1 \n z \n o \n 1 \n b" "u bent geschikt voor een beta studie"

# Very ugly but whatever, what works, works.
echo ""
echo "Test happy flow: youngest possible (now - 10 years)."
let COUNTER++
YEAR="$(($(date '+%Y') -10))"
MONTH="$(date '+%-m')"
DAY="$(date '+%-d')"
DAY_STR_1="$(LC_TIME="nl_NL.UTF-8" date -d "${YEAR}-${MONTH}-${DAY}" "+%A" | cut -b 1)"
DAY_STR_2="$(LC_TIME="nl_NL.UTF-8" date -d "${YEAR}-${MONTH}-${DAY}" "+%A" | cut -b 2)"
echo -e "${YEAR} \n ${MONTH} \n ${DAY} \n ${DAY_STR_1} \n ${DAY_STR_2} \n 1 \n b" | ./main.o | grep -q "je bent geschikt voor een beta studie" && echo "Test ${COUNTER} Ok" || ( \
    echo -e "${YEAR} \n ${MONTH} \n ${DAY} \n ${DAY_STR_1} \n 1 \n b" | ./main.o | grep -q "je bent geschikt voor een beta studie" && echo "Test ${COUNTER} Ok" || ( \
        echo "Test ${COUNTER} Failed" && echo -e "${YEAR} \n ${MONTH} \n ${DAY} \n ${DAY_STR_1} \n 1 \n b" | ./main.o))

# Very ugly but whatever, what works, works.
echo ""
echo "Test happy flow: Barely too young (now - 10 years +1 day.)."
let COUNTER++
YEAR="$(($(date '+%Y') -10))"
MONTH="$(date '+%-m')"
DAY="$(($(date '+%-d') + 1))"
DAY_STR_1="$(LC_TIME="nl_NL.UTF-8" date -d "${YEAR}-${MONTH}-${DAY}" "+%A" | cut -b 1)"
DAY_STR_2="$(LC_TIME="nl_NL.UTF-8" date -d "${YEAR}-${MONTH}-${DAY}" "+%A" | cut -b 2)"
echo -e "${YEAR} \n ${MONTH} \n ${DAY} \n ${DAY_STR_1} \n ${DAY_STR_2} \n 1 \n b" | ./main.o | grep -q "Je bent te jong om deze vragenlijst in te vullen" && echo "Test ${COUNTER} Ok" || ( \
        echo "Test ${COUNTER} Failed" && echo -e "${YEAR} \n ${MONTH} \n ${DAY} \n ${DAY_STR_1} \n 1 \n b" | ./main.o)

echo ""
echo "Test happy flow schrikkeljaar, beta study."
do_test "2000 \n 2 \n 29 \n d \n i \n 1 \n b" "je bent geschikt voor een beta studie."

echo ""
echo "Test happy flow when user is not suitable for a study."
do_test "1994 \n 2 \n 28 \nm \n 1 \n c" "u bent waarschijnlijk niet geschikt voor een opleiding aan de universiteit"

echo ""
echo "Test fail state invalid day string. 1994-2-28 should be 'm'"
do_test "1994 \n 2 \n 28 \n d \n i" "De opgegeven dag is niet valide, gegevens verwijderd!"

echo ""
echo "Test fail state invalid day string in schikkeljaar. 2000-2-29 should be 'd' 'i'"
do_test "2000 \n 2 \n 29 \n d \n o" "De opgegeven dag is niet valide, gegevens verwijderd!"

echo ""
echo "Test fail state when user is too young."
do_test "2023" "Je bent te jong"

echo ""
echo "Test fail state invalid input: year"
do_test "a" "gegeven informatie is niet valide"

echo ""
echo "Test fail state invalid input: month"
do_test "1994 \n a" "gegeven informatie is niet valide"

echo ""
echo "Test fail state invalid input: day"
do_test "1994 \n 2 \n a" "gegeven informatie is niet valide"

echo ""
echo "Test fail state invalid input: day_string"
do_test "1994 \n 2 \n 28 \n a" "gegeven informatie is niet valide"

echo ""
echo "Test fail state invalid input: day_string -> d"
do_test "2000 \n 2 \n 29 \n d \n a" "gegeven informatie is niet valide" 

echo ""
echo "Test fail state invalid input: day_string -> z"
do_test "2000 \n 2 \n 29 \n z \n d" "gegeven informatie is niet valide" 

echo ""
echo "Test fail state invalid input: math input teller"
do_test "1994 \n 2 \n 28 \n m \n a" "gegeven informatie is niet valide" 

echo ""
echo "Test fail state invalid input: math input noemer"
do_test "1994 \n 2 \n 28 \n m \n 91823 \n a" "gegeven informatie is niet valide" 

echo ""
echo "Test fail state invalid input: math input float"
do_test "1994 \n 2 \n 28 \n m \n 91823 \n 91823 \n a" "gegeven informatie is niet valide" 

echo ""
echo "Test fail state invalid input: literature question."
do_test "1994 \n 2 \n 28 \n m \n 1 \n e" "gegeven informatie is niet valide" 