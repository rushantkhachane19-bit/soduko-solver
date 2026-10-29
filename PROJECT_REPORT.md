# Sudoku Solver and Puzzle Analyzer Using Backtracking

## Abstract
This project implements a Sudoku puzzle solver and analyzer. The user interacts with a browser-based 9×9 grid, while a C++ backend validates clues, searches for solutions using recursive backtracking, and reports performance statistics. It demonstrates a practical application of recursion, constraint checking, search-space pruning, and algorithm complexity analysis.

## Problem Statement
Solving Sudoku manually can require repeated trial and error. The goal is to develop a program that can solve a valid Sudoku puzzle and explain basic characteristics of the search performed by the algorithm.

## Objectives
1. Implement Sudoku solving using recursive backtracking in C++.
2. Validate that initial clues do not conflict in rows, columns, or 3×3 boxes.
3. Identify whether a puzzle is unsolvable, has a unique solution, or has multiple solutions.
4. collect search statistics for algorithm analysis.
5. Provide a simple and accessible user interface.

## Technologies
- C++17: solver, validation, HTTP server, timing
- HTML: page structure
- CSS: responsive styling
- JavaScript: grid interaction and frontend/backend communication
- Windows Winsock: local HTTP communication

## Methodology
The solver finds an empty cell, tries values 1 through 9, and checks each value against Sudoku constraints. A valid candidate is placed and the algorithm recurses. If the recursive search cannot complete the board, the candidate is removed and the next value is tried. The solution counter stops at two because two solutions are sufficient to establish that the puzzle is not unique.

## Algorithm (pseudocode)
```text
COUNT_SOLUTIONS(board, limit = 2):
    if solution_count >= limit:
        return
    recursive_calls += 1
    find an empty cell
    if no empty cell:
        solution_count += 1
        return
    for value from 1 to 9:
        if value is safe in the cell:
            attempted_placements += 1
            place value
            COUNT_SOLUTIONS(board, limit)
            remove value
            if solution_count >= limit:
                return
```

## Complexity Analysis
Let E be the number of initially empty cells. A simplified worst-case time bound is O(9^E), since up to nine candidates may be explored per empty cell. Constraint checking and pruning reduce practical search. The recursion depth is at most E, so auxiliary recursion space is O(E), excluding the fixed-size board.

## Results
The interface displays a solution when one exists and reports whether one or at least two solutions were found. It also displays recursive calls, candidate placements, backtracks, and elapsed time. Measurements depend on the selected puzzle and machine.

## Conclusion
The project demonstrates how backtracking solves a constraint-satisfaction problem by exploring candidate assignments and undoing choices when a branch fails. Performance counters make the search process more observable and connect the implementation to DAA concepts.

## Future Enhancements
- Compare standard backtracking with a minimum-remaining-values (MRV) heuristic.
- Add animated step-by-step solving.
- Add puzzle import/export.
- Improve statistics and create charts across a benchmark set.
- Port the local server to cross-platform networking.
