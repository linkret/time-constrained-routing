# HMO Validator

* install and use Python >= 3.7

## INSTALL
```bash
pipenv install
source <virtualenv_activate_path> # (reported by virtualenv)
```
or
```bash
# create virtualenv manually
pip install
source <virtualenv_activate_path> # (created manually)
```
or
```bash
# globally
pip3 install click==7.0
pip3 install numpy==1.18.0 # most other versions should be fine
# and now run with global python3.7
```

## USAGE
```bash
python3.7 validator.py -i ../instances/i1.txt -o ../out/res-1m-i1.txt
```
or see `help`
```bash
python3.7 validator.py --help
```
