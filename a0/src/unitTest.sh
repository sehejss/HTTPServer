#/bin/bash
curl https://ocw.mit.edu/ans7870/6/6.006/s08/lecturenotes/files/t8.shakespeare.txt --output shakespear.txt
make dog

echo "this is a test file\n" > test1.txt

cat test1.txt > testcat.txt
./dog test1.txt > testdog.txt

DIFF=`diff testcat.txt testdog.txt`
if [ "$DIFF" != "" ] 
then
    echo "test1: failed"
else
    echo "test1: passed"
fi 

rm test*

echo "shakespear test" > test1.txt

cat test1.txt shakespear.txt > testcat.txt
./dog test1.txt shakespear.txt > testdog.txt

DIFF=`diff testcat.txt testdog.txt`
if [ "$DIFF" != "" ] 
then
    echo "test2: failed"
else
    echo "test2: passed"
fi 

rm test*

./dog shakespear.txt > testdog.txt

DIFF=`diff shakespear.txt testdog.txt`
if [ "$DIFF" != "" ] 
then
    echo "test3: failed"
else
    echo "test3: passed"
fi 

echo "hello world" | cat - > testcat.txt

echo "hello world" | ./dog - > testdog.txt

DIFF=`diff testcat.txt testdog.txt`
if [ "$DIFF" != "" ] 
then
    echo "test4: failed"
else
    echo "test4: passed"
fi 

rm test*
rm shakespear.txt
make spotless