#include<iostream>
#include<fstream>
#include<sstream>
#include<unordered_map>
#include<unordered_set>
#include<string>
#include<vector>
#include<algorithm>
#include<queue>
#include<map>
#include<chrono>
#include<iomanip>
using namespace std;


/// Global Variables
unordered_map<string,string> titleToID;
unordered_map<string,string> idToTitle;
unordered_map<string,vector<string>> actorToMovies;
unordered_map<string,string> actorIDToName;
unordered_map<string, vector<pair<string,double>>> movieGraph;

struct Path {
  vector<string> movies;
  vector<vector<string>> actors;
};

/// User Input
void getInput(string& from, string& to) {
    cout << "Enter start movie (or 'done'): ";
    getline(cin, from);
    if (from == "done") return;
    cout << "Enter end movie (or 'done'): ";
    getline(cin, to);
}

/// Title Matching
string findMatch(const string& input) {
    if (titleToID.count(input)) return input;
    string inputLower = input;
    transform(inputLower.begin(), inputLower.end(), inputLower.begin(), ::tolower);
    vector<string> matches;
    for (const auto& [title, _] : titleToID) {
        string lower = title;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find(inputLower) != string::npos)
            matches.push_back(title);
    }

    if (matches.empty()) return "";

    if (matches.size() == 1) return matches[0];

    sort(matches.begin(), matches.end());
    cout << "Found multiple matches for \"" << input << "\":\n";
    for (size_t i=0; i < matches.size(); i++) cout << "  " << (i+1) << ": " << matches[i] << "\n";

    int choice = 0;
    while (choice < 1 || choice > (int)matches.size()) {
        cout << "Choose a number (1-" << matches.size() << "): ";
        cin >> choice;
        cin.ignore();
    }
    return matches[choice-1];
}

/// Loaders
void loadMovieMap(const string& file) {
    ifstream in(file); string line;
    while (getline(in, line)) {
        size_t tab = line.find('\t');
        if (tab == string::npos) continue;
        string title = line.substr(0, tab), id = line.substr(tab + 1);
        titleToID[title] = id;
        idToTitle[id] = title;
    }
}

void loadActorMovies(const string& filename) {
    ifstream in(filename); string line;
    while (getline(in, line)) {
        stringstream ss(line);
        string id, movie;
        ss >> id;
        while (ss >> movie)
            actorToMovies[id].push_back(movie);
    }
}

void loadActorNames(const string& filename){
    ifstream file(filename);
    string line;
    while(getline(file, line)){
        size_t tab = line.find('\t');
        if(tab == string::npos) continue;
        string id = line.substr(0, tab);
        string name = line.substr(tab + 1);
        actorIDToName[id] = name;
    }
}

/// Build Graph
void buildGraph() {
    unordered_map<string, unordered_map<string, int>> edges;
    for (const auto& [actor, movies] : actorToMovies) {
        unordered_set<string> unique(movies.begin(), movies.end());
        if (unique.size() < 2) continue;
        vector<string> list(unique.begin(), unique.end());
        for (size_t i = 0; i < list.size(); ++i)
            for (size_t j = i + 1; j < list.size(); ++j) {
                edges[list[i]][list[j]]++;
                edges[list[j]][list[i]]++;
            }
    }

    for (auto& [a, adj] : edges)
        for (auto& [b, count] : adj)
            movieGraph[a].emplace_back(b, 1.0 / count);
}

/// BFS
Path runBFS(const string& from, const string& to) {
    unordered_map<string, string> parent;
    unordered_set<string> visited;
    queue<string> q; q.push(from); visited.insert(from);
    parent[from] = "";

    while (!q.empty()) {
        string cur = q.front(); q.pop();
        if (cur == to) break;
        for (const auto& [next, _] : movieGraph[cur]) {
            if (!visited.count(next)) {
                visited.insert(next);
                parent[next] = cur;
                q.push(next);
            }
        }
    }

    Path path;
    if (!parent.count(to)) return path;

    for (string at = to; !at.empty(); at = parent[at])
        path.movies.push_back(at);
    reverse(path.movies.begin(), path.movies.end());

    for (size_t i=0; i+1 < path.movies.size(); i++) {
        string a = path.movies[i], b = path.movies[i+1];
        vector<string> actors;
        for (const auto& [actor, list] : actorToMovies) {
            unordered_set<string> s(list.begin(), list.end());
            if (s.count(a) && s.count(b)) {
                actors.push_back(actorIDToName[actor]);
                if (actors.size() >= 3) break;
            }
        }
        path.actors.push_back(actors);
    }

    return path;
}

