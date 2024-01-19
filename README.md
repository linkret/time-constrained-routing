# time-constrained-routing
"Heuristic Optimization Methods" project at The Faculty of Electrical Engineering and Computing, University of Zagreb, 2023

Solving "The Capacitated Vehicle Routing Problem with Time Windows" (CVRPTW)

Folder `sol-instances` contains Solomon's instances (http://web.cba.neu.edu/~msolomon/problems.htm), with sizes up to 100 vehicles. The solver was tried on some of them, an immediately constructed feasible solutions with minimal number of vehicles. The sum of travelled distances was within a few percentage points compared to the best known solutions cross-referenced online.

In the future, we will acquire bigger instances and run the solver on all of them - then precisely document our best achieved results.

# Usage

## Solver:
```
time-constrained-routing.exe ./instances/inst1.txt
```

It automatically saves the result in the `res` folder, but you can specify the output file as the second command line argument.

Three results are produced - one for the best found solution in 1 minute, one for 5 minutes, and one with no time limit.

## Validator:
```
python validator/validator.py -i instances/inst6.txt -o res/inst6.txt
```

## Visualizers:
```
python plot.py instances/inst1.txt

python plot_solutions.py instances/inst1.txt res/inst1.txt
```

