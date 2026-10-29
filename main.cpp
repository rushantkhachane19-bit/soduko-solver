#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdlib>

using namespace std;

struct Puzzle {
    int n = 9, boxRows = 3, boxCols = 3;
    vector<vector<int>> cells;
};
struct Stats {
    long long recursiveCalls = 0;
    long long attemptedPlacements = 0;
    long long backtracks = 0;
    int solutions = 0;
    vector<vector<int>> firstBoard;
    double elapsedMs = 0;
};

string readFile(const string& path) {
    ifstream f(path, ios::binary);
    if (!f) return "";
    ostringstream ss; ss << f.rdbuf(); return ss.str();
}
string jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}
string mimeType(const string& p) {
    if (p.size() >= 5 && p.substr(p.size()-5) == ".html") return "text/html; charset=utf-8";
    if (p.size() >= 4 && p.substr(p.size()-4) == ".css") return "text/css; charset=utf-8";
    if (p.size() >= 3 && p.substr(p.size()-3) == ".js") return "application/javascript; charset=utf-8";
    return "text/plain; charset=utf-8";
}
int symbolValue(char c) {
    if (c >= '1' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'G') return 10 + c - 'A';
    if (c >= 'a' && c <= 'g') return 10 + c - 'a';
    return -1;
}
char valueSymbol(int v) {
    if (v >= 1 && v <= 9) return char('0' + v);
    if (v >= 10 && v <= 16) return char('A' + v - 10);
    return '0';
}
string getStringField(const string& body, const string& key, const string& fallback) {
    string needle = "\"" + key + "\"";
    size_t p = body.find(needle);
    if (p == string::npos) return fallback;
    p = body.find(':', p + needle.size());
    if (p == string::npos) return fallback;
    p = body.find('"', p + 1);
    if (p == string::npos) return fallback;
    size_t e = body.find('"', p + 1);
    if (e == string::npos) return fallback;
    return body.substr(p + 1, e - p - 1);
}
int getIntField(const string& body, const string& key, int fallback) {
    string needle = "\"" + key + "\"";
    size_t p = body.find(needle);
    if (p == string::npos) return fallback;
    p = body.find(':', p + needle.size());
    if (p == string::npos) return fallback;
    ++p; while (p < body.size() && isspace((unsigned char)body[p])) ++p;
    try { return stoi(body.substr(p)); } catch (...) { return fallback; }
}
bool parseRequest(const string& body, Puzzle& p, string& error) {
    p.n = getIntField(body, "size", 9);
    if (p.n != 4 && p.n != 6 && p.n != 9 && p.n != 16) {
        error = "Supported grid sizes are 4, 6, 9 and 16."; return false;
    }
    p.boxRows = getIntField(body, "boxRows", p.n == 4 ? 2 : p.n == 6 ? 2 : p.n == 9 ? 3 : 4);
    p.boxCols = getIntField(body, "boxCols", p.n == 4 ? 2 : p.n == 6 ? 3 : p.n == 9 ? 3 : 4);
    if (p.boxRows * p.boxCols != p.n || p.n % p.boxRows || p.n % p.boxCols) {
        error = "Invalid box layout for the selected grid size."; return false;
    }
    string needle = "\"board\"";
    size_t k = body.find(needle);
    size_t start = k == string::npos ? string::npos : body.find('[', k);
    if (start == string::npos) { error = "Request must contain a board array."; return false; }
    size_t end = string::npos; int depth = 0;
    for (size_t i = start; i < body.size(); ++i) {
        if (body[i] == '[') ++depth;
        else if (body[i] == ']') { --depth; if (depth == 0) { end = i; break; } }
    }
    if (end == string::npos) { error = "Board array is malformed."; return false; }
    vector<vector<string>> rows;
    vector<string> row;
    string token;
    depth = 0;
    for (size_t i = start; i <= end; ++i) {
        char c = i == end ? ']' : body[i];
        if (c == '[') { ++depth; if (depth == 2) row.clear(); }
        else if (c == ']') {
            if (depth == 2) rows.push_back(row);
            --depth;
        } else if (depth == 2) {
            if (c == '"') {
                size_t q = body.find('"', i + 1);
                if (q == string::npos || q > end) { error = "Malformed board symbol."; return false; }
                row.push_back(body.substr(i + 1, q - i - 1));
                i = q;
            } else if (isdigit((unsigned char)c)) {
                size_t j = i;
                while (j <= end && isdigit((unsigned char)body[j])) ++j;
                row.push_back(body.substr(i, j - i)); i = j - 1;
            }
        }
    }
    if ((int)rows.size() != p.n) { error = "The board must have " + to_string(p.n) + " rows."; return false; }
    p.cells.assign(p.n, vector<int>(p.n, 0));
    for (int r = 0; r < p.n; ++r) {
        if ((int)rows[r].size() != p.n) { error = "Each row must contain " + to_string(p.n) + " cells."; return false; }
        for (int c = 0; c < p.n; ++c) {
            string s = rows[r][c];
            int v = 0;
            if (s == "0" || s == "." || s.empty()) v = 0;
            else if (s.size() == 1) v = symbolValue(s[0]);
            else {
                try { v = stoi(s); } catch (...) { v = -1; }
            }
            if (v < 0 || v > p.n) {
                error = "Invalid symbol in puzzle. Use 1-9, and A-G for values 10-16."; return false;
            }
            p.cells[r][c] = v;
        }
    }
    return true;
}
bool safe(const Puzzle& p, int r, int c, int v) {
    for (int i = 0; i < p.n; ++i)
        if (p.cells[r][i] == v || p.cells[i][c] == v) return false;
    int sr = (r / p.boxRows) * p.boxRows, sc = (c / p.boxCols) * p.boxCols;
    for (int rr = sr; rr < sr + p.boxRows; ++rr)
        for (int cc = sc; cc < sc + p.boxCols; ++cc)
            if (p.cells[rr][cc] == v) return false;
    return true;
}
bool validate(Puzzle& p, string& error) {
    for (int r=0;r<p.n;r++) for(int c=0;c<p.n;c++) {
        int v=p.cells[r][c];
        if (!v) continue;
        p.cells[r][c] = 0;
        bool ok = safe(p,r,c,v);
        p.cells[r][c] = v;
        if (!ok) { error = "Conflicting clues: a number repeats in a row, column, or box."; return false; }
    }
    return true;
}
bool findEmpty(Puzzle& p, int& br, int& bc) {
    // Minimum-remaining-values heuristic helps the larger grids finish more reliably.
    int best = p.n + 1; br = -1; bc = -1;
    for (int r=0;r<p.n;r++) for(int c=0;c<p.n;c++) if(!p.cells[r][c]) {
        int count=0;
        for(int v=1;v<=p.n;v++) if(safe(p,r,c,v)) ++count;
        if(count < best) { best=count; br=r; bc=c; if(best<=1) return true; }
        if(count==0) return true;
    }
    return br != -1;
}
void countSolutions(Puzzle& p, Stats& st, int limit=2) {
    if (st.solutions >= limit) return;
    st.recursiveCalls++;
    int r,c;
    if (!findEmpty(p,r,c)) {
        if (st.solutions == 0) st.firstBoard = p.cells;
        st.solutions++; return;
    }
    if (r < 0 || c < 0) {
        if (st.solutions == 0) st.firstBoard = p.cells;
        st.solutions++; return;
    }
    vector<int> candidates;
    for(int v=1;v<=p.n;v++) if(safe(p,r,c,v)) candidates.push_back(v);
    if(candidates.empty()) { st.backtracks++; return; }
    for(int v:candidates) {
        if(st.solutions>=limit) return;
        st.attemptedPlacements++;
        p.cells[r][c]=v;
        int before=st.solutions;
        countSolutions(p,st,limit);
        p.cells[r][c]=0;
        if(st.solutions==before) st.backtracks++;
    }
}
string boardJson(const vector<vector<int>>& cells) {
    int n = (int)cells.size();
    ostringstream o; o << "[";
    for(int r=0;r<n;r++) { if(r)o<<","; o<<"[";
        for(int c=0;c<n;c++) { if(c)o<<","; o << "\"" << (cells[r][c] ? valueSymbol(cells[r][c]) : '0') << "\""; }
        o<<"]";
    } o<<"]"; return o.str();
}
string analyze(const string& body) {
    Puzzle p; string error;
    if(!parseRequest(body,p,error)) return "{\"ok\":false,\"message\":\""+jsonEscape(error)+"\"}";
    if(!validate(p,error)) return "{\"ok\":false,\"message\":\""+jsonEscape(error)+"\"}";
    Stats st;
    auto begin=chrono::high_resolution_clock::now();
    countSolutions(p,st,2);
    auto finish=chrono::high_resolution_clock::now();
    st.elapsedMs=chrono::duration<double,milli>(finish-begin).count();
    string status=st.solutions==0?"unsolvable":st.solutions==1?"unique":"multiple";
    ostringstream out;
    out<<"{\"ok\":true,\"status\":\""<<status<<"\",\"message\":\"";
    if(st.solutions==0) out<<"No solution exists for this puzzle.";
    else if(st.solutions==1) out<<"Solved! This puzzle has a unique solution.";
    else out<<"Solved! This puzzle has at least two solutions.";
    out<<"\",\"solutionsFound\":"<<st.solutions<<",\"board\":"<<(st.solutions?boardJson(st.firstBoard):"null")
       <<",\"stats\":{\"recursiveCalls\":"<<st.recursiveCalls<<",\"attemptedPlacements\":"<<st.attemptedPlacements
       <<",\"backtracks\":"<<st.backtracks<<",\"elapsedMs\":"<<st.elapsedMs<<"}}";
    return out.str();
}