/// Dijkstra
Path runDijkstra(const string& from, const string& to){
    unordered_map<string, double> dist;
    unordered_map<string, string> parent;
    unordered_set<string> visited;
    priority_queue<pair<double,string>, vector<pair<double,string>>, greater<>> pq;

    for (const auto& [movie, _] : movieGraph) dist[movie] = 1e9;
    dist[from] = 0.0; pq.emplace(0.0, from);

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (visited.count(u)) continue;
        visited.insert(u);
        if (u == to) break;

        for (const auto& [v, w] : movieGraph[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.emplace(dist[v], v);
            }
        }
    }

    Path path;
    if (!parent.count(to)) return path;

    for (string at = to; !at.empty(); at = parent[at])
        path.movies.push_back(at);
    reverse(path.movies.begin(), path.movies.end());

    for (size_t i=0; i+1 < path.movies.size(); i++) {
        string a = path.movies[i], b = path.movies[i+1];
        vector<string> actors;
        for (const auto& [actor, list] : actorToMovies) {
            unordered_set<string> s(list.begin(), list.end());
            if (s.count(a) && s.count(b)) {
                actors.push_back(actorIDToName[actor]);
                if (actors.size() >= 3) break;
            }
        }
        path.actors.push_back(actors);
    }

    return path;
}

/// Main Program Loop
int main() {
    loadMovieMap("movie_map.txt");
    loadActorMovies("actor_movies.txt");
    loadActorNames("actor_names.txt");
    buildGraph();

    while (true) {
        string from, to;
        getInput(from, to);
        if (from == "done" || to == "done") break;

        from = findMatch(from);
        to = findMatch(to);

        if (!titleToID.count(from) || !titleToID.count(to)) {
            cout << "Movie not found.\n\n"; continue;
        }

        string idFrom = titleToID[from], idTo = titleToID[to];

        auto t0 = chrono::high_resolution_clock::now();
        Path bfs = runBFS(idFrom, idTo);
        auto t1 = chrono::high_resolution_clock::now();
        Path djk = runDijkstra(idFrom, idTo);
        auto t2 = chrono::high_resolution_clock::now();

        double bfsTime = chrono::duration<double, milli>(t1 - t0).count();
        double djkTime = chrono::duration<double, milli>(t2 - t1).count();

        cout << "\nBFS Path (" << bfs.movies.size() << " steps, " << fixed << setprecision(2) << bfsTime << " ms):\n";
        for (size_t i=0; i<bfs.movies.size(); i++) {
            cout << " - " << idToTitle[bfs.movies[i]] << "\n";
            if (i + 1 < bfs.movies.size()) {
                cout << "    Shared actors: ";
                if (i < bfs.actors.size() && !bfs.actors[i].empty())
                    for (size_t j = 0; j < bfs.actors[i].size(); ++j) {
                        cout << bfs.actors[i][j];
                        if (j + 1 < bfs.actors[i].size()) cout << ", ";
                    }
                else cout << "(none)";
                cout << "\n";
            }
        }

        cout << "\nDijkstra Path (" << djk.movies.size() << " steps, " << fixed << setprecision(2) << djkTime << " ms):\n";
        for (size_t i=0; i<djk.movies.size(); i++) {
            cout << " - " << idToTitle[djk.movies[i]] << "\n";
            if (i + 1 < djk.movies.size()) {
                cout << "    Shared actors: ";
                if (i < djk.actors.size() && !djk.actors[i].empty()) {
                    for (size_t j = 0; j < djk.actors[i].size(); ++j) {
                        cout << djk.actors[i][j];
                        if (j + 1 < djk.actors[i].size()) cout << ", ";
                    }
                } else cout << "(none)";
                cout << "\n";
            }
        }

        cout << "\n--- Type 'done' at any time to exit ---\n\n";
    }

    cout << "Goodbye!\n";
    return 0;
}
