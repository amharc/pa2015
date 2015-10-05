#!/bin/bash

PROG=$1
shift

OK=0
NOK=0

function run() {
    name=${1%.in}
    ${PROG} < ${name}.in > ${name}.myout
    diff -bBwq ${name}.myout ${name}.out
    if [ $? -ne 0 ]
    then
        NOK=$(($NOK+1))
        echo -e "Test ${name} \e[31mNOT OK\e[0m"
    else
        OK=$(($OK+1))
        echo -e "Test ${name} \e[32mOK\e[0m"
    fi
}

for i in $@
do
    if [ -f $i ];
    then
        run $i
    elif [ -d $i ];
    then
        for file in $i/*.in
        do
            run $file
        done
    fi
done

if [ ${OK} -gt 0 ]
then
    echo -e "${OK} tests \e[32mPASSED\e[0m"
fi

if [ ${NOK} -gt 0 ]
then
    echo -e "${NOK} tests \e[31mFAILED\e[0m"
fi
