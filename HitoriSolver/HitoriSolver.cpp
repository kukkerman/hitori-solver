#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

class DisjointSets {
private:
    std::vector<int> parents;

public:
    DisjointSets(unsigned size):
        parents(size, -1) { }

    int findSet(int e) {
        assertElementRange(e);

        auto root = e;
        while (parents[root] >= 0) {
            root = parents[root];
        }

        while (parents[e] >= 0) {
            const auto parent = parents[e];
            parents[e] = root;
            e = parent;
        }

        return root;
    }

    void unionSets(int e1, int e2) {
        const auto root1 = findSet(e1);
        const auto root2 = findSet(e2);
        linkSets(root1, root2);
    }

    void linkSets(int set1, int set2) {
        assertElementRange(set1);
        assertElementRange(set2);

        if (parents[set1] >= 0 || parents[set2] >= 0) {
            throw std::logic_error("at least one element is not root");
        }

        if (set1 != set2) {
            const auto rank1 = -parents[set1] - 1;
            const auto rank2 = -parents[set2] - 1;
            if (rank1 < rank2) {
                parents[set1] = set2;

            } else if (rank2 < rank1) {
                parents[set2] = set1;

            } else {
                parents[set1] = set2;
                parents[set2]--;
            }
        }
    }

private:
    void assertElementRange(int e) {
        if (e < 0 || static_cast<size_t>(e) >= parents.size()) {
            throw std::range_error("element index is out of range");
        }
    }
};

class Table {
public:
    class Cell {
    public:
        enum class State {
            Unknown,
            Final,
            Deleted
        };

    private:
        const unsigned value;
        State state;

    public:
        Cell(unsigned value):
            value(value),
            state(State::Unknown) { }

        unsigned getValue() const {
            return value;
        }

        State getState() const {
            return state;
        }

        void setState(State newState) {
            state = newState;
        }
    };

    class InvalidShapeException: public std::length_error {
    public:
        InvalidShapeException():
            std::length_error("table shape is invalid") { }
    };

    class InvalidValueException : public std::range_error {
    public:
        InvalidValueException():
            std::range_error("invalid value") { }
    };

    class MultipleStateChangeException : public std::logic_error {
    public:
        MultipleStateChangeException():
            std::logic_error("tried to set an entry's state multiple times") { }
    };

    class NoSolutionExistsException : public std::logic_error {
    public:
        NoSolutionExistsException(const std::string &reason) :
            std::logic_error(reason) { }
    };

private:
    const unsigned size;
    std::vector<std::vector<Cell>> cells;
    std::vector<std::vector<unsigned>> rowCounts, columnCounts;
    unsigned unknownCellCount;
    DisjointSets deletedTrees;

public:
    Table(const std::vector<std::vector<unsigned>> &initValues):
        size(static_cast<unsigned>(initValues.size())),
        rowCounts(size, std::vector<unsigned>(size + 1, 0)),
        columnCounts(size, std::vector<unsigned>(size + 1, 0)),
        unknownCellCount(size * size),
        deletedTrees(size * size + 1) {

        auto actRow = 0U;
        for (const auto &row : initValues) {
            if (row.size() != size) {
                throw InvalidShapeException();
            }

            std::vector<Cell> rowCells;
            auto actColumn = 0U;
            for (auto value : row) {
                if (value == 0 || value > size) {
                    throw InvalidValueException();
                }
                rowCells.emplace_back(value);

                rowCounts[actRow][value]++;
                columnCounts[actColumn][value]++;

                actColumn++;
            }
            cells.push_back(std::move(rowCells));

            actRow++;
        }
    }

    unsigned getSize() const {
        return size;
    }

    auto begin() const {
        return cells.cbegin();
    }

    auto end() const {
        return cells.cend();
    }

    bool isSolved() const {
        return unknownCellCount == 0;
    }

    void finalizeUniqueCells() {
        for (auto r = 0U; r < size; r++) {
            for (auto c = 0U; c < size; c++) {
                auto &cell = cells[r][c];
                if (cell.getState() == Cell::State::Unknown) {
                    const auto value = cell.getValue();
                    if (rowCounts[r][value] == 1 && columnCounts[c][value] == 1) {
                        cell.setState(Cell::State::Final);
                        rowCounts[r][value] = 0;
                        columnCounts[c][value] = 0;
                        unknownCellCount--;
                    }
                }
            }
        }
    }

    void finalizeCell(unsigned row, unsigned column) {
        auto &cell = cells[row][column];
        if (cell.getState() != Cell::State::Unknown) {
            throw MultipleStateChangeException();
        }
        cell.setState(Cell::State::Final);

        const auto value = cell.getValue();
        rowCounts[row][value]--;
        columnCounts[column][value]--;
        unknownCellCount--;
        
        if (columnCounts[column][value] > 0) {
            for (auto r = 0U; r < size; r++) {
                auto &cell = cells[r][column];
                if (r != row && cell.getValue() == value) {
                    switch (cell.getState()) {
                    case Cell::State::Final:
                        throw NoSolutionExistsException("multiple finalized values found in a column");

                    case Cell::State::Unknown:
                        deleteCell(r, column);
                    }
                }
            }
        }

        if (rowCounts[row][value] > 0) {
            for (auto c = 0U; c < size; c++) {
                auto &cell = cells[row][c];
                if (c != column && cell.getValue() == value) {
                    switch (cell.getState()) {
                    case Cell::State::Final:
                        throw NoSolutionExistsException("multiple finalized values found in a row");

                    case Cell::State::Unknown:
                        deleteCell(row, c);
                    }
                }
            }
        }
    }

