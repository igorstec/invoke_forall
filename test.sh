#!/usr/bin/bash

GREEN='\033[1;32m'
RED='\033[1;31m'
RESET='\033[0m'
EXIT_CODE=0

echo "testuję..."

# tablica testów — każda linia to zestaw parametrów dla programu
EXPECTED_FILES=(
    invoke_forall_example.out
)

OUTPUT_FILES=(
	invoke_forall.out
)

for i in "${!OUTPUT_FILES[@]}"; do
    OUTPUT=(${OUTPUT_FILES[$i]})
    EXPECTED=(${EXPECTED_FILES[$i]})

    if ! cmp $OUTPUT $EXPECTED; then
        echo -e "${RED}test $OUTPUT ERROR${RESET}"
        EXIT_CODE=1
    else
        echo -e "${GREEN}test $OUTPUT SUKCES${RESET}"
        rm -f $OUTPUT
    fi
done

echo "koniec testu"
exit $EXIT_CODE
