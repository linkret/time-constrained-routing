# time-constrained-routing
"Heuristic Optimization Methods" project at The Faculty of Electrical Engineering and Computing, University of Zagreb, 2023

Solving "The Capacitated Vehicle Routing Problem with Time Windows" (CVRPTW)

# Usage

Solver:
```
time-constrained-routing.exe ./instances/inst1.txt
```

It automatically saves the result in the `res` folder, but you can specify the output file as the second command line argument.

Validator:
```
python validator/validator.py -i instances/inst6.txt -o res/inst6.txt
```

Input visualizer:
```
python plot.py instances/inst1.txt
```
