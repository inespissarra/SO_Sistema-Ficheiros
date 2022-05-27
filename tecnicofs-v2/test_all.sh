make
cd tests/
echo "copy_to_external_errors:"
./copy_to_external_errors
echo "copy_to_external_simple:"
./copy_to_external_simple
echo "test1:"
./test1
echo "write_10_blocks_simple:"
./write_10_blocks_simple
echo "write_10_blocks_spill:"
./write_10_blocks_spill
echo "write_more_than_10_blocks_simple:"
./write_more_than_10_blocks_simple
echo "multi_thread_test1:"
./multi_thread_test1
echo "multi_thread_test2:"
./multi_thread_test2
echo "multi_thread_test3:"
./multi_thread_test3
