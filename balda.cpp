#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

struct dict_node
{
    bool exists = false;
    std::unordered_map<char, std::unique_ptr<dict_node>> children;
};

struct dict
{
    struct dict_node root;

    struct iterator
    {
        std::vector<dict_node*> path;
        std::string cur;

        std::vector<char> next_letters() const
        {
            std::vector<char> letters;
            for (const auto& x: path.back()->children) {
                letters.push_back(x.first);
            }
            return letters;
        }

        bool advance(char c)
        {
            auto it = path.back()->children.find(c);
            if (it == path.back()->children.end()) {
                return false;
            }

            cur.append(1, c);
            path.push_back(it->second.get());
            return true;
        }

        bool pop()
        {
            if (cur.empty()) {
                return false;
            }

            cur.pop_back();
            path.pop_back();
            return true;
        }

        const std::string* value() const
        {
            if (path.back()->exists) {
                return &cur;
            }

            return nullptr;
        }
    };

    iterator make_iterator()
    {
        iterator it;
        it.path.push_back(&root);
        return it;
    }

    void insert(const std::string& s, int pos, dict_node* node)
    {
        if (pos == s.size()) {
            node->exists = true;
            return;
        }

        auto& child = node->children[s[pos]];
        if (!child) {
            child = std::make_unique<dict_node>();
        }
        insert(s, pos + 1, child.get());
    }

    void insert(const std::string& s)
    {
        insert(s, 0, &root);
    }
};

using table = std::vector<std::vector<char>>;

void print_table(const table& t)
{
    for (int i = 0; i < t.size(); ++i) {
        for (int j = 0; j < t[i].size(); ++j) {
            if (j) {
                std::cout << " ";
            }
            std::cout << t[i][j];
        }

        std::cout << std::endl;
    }
}

using findings = std::vector<std::pair<std::string, table>>;

void find_words(
    dict::iterator& di,
    std::vector<std::pair<int, int>>& cpath,
    const table& t,
    int i,
    int j,
    bool added_letter,
    findings& f)
{
    if (i < 0 || i >= t.size() || j < 0 || j >= t[0].size()) {
        return;
    }

    for (const auto& x: cpath) {
        if (x == std::make_pair(i, j)) {
            return;
        }
    }

    std::vector<char> letters;
    if (t[i][j] == '_') {
        if (added_letter) {
            return;
        }

        added_letter = true;
        letters = di.next_letters();
    } else {
        letters.push_back(t[i][j]);
    }

    for (const auto letter: letters) {
        if (!di.advance(letter)) {
            continue;
        }

        cpath.push_back({i, j});

        if (di.value() && di.value()->size() > 1) {
            bool found = false;
            for (const auto& finding: f) {
                if (finding.first == *di.value()) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                table word;
                word.resize(t.size());
                for (auto& row: word) {
                    row.resize(t[0].size(), '_');
                }
                int pos = 0;
                for (const auto& x: cpath) {
                    word[x.first][x.second] = (*di.value())[pos];
                    ++pos;
                }
                f.emplace_back(*di.value(), std::move(word));
            }
        }

        const std::vector<std::pair<int, int>> moves = {
            {0, -1},
            {1, 0},
            {0, 1},
            {-1, 0},
        };

        for (const auto& m: moves) {
            find_words(
                di,
                cpath,
                t,
                i + m.first,
                j + m.second,
                added_letter,
                f);
        }

        di.pop();
        cpath.pop_back();
    }
}

int main(int argc, const char** argv)
{
    dict d;

    {
        std::ifstream f(argv[1]);
        std::string line;
        while (std::getline(f, line)) {
            d.insert(line);
        }
    }

    table t;

    {
        std::ifstream f(argv[2]);
        std::string line;
        while (std::getline(f, line)) {
            t.emplace_back();
            auto& row = t.back();
            for (int i = 0; i < line.size(); i += 2) {
                if (i + 1 < line.size() && line[i + 1] != ' ') {
                    return 1;
                }

                row.push_back(line[i]);
            }

            if (t.size() > 1 && t[0].size() != row.size()) {
                return 1;
            }
        }
    }

    print_table(t);

    findings f;

    for (int i = 0; i < t.size(); ++i) {
        for (int j = 0; j < t[i].size(); ++j) {
            auto di = d.make_iterator();
            std::vector<std::pair<int, int>> cpath;
            find_words(di, cpath, t, i, j, false, f);
        }
    }

    std::sort(f.begin(), f.end(), [] (const auto& l, const auto& r) {
        return l.first.size() > r.first.size();
    });

    for (const auto& x: f) {
        std::cout << "WORD: " << x.first << std::endl;
        print_table(x.second);
        std::cout << std::endl;
    }

    return 0;
}