    void deleteCell(unsigned row, unsigned column) {
        auto &cell = cells[row][column];
        if (cell.getState() != Cell::State::Unknown) {
            throw MultipleStateChangeException();
        }

        auto rOffs = 0;
        auto cOffs = -1;
        for (auto i = 0; i < 4; i++) {
            const auto r = row + rOffs;
            const auto c = column + cOffs;
            if (r >= 0 && r < size && c >= 0 && c < size && cells[r][c].getState() == Cell::State::Deleted) {
                throw NoSolutionExistsException("deleted neighbor found");
            }

            auto t = rOffs;
            rOffs = -cOffs;
            cOffs = t;
        }

        const auto cellSet = static_cast<int>(row * size + column + 1);
        if (row == 0 || row == size - 1 || column == 0 || column == size - 1) {
            deletedTrees.unionSets(0, cellSet);
        }

        rOffs = -1;
        cOffs = -1;
        for (auto i = 0; i < 4; i++) {
            const auto r = row + rOffs;
            const auto c = column + cOffs;
            const auto diagonalNeighborSet = static_cast<int>(r * size + c + 1);
            if (r >= 0 && r < size && c >= 0 && c < size && cells[r][c].getState() == Cell::State::Deleted) {
                const auto diagonalRoot = deletedTrees.findSet(diagonalNeighborSet);
                const auto cellRoot = deletedTrees.findSet(cellSet);
                if (diagonalRoot != cellRoot) {
                    deletedTrees.linkSets(diagonalRoot, cellRoot);

                } else {
                    throw NoSolutionExistsException("circular neighbors found");
                }
            }

            auto t = rOffs;
            rOffs = -cOffs;
            cOffs = t;
        }

        cell.setState(Cell::State::Deleted);

        const auto value = cell.getValue();
        rowCounts[row][value]--;
        columnCounts[column][value]--;
        unknownCellCount--;

        rOffs = 0;
        cOffs = 1;
        for (auto i = 0; i < 4; i++) {
            const auto r = row + rOffs;
            const auto c = column + cOffs;
            if (r >= 0 && r < size && c >= 0 && c < size && cells[r][c].getState() == Cell::State::Unknown) {
                finalizeCell(r, c);
            }

            auto t = rOffs;
            rOffs = -cOffs;
            cOffs = t;
        }
    }

    auto getFinalizeCandidatePos() const {
        auto rMax = 0U;
        auto cMax = 0U;
        auto maxCount = 0U;

        for (auto r = 0U; r < size; r++) {
            for (auto c = 0U; c < size; c++) {
                for (auto v = 1U; v <= size; v++) {
                    const auto &cell = cells[r][c];
                    if (cell.getState() == Cell::State::Unknown) {
                        auto actCount = rowCounts[r][v] + columnCounts[c][v] - 1;
                        if (actCount > maxCount) {
                            maxCount = actCount;
                            rMax = r;
                            cMax = c;
                        }
                    }
                }
            }
        }

        return std::make_pair(rMax, cMax);
    }
};

std::ostream& operator<<(std::ostream &out, const Table &t) {
    auto tableSize = t.getSize();
    auto digits = 0;
    do {
        tableSize /= 10;
        digits++;
    } while (tableSize > 0);

    for (const auto &row : t) {
        for (const auto &cell : row) {
            switch (cell.getState()) {
            case Table::Cell::State::Deleted:
                out << std::setw(digits) << "-";
                break;

            case Table::Cell::State::Final:
                out << std::setw(digits) << cell.getValue();
                break;

            case Table::Cell::State::Unknown:
                out << std::setw(digits) << "?";
                break;
            }
            out << ' ';
        }
        out << std::endl;
    }

    return out;
}

Table readTable(std::istream &in) {
    std::vector<std::vector<unsigned>> values;

    while (!in.eof()) {
        std::string line;
        std::getline(in, line);
        std::stringstream valueStream(line);

        std::vector<unsigned> row;
        while (valueStream.good()) {
            unsigned v;
            valueStream >> v;
            if (valueStream.fail()) {
                if (!valueStream.eof()) {
                    throw std::logic_error("non numeric value encountered");
                }
            } else {
                row.push_back(v);
            }
        }

        if (!row.empty()) {
            if (!values.empty() && values.front().size() != row.size()) {
                throw std::logic_error("invalid table shape");
            }

            values.push_back(std::move(row));
        }
    }

    if (values.empty() || values.size() != values.front().size()) {
        throw std::logic_error("invalid table shape");
    }

    return Table(values);
}

Table solve(Table t) {
    t.finalizeUniqueCells();
    if (t.isSolved()) {
        return t;
    }

    const auto pos = t.getFinalizeCandidatePos();
    try {
        Table tCopy{ t };
        tCopy.finalizeCell(pos.first, pos.second);
        return solve(tCopy);

    } catch (Table::NoSolutionExistsException e) {
        t.deleteCell(pos.first, pos.second);
        return solve(t);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: HitoriSolver <table.txt>\n";
        return 0;
    }

    std::ifstream in(argv[1]);
    if (!in.good()) {
        std::cout << "Unable to open " << argv[1] << " for reading\n";
        return 1;
    }

    try {
        Table t = readTable(in);
        std::cout << solve(t);

    } catch (std::exception e) {
        std::cout << e.what() << std::endl;
        return 2;
    }

    in.close();

    return 0;
}
