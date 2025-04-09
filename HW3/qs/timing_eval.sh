make clean
make
echo "Running timing evaluation..."
echo "### Normal Timing" > timing_eval.txt
echo "time ./runme_large.sh" >> timing_eval.txt
{ time ./runme_large.sh ; } 2>> timing_eval.txt
make clean
echo "" >> timing_eval.txt

echo "### Optimize/Debug Timing" >> timing_eval.txt
cp Makefile_debug Makefile
make
echo "time ./runme_large.sh" >> timing_eval.txt
{ time ./runme_large.sh ; } 2>> timing_eval.txt
make clean
echo "" >> timing_eval.txt

# echo "### Gprof Timing" >> timing_eval.txt
# cp Makefile_gprof Makefile
# make
# echo "time ./runme_large.sh" >> timing_eval.txt
# { time ./runme_large.sh ; } 2>> timing_eval.txt
# echo "" >> timing_eval.txt
# echo "gprof output:" >> timing_eval.txt
# gprof ./qsort_large gmon.out >> timing_eval.txt
# echo "" >> timing_eval.txt
# make clean

echo "### Optimized Code Timing" >> timing_eval.txt
cp Makefile_code_optimal Makefile
make
echo "time ./runme_large.sh" >> timing_eval.txt
{ time ./runme_large.sh ; } 2>> timing_eval.txt
make clean
echo "" >> timing_eval.txt

echo "### Fully Optimized Timing" >> timing_eval.txt
cp Makefile_full_optimal Makefile
make
echo "time ./runme_large.sh" >> timing_eval.txt
{ time ./runme_large.sh ; } 2>> timing_eval.txt
make clean
echo "" >> timing_eval.txt

cp Makefile_classic Makefile
