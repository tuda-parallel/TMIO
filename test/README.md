# Test framework

## Original test cases

For the MPI and zmq, the test cases isn't written by Catch2, but with raw C code. If you want to run the realted test:

+ MPI: Should go to the `build` directory and run `mpirun -np 4 ./test_mpi`, the soruce code for it is in `src/test.cxx`, and the explaination is in `test/MPI/README.md`.
+ ZMQ: Should go to the `build` directory and run `./test_zmq`, the soruce code for it is in `src/test_zmq.cxx`, and the explaination is in `test/ZMQ/README.md`.

## Catch2 test cases

For some of the MPI test (single process) and libcxx test, the test cases are written by Catch2. If you want to run the related test:

```bash
cd build
# Build the project and testing
make test
# Run the test
./run_test_mpi
./run_test_libc
```

### Add new test cases

If only need to add new test cases for the existing test folder, just add the new test file in the corresponding folder. Not need to modify the `Makefile` or other files.

But if need to add new folder for testing, remember to add:

```cpp
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
```

in the new test file. And modify the `Makefile` to add the new test file target.