void sendResponse(int client, int code, const string& status, const string& type, const string& body) {
    ostringstream h;
    h << "HTTP/1.1 " << code << " " << status
      << "\r\nContent-Type: " << type
      << "\r\nContent-Length: " << body.size()
      << "\r\nAccess-Control-Allow-Origin: *"
      << "\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS"
      << "\r\nAccess-Control-Allow-Headers: Content-Type"
      << "\r\nConnection: close\r\n\r\n" << body;
    string data = h.str();
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(client, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
    }
}

void handleClient(int client) {
    string req;
    char buf[8192];
    size_t headerEnd = string::npos;
    size_t contentLength = 0;

    while (req.size() <= 3 * 1024 * 1024) {
        ssize_t n = recv(client, buf, sizeof(buf), 0);
        if (n <= 0) break;
        req.append(buf, static_cast<size_t>(n));

        headerEnd = req.find("\r\n\r\n");
        if (headerEnd != string::npos) {
            size_t cl = req.find("Content-Length:");
            if (cl != string::npos && cl < headerEnd) {
                cl += 15;
                while (cl < headerEnd && req[cl] == ' ') ++cl;
                size_t lineEnd = req.find("\r\n", cl);
                try {
                    contentLength = stoul(req.substr(cl, lineEnd - cl));
                } catch (...) {
                    contentLength = 0;
                }
            }
            if (req.size() >= headerEnd + 4 + contentLength) break;
        }
    }

    size_t lineEnd = req.find("\r\n");
    if (lineEnd == string::npos) {
        sendResponse(client, 400, "Bad Request", "text/plain; charset=utf-8", "Bad request");
        return;
    }

    istringstream line(req.substr(0, lineEnd));
    string method, path, version;
    line >> method >> path >> version;

    if (method == "OPTIONS") {
        sendResponse(client, 204, "No Content", "text/plain; charset=utf-8", "");
        return;
    }

    if (method == "GET" && (path == "/" || path == "/health")) {
        sendResponse(client, 200, "OK", "application/json; charset=utf-8",
                     "{\"ok\":true,\"message\":\"Sudoku Solver API is running.\"}");
        return;
    }

    if (method == "POST" && path == "/api/solve") {
        size_t split = req.find("\r\n\r\n");
        string body = split == string::npos ? "" : req.substr(split + 4);
        sendResponse(client, 200, "OK", "application/json; charset=utf-8", analyze(body));
        return;
    }

    sendResponse(client, 404, "Not Found", "application/json; charset=utf-8",
                 "{\"ok\":false,\"message\":\"Route not found.\"}");
}

int main() {
    const char* portEnv = getenv("PORT");
    int port = 10000;
    if (portEnv && *portEnv) {
        try { port = stoi(portEnv); } catch (...) {}
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        cerr << "Socket creation failed: " << strerror(errno) << "\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        cerr << "Bind failed on port " << port << ": " << strerror(errno) << "\n";
        close(server);
        return 1;
    }

    if (listen(server, SOMAXCONN) < 0) {
        cerr << "Listen failed: " << strerror(errno) << "\n";
        close(server);
        return 1;
    }

    cout << "Sudoku Solver API listening on port " << port << "\n";
    while (true) {
        int client = accept(server, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) continue;
            cerr << "Accept failed: " << strerror(errno) << "\n";
            continue;
        }
        handleClient(client);
        shutdown(client, SHUT_WR);
        close(client);
    }

    close(server);
    return 0;
}

