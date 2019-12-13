# QuadMat Tests

We have two types of tests: unit and medium.

Tests use the [Catch2](https://github.com/catchorg/Catch2) framework. This means the tests should integrate well with
IDEs such as CLion.

Tests bundle some external dependencies. See the [test_dependencies/](test_dependencies) directory for a full listing.

## Unit tests

Unit tests each test small parts of the code.

Unit tests run very quickly, on the order of milliseconds each. They are intended to be run often to quickly catch regressions.

Code coverage target is 100% of QuadMat code. Commits should have a 100% pass rate.

There are some tests that appear artificial because they trigger error conditions in order to reach that 100% code
coverage goal. That is ok. Such tests have caught problems.

Unit tests are organized identically to the QuadMat `include` directory. Each test file name follows the pattern `X_test.cpp`
where `X` is a QuadMat header.

## Medium tests

Medium tests run medium-scale problems to make sure the code works on non-trivial inputs. They can take non-trivial time to run.

Medium tests do not need 100% code coverage. Commits should have a 100% pass rate.

Medium tests are organized by functionality.

Inputs and expected outputs can be large. Medium test files are generated using a
Python script to avoid making the Git repo unwieldy. See the [gen](../gen) directory.