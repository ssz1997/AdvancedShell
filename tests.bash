echo "Pushd to student directory"
pushd $2 >/dev/null

compare_vs_sh() {

    testcase=$(($testcase + 1))
    echo "TESTCASE $testcase - $1"
    touch /tmp/fileout
    sh <<< $1 > "/tmp/sh_out.txt"
    mv /tmp/fileout /tmp/fileout_sh.txt
    touch /tmp/fileout
    ./dsh <<< $1 > "/tmp/dsh_out.txt"
    mv /tmp/fileout /tmp/fileout_dsh.txt

    error=$?
    if [ $error -ne 0 ]
    then
        echo FAIL - dsh returned nonzero
        return

    fi
      
    diff -B -w /tmp/sh_out.txt /tmp/dsh_out.txt
    error=$?
    if [ $error -eq 0 ]
    then
        diff -B -w /tmp/fileout_sh.txt /tmp/fileout_dsh.txt
        error=$?
        if [ $error -eq 0 ]
        then
            echo PASS
        else
            echo FAIL - fileout does not match solution
        fi
    else
        echo FAIL - output does not match solution
    fi
}


rm -f *.bin *.dat *~ core .test_* *.o dsh

gcc -I. -Wall -g3 -o dsh dsh.c parse.c helper.c

testcase=0
compare_vs_sh "ls"
compare_vs_sh "ls -l -h"
compare_vs_sh "echo Directory: ; ls"
compare_vs_sh "ls > /tmp/fileout"
compare_vs_sh "grep compare < tests.bash" 
compare_vs_sh "ls | wc -l"
compare_vs_sh "ls | grep .c | wc -l"
compare_vs_sh "cat < dsh.c | wc > /tmp/fileout" 
compare_vs_sh "ls | grep .c | cat | cat | cat | wc -l" 

popd >/dev/null
