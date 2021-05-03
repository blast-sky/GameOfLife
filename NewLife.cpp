#include <vector>
#include <Windows.h>
#include <list>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <random>
using namespace std;
// 0 -------\ Y (m)
//   -------/
// ||
// ||
// ||
// ||
// \/ X (n)
enum TypeCell
{
    env,
    alive
};
struct Cell
{
    TypeCell type;
    friend ostream& operator<<(ostream& out, const Cell& cell)
    {
        if (cell.type == env) out << '.';
        else if (cell.type == alive) out << '#';
        return out;
    }
};
struct Field1D
{
    int n = 0;
    vector<Cell> cells;
    Field1D(int n):n(n), cells(vector<Cell>(n)) {}
    int getNum(int pos, TypeCell type = alive, int radius = 1) const
    {
        int count = 0;
        for (int i = pos - radius; i <= pos + radius; i++)
            if (cells[(i + n) % n].type == type)
                count++;
        return count;
    }
    Cell& operator[](int i) { return cells[i]; }
    Cell operator[](int i) const { return cells[i]; } // const вариант для cout
    friend ostream& operator<<(ostream& out, const Field1D& field)
    {
        for (int i = 0; i < field.n; i++)
            out << field[i];
        return out;
    }
};
struct Field2D
{
    int n = 0;
    int m = 0;
    vector<Field1D> cells;
    Field2D() {}
    Field2D(int n, int m): n(n), m(m), cells(vector<Field1D>(n, Field1D(m))) {}
    int getNum(int posX, int posY, TypeCell type = alive, int radius = 1) const
    {
        int count = 0;
        for (int i = posX - radius; i <= posX + radius; i++)
            count += cells[(i + n) % n].getNum(posY, type, radius);
        return count;
    }
    Field1D& operator[](int i) { return cells[i]; }
    Field1D operator[](int i) const { return cells[i]; }
    friend ostream& operator<<(ostream& out, const Field2D& field)
    {
        for (int i = 0; i < field.n; i++)
            out << field[i] << "\n";
        return out;
    }
};
struct Field3D
{
    int n = 0;
    int m = 0;
    int k = 0;
    vector<Field2D> cells;
    Field3D() = default;
    Field3D(int n, int m, int k) : n(n), m(m), k(k), cells(vector<Field2D>(k, Field2D(n, m))) {}

    int getNum(int posZ, int posX, int posY, TypeCell type = alive, int radius = 1) const
    {
        int count = 0;
        for (int i = posZ - radius; i <= posZ + radius; i++)
        {
            count += cells[(i + k) % k].getNum(posX, posY, type, radius);
        }
        return count;
    }
    Field2D& operator[](int i) { return cells[i]; }
    Field2D operator[](int i) const { return cells[i]; }
    friend ostream& operator<<(ostream& out, const Field3D& field)
    {
        for (int i = 0; i < field.k; i++)
            out << i << ":\n" << field[i] << "\n";
        return out;
    }
};
struct iGame
{
    int n = 0;
    int m = 0;
    int k = 0;

    int seed = 0; // случайная величина для генератора
    double probability = 0.0;  // вероятность того, что клетка живая
    int dimension = 1; // размерность

    int radius = 1; // радиус проверки, граница включена
    int loneliness = 2; // с этого числа и меньше клетки умирают от одиночества
    int birth_start = 3; // с этого числа и до birth_end появляется живая клетка
    int birth_end = 3;
    int overpopulation = 5; // с этого числа и дальше клетки погибают от перенаселения
    virtual void runGame(int numIt) = 0;
};

