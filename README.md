# Sudoku Lab — Solver & Puzzle Analyzer

A demonstration-ready DAA mini project with an HTML/CSS/JavaScript frontend and a C++ backend. The C++ backend uses recursive backtracking to count solutions (stopping after two) and solve the board.

## Features

- Interactive 9×9 Sudoku board with keyboard arrow navigation
- Four selectable sample puzzles (Easy, Medium and Hard labels), clear board, and editable non-given cells
- C++ validation for duplicate clues in rows, columns, and 3×3 boxes
- Backtracking solution search
- Unique, multiple (at least two), and unsolvable puzzle classification
- Statistics: recursive calls, attempted placements, backtracks, and elapsed time
- Responsive layout for laptop and mobile screens
- No third-party C++ libraries required

## Requirements

- Windows 10/11
- VS Code (recommended, but not required)
- GCC C++ compiler (MinGW-w64 / MSYS2)
- A modern browser

## Run it

1. Extract the entire ZIP folder. Do not move `frontend` or `backend` outside the project folder.
2. Install GCC if you do not already have it. In a new terminal, `g++ --version` should print a version.
3. Double-click `run.bat`, or open a terminal in the project folder and run `run.bat`.
4. Keep the terminal open. Open **http://127.0.0.1:8080** in your browser.
5. Choose a puzzle from the dropdown, click **Load example**, then click **Solve & analyze**. The dropdown also loads a selected puzzle immediately.
6. Press `Ctrl+C` in the server terminal when you are finished.

The app is hosted locally; it does not need an internet connection for its functionality. The Google Fonts import is decorative only and falls back to system fonts when offline.

## Project structure

```text
Sudoku-Project/
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── script.js
├── backend/
│   └── main.cpp
├── run.bat
└── README.md
```

## DAA explanation

- **Technique:** Backtracking (depth-first search over candidate values).
- **Simplified worst-case time bound:** `O(9^E)`, where `E` is the number of empty cells. This is a coarse upper-bound model; constraints prune many branches in practice.
- **Auxiliary space:** `O(E)` recursion depth, excluding the fixed-size 9×9 board.
- **Solution counting:** Stops once two solutions have been found, enough to distinguish a unique solution from a puzzle with multiple solutions.

## What the statistics mean

- **Recursive calls:** Number of calls made by the solution-counting search.
- **Attempted placements:** Number of candidate placements that passed the row/column/box check and were explored.
- **Backtracks:** Failed search paths recorded by the solver's counting routine. This is an implementation-specific counter, not a universal measure of puzzle difficulty.
- **Execution time:** Time used for solution counting and (when a solution exists) obtaining the displayed first solution, measured by C++ `high_resolution_clock`.

## Test cases for a demonstration

1. **Standard puzzle:** Click Load example, then Solve & analyze. Expect a completed board.
2. **Duplicate clue:** Enter the same number twice in a row/column/box where possible. The interface highlights conflicts and refuses to send the puzzle.
3. **Unsolvable but non-duplicate puzzle:** Start with a valid puzzle and add a clue that does not duplicate its row, column, or box but makes the puzzle impossible. The backend should report no solution.
4. **Multiple solutions:** Clear the board and solve it. It will report multiple solutions; the search stops after finding two.
5. **Empty board:** Demonstrates multiple solutions quickly because counting stops at two.

## Troubleshooting

- **`g++` not found:** Install MinGW-w64 or MSYS2 GCC, add its `bin` directory to PATH, then open a new terminal.
- **Port 8080 is in use:** Close the other program using port 8080, or change `htons(8080)` in `backend/main.cpp` and the URL printed in the console.
- **404 File not found:** Run `run.bat` from the extracted project folder and preserve the `frontend` directory.
- **Backend offline:** Open the URL printed by the server, not the HTML file directly from File Explorer.
- **Windows Firewall prompt:** This server binds only to `127.0.0.1` (your own computer); allow local/private access only if Windows asks.

## Viva questions to prepare

1. What is backtracking?
2. Why is Sudoku a constraint-satisfaction problem?
3. What does the `isSafe` function check?
4. What is the base case of the recursive solver?
5. Why does the algorithm reset a cell to zero?
6. What is the simplified worst-case time complexity?
7. Why does counting stop after two solutions?
8. What is the difference between invalid and unsolvable input?
9. Why might execution time differ between puzzles?
10. What is the role of the frontend and backend?

## Notes / limitations

- The backend uses a small, single-threaded Windows Winsock HTTP server for local demonstration; it is not intended for production or public-network hosting.
- The current implementation is Windows-specific because it uses Winsock.
- The solution-counting routine stops at two solutions by design.
