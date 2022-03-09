OUT=tests/output/`basename ${0%.sh}`.out
EOUT=tests/output/`basename ${0%.sh}`.eout
rm -f $OUT $EOUT

valgrind -h > /dev/null 2>&1
if [ $? != 0 ]; then
    echo "UNABLE TO RUN TEST $0"
    exit
fi

valgrind --leak-check=summary ./lobo_shell.x > $OUT 2>&1 << 'EOF'
whoami
EOF

BYTES=`cat $OUT | grep "definitely lost" | tr -s ' ' | cut -d ' ' -f4`
LEAKS=`cat $OUT | grep "no leaks are possible"`
if [ "$BYTES" == 0 ] || [ "x$LEAKS" != "x" ]; then
    echo "TEST PASSED $0"
else
    echo "TEST DETECTED MEMORY LEAKS $0"
fi


