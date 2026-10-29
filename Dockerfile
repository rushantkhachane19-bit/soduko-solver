FROM gcc:14
WORKDIR /app
COPY main.cpp .
RUN g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o sudoku-server
ENV PORT=10000
EXPOSE 10000
CMD ["sh", "-c", "./sudoku-server"]
