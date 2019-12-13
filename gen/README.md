Scripts to generate test cases.

### Setup

Standard Python3 scripts that use SciPy to generate matrices. Your system interpreter should work, but if not:

```bash
pip3 install -r requirements.txt
```

### Usage

```bash
python3 generate_test_problems.py
```

Unit test output of `generate_test_problems.py` is checked in.

Only medium tests require running the test generator. This is due to the large size of medium test matrices.