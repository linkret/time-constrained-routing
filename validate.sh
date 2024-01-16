
for f in $(ls res | grep -v _itercnt); do
    if [[ $f == *"i1"* ]]; then
        inst="inst1.TXT"
    elif [[ $f == *"i2"* ]]; then
        inst="inst2.TXT"
    elif [[ $f == *"i3"* ]]; then
        inst="inst3.TXT"
    elif [[ $f == *"i4"* ]]; then
        inst="inst4.TXT"
    elif [[ $f == *"i5"* ]]; then
        inst="inst5.TXT"
    elif [[ $f == *"i6"* ]]; then
        inst="inst6.TXT"
    fi

    python3 validator/validator.py -i instances/$inst -o res/$f 2>&1 | grep SUCCESSFUL > /dev/null

    if [ $? -ne 0 ]; then
        echo "ERROR: $f failed validation"
    else
        echo "SUCCESS: $f passed validation"
    fi

done