struct Game2D : public iGame
{
    Field2D field;
    Field2D fieldNext;
    Game2D() { dimension = 2; }
    Game2D(int n, int m){
        this->n = n;
        this->m = m;
        dimension = 2;
        field = fieldNext = Field2D(n, m);
    }
    void setGame(double p, int s = 0)
    {
        probability = p;
        seed = s;
        field = Field2D(n, m);
        vector<int> tmp(n * m);
        iota(tmp.begin(), tmp.end(), 0);
        shuffle(tmp.begin(), tmp.end(), std::mt19937(seed));
        for (int i = 0; i < (int)(p * n * m + 0.5); i++)
        {
            int x = tmp[i] / m;
            int y = tmp[i] % m;
            field[x][y].type = TypeCell::alive;
        }
    }
    void runGame(int numIt) override
    {
        for (int it = 0; it < numIt; it++)
        {
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < m; j++)
                {
                    int count = field.getNum(i, j);
                    fieldNext[i][j].type = field[i][j].type;
                    if (count <= loneliness || count >= overpopulation) fieldNext[i][j].type = TypeCell::env;
                    else if (count >= birth_start && count <= birth_end) fieldNext[i][j].type = TypeCell::alive;
                }
            }
            field = fieldNext;
        }
    }
    void startGame()
    {

    }
};
// реализовать Game3D
struct Game3D : iGame
{
    Field3D field;
    Field3D fieldNext;
    Game3D() { dimension = 3; }
    Game3D(int n, int m, int k) {
        this->n = n;
        this->m = m;
        this->k = k;
        dimension = 3;
        field = fieldNext = Field3D(n, m, k);
    }
    double getAliveFraction()
    {
        int aliveCount = 0;
        for (int z = 0; z < field.k; ++z)
        {
            for (int x = 0; x < field.n; ++x)
            {
                for (int y = 0; y < field.m; ++y)
                {
                    aliveCount += (field[z][x][y].type == TypeCell::alive);
                }
            }
        }
        return double(aliveCount) / (double(field.k) * double(field.m) * double(field.n));
    }
    void setGame(double p, int s = 0)
    {
        probability = p;
        seed = s;
        field = Field3D(n, m, k);
        vector<int> tmp(n * m * k);
        iota(tmp.begin(), tmp.end(), 0);
        shuffle(tmp.begin(), tmp.end(), std::mt19937(seed));
        for (int i = 0; i < (int)(p * n * m * k + 0.5); i++)
        {
            int z = tmp[i] / (n * m);
            int x = tmp[i] % (n * m) / m;
            int y = tmp[i] % (n * m) % m;
            field[z][x][y].type = TypeCell::alive;
        }
    }
    void runGame(int numIt) override
    {
        for (int it = 0; it < numIt; it++)
        {
            for (int l = 0; l < k; l++)
            {
                for (int i = 0; i < n; i++)
                {
                    for (int j = 0; j < m; j++)
                    {
                        int count = field.getNum(l, i, j, TypeCell::alive, radius);
                        fieldNext[l][i][j].type = field[l][i][j].type;
                        if (count <= loneliness || count >= overpopulation) fieldNext[l][i][j].type = TypeCell::env;
                        else if (count >= birth_start && count <= birth_end) fieldNext[l][i][j].type = TypeCell::alive;
                    }
                }
            }
            field = fieldNext;
        }
    }
    void startGame()
    {

    }
};

void doExperiment(Game3D& g3d, const Field3D& baseField)
{
    for (int r = 1; r <= 2; ++r)
    {
        for (int ll = 0; ll <= 25; ++ll)
        {
            for (int bs = ll + 1; bs <= 26; ++bs)
            {
                for (int be = bs; be <= 26; ++be)
                {
                    for (int op = be + 1; op <= 27; ++op)
                    {
                        g3d.field = baseField;

                        g3d.radius = r;
                        g3d.loneliness = ll;
                        g3d.birth_start = bs;
                        g3d.birth_end = be;
                        g3d.overpopulation = op;

                        g3d.runGame(20);

                        double frac = g3d.getAliveFraction();
                        if (frac > 0.15 && frac < 0.2)
                        {
                            std::cout << r << ' ' << ll << ' ' << bs << ' ' << be << ' ' << op << '\n';
                        }
                    }
                }
            }
        }
    }
};

int main()
{
    Game3D g3d(4, 4, 4);

    Field3D baseField1(4, 4, 4);
    baseField1[1][0][1].type = TypeCell::alive;
    baseField1[1][2][3].type = TypeCell::alive;
    baseField1[2][0][1].type = TypeCell::alive;
    baseField1[2][2][2].type = TypeCell::alive;
    baseField1[3][2][2].type = TypeCell::alive;
    baseField1[3][3][1].type = TypeCell::alive;

    Field3D baseField2(4, 4, 4);
    for (int l = 0; l < 4; ++l)
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                baseField2[l][i][j].type = TypeCell::alive;
            }
        }
    }
    baseField2[2][1][0].type = TypeCell::env;
    baseField2[2][0][2].type = TypeCell::env;
    baseField2[3][2][1].type = TypeCell::env;
    

    doExperiment(g3d, baseField2); // baseField1 change to baseField2

    return 0;
}

// 0) Cell, Field, type Enum, operator <<
// 1) view в данном варианте не нужен, достаточно иметь Field
// 2) operator[] and "operator[][]" and operator(int i, int j, int k)
// 3) iota, random
// 4) view должен быть связан с game, чтобы получать от него "обновления" и взаимодействовать с ним
// 4) например, мы хотим реализовать меню для игры: паузу, донастройку, чтобы игра сообщала о некоторых событиях.

struct iView
{
    char livingCell; // символ "живой" клетки
    char dyingCell; // символ "неживой" клетки
    // ... свобода творчества для реализации и взаимодействия view, можно реализовать 2d draw и использвать его в 3d
};
struct View2d : iView
{
    void draw(char** field, int n, int m)
    {
        system("cls");
        for (int i = 0; i < n; i++)
            fwrite(field[i], sizeof(char), m + 1, stdout);
    }
};
struct View3d : iView
{
    char*** field;
    void setField(char*** field) {};
    void draw()
    {

    }
};