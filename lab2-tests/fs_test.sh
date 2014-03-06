NUMTESTS=7
CORRECT=0

echo "Mounting ramfs..."
mount -t ramfs ramfs /mnt

echo "Running fs test 0: File reads"

if [ -f /mnt/fs_test0.txt ]; then
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 0.1: Existing file is locatable"
else
    echo "Failed fs test 0.1: Existing file could not be located"
    exit $(( NUMTESTS - CORRECT ))
fi

if [ -d /mnt/fs_testDir0 ]; then
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 0.2: Existing directory is detectable as a directory"
else
    echo "Failed fs test 0.2: Existing directory could not be detected or was not considered a directory"
    exit $(( NUMTESTS - CORRECT ))
fi

if [ $(cat /mnt/fs_test0.txt | diff -q - /test/fs_test0.std | wc -l) = "0" ] ; then
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 0.3: Existing file was read correctly"
else
    echo "Failed fs test 0.3: Existing file could not be read correctly"
    exit $(( NUMTESTS - CORRECT ))
fi

echo "Running fs test 1: New file creation"

touch /mnt/fs_test1.txt

if [ -f /mnt/fs_test1.txt ]; then
    CORRECT=$(( CORRECT + 1))
    echo "Passed fs test 1.1: New file created successfully"
else
    echo "Failed fs test 1.1: New file not created"
    exit $(( NUMTESTS - CORRECT ))
fi

cat /mnt/fs_test1.txt > /test/fs_test1.out
if [ $(cat /mnt/fs_test1.txt | diff -q - /test/fs_test1.std | wc -l) = "0" ] ; then
    CORRECT=$(( CORRECT + 1))
    echo "Passed fs test 1.2: New file is empty"
else
    echo "Failed fs test 1.2: New file was nonempty"
    exit $(( NUMTESTS - CORRECT ))
fi

echo "This is a file" > /mnt/fs_test2.txt
if [ -f /mnt/fs_test2.txt ] ; then
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 1.3: Created file by redirection"
else
    echo "Failed fs test 1.3: Could not create file by redirection"
    exit $(( NUMTESTS - CORRECT ))
fi

cat /mnt/fs_test2.txt > /test/fs_test2.out
if [ $(diff -q /test/fs_test2.out /test/fs_test2.std | wc -l) = "0" ] ; then
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 1.4: File created by redirection has correct contents"
else
    echo "Failed fs test 1.4: File created by redirection has incorrect contents"
    exit $(( NUMTESTS - CORRECT ))
fi

echo "Running fs test 2: File deletion/unlink"

if [ -f /mnt/fs_test1.txt ] ; then
    rm /mnt/fs_test1.txt
else
    echo "Cannot run test; deletion target does not exist. Pass other tests first"
    exit $(( NUMTESTS - CORRECT ))
fi

if [ -e /mnt/fs_test1.txt ] ; then
    echo "Failed fs test 2.1: File was not deleted successfully"
    exit $(( NUMTESTS - CORRECT ))
else
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 2.1: File was deleted successfully"
fi

echo "Running fs test 3: Write to existing file"

if [ -f /mnt/fs_test3.txt ] ; then
    echo "This was added to a file" >> /mnt/fs_test3.txt
else
    echo "Cannot run test; base file did not exist or could not be found."
fi

if [ $(cat /mnt/fs_test3.txt | diff -q - /test/fs_test3.std | wc -l) = "0" ] ; then
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 3.1: File was appended to successfully"
else
    echo "Failed fs test 3.1: File was not appended to correctly"
    exit $(( NUMTESTS - CORRECT ))
fi

if [ -f /mnt/fs_test4.txt ] ; then
    echo "This overwrote a file" > /mnt/fs_test4.txt
else
    echo "Cannot run test; base file did not exist or could not be found."
    exit $(( NUMTESTS - CORRECT ))
fi

if [ $(cat /mnt/fs_test4.txt | diff -q - /test/fs_test4.std | wc -l) = "0" ] ; then
    CORRECT=$(( CORRECT + 1 ))
    echo "Passed fs test 3.2: File was overwritten successfully"
else
    echo "Failed fs test 3.2: File was not overwritten successfully"
    exit $(( NUMTESTS - CORRECT ))
fi
