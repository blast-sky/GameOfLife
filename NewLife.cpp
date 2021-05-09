#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <random>

#include <Windows.h>
#include <conio.h>

#include "Observer.hpp"

using namespace std;

// 0 -------\ Y (m)
//   -------/
// ||
// ||
// ||
// ||
// \/ X (n)

enum GameState
{
    SETUP,
    READY,
    RUN,
    PAUSE,
    OVER
};

enum GameEvent
{
    FULL_FIELD,
    EMPTY_FIELD
};

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

struct iField
{
    virtual void show() = 0;
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
    Cell operator[](int i) const { return cells[i]; } // const вариант дл€ cout
    friend ostream& operator<<(ostream& out, const Field1D& field)
    {
        for (int i = 0; i < field.n; i++)
            out << field[i];
        return out;
    }
};
struct Field2D : iField
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
    virtual void show() override
    {
        std::cout << *this;
    }
};
struct Field3D : iField
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
    virtual void show() override
    {
        std::cout << *this;
    }
};
struct iGame
{
    virtual void runGame(int numIt) = 0;
};

struct GameSettings
{
    int n = 0;
    int m = 0;
    int k = 0;

    int seed = 0; // случайна€ величина дл€ генератора
    double probability = 0.0;  // веро€тность того, что клетка жива€
    int dimension = 1; // размерность

    int radius = 1; // радиус проверки, граница включена
    int loneliness = 2; // с этого числа и меньше клетки умирают от одиночества
    int birth_start = 3; // с этого числа и до birth_end по€вл€етс€ жива€ клетка
    int birth_end = 3;
    int overpopulation = 5; // с этого числа и дальше клетки погибают от перенаселени€
};

struct Game2D : iGame, GameSettings
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
};
struct Game3D : iGame, GameSettings, Subject<GameEvent>
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
        double frac = getAliveFraction();
        double epsilon = 0.0001;
        if (frac >= 0.0 - epsilon && frac <= 0.0 + epsilon) { sendEvent(EMPTY_FIELD); return; }
        if (frac >= 1.0 - epsilon && frac <= 1.0 + epsilon) { sendEvent(FULL_FIELD); return; }
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

struct View : Observer<GameEvent>
{
    iGame* game = nullptr;
    iField* field = nullptr;
    GameState currentState = SETUP;
    std::string overMessage = { 0 };

    void start()
    {
        bool isGameRun = true;
        while (isGameRun)
        {
            switch (currentState)
            {
            case SETUP:
            {
                system("cls");

                overMessage.clear();
                applyGameSettings(getGameSettings());

                currentState = READY;
                break;
            }

            case READY:
                field->show();
                Sleep(500);

                currentState = RUN;
                break;

            case RUN:
            {
                system("cls");

                game->runGame(1);
                field->show();
                Sleep(500);

                int pressedKey = getPressedKey();
                if (pressedKey == 'з' || pressedKey == 'p') currentState = PAUSE;
                break;
            }
            case PAUSE:
            {
                std::cout << "Ќажмите R, чтобы вернутьс€ в SETUP, или любую клавишу, чтобы продолжить.\n";
                int key = 0;
                while((key = getPressedKey()) == -1);
                if (key == 'r' || key == 'к') currentState = SETUP;
                else currentState = RUN;
                break;
            }
            case OVER:
            {
                char answer = 0;
                //if 
                std::cout << "GAME OVER (" << overMessage << ").\n";
                do {
                    std::cout << "Ќачать сначала? (y/n): ";
                    std::cin >> answer;
                    std::cout << '\n';
                    if (answer == 'y') currentState = SETUP;
                    if (answer == 'n') exit(0);
                } while (currentState == OVER);
                break;
            }
            default:
                std::cout << "Ќеверное состо€ние игры.\n";
                break;
            }
        }
    }

    int getPressedKey()
    {
        if (_kbhit()) return _getch();
        return -1;
    }

    GameSettings getGameSettings()
    {
        GameSettings gs;
        bool isOk = true;
        do {
            std::cout << "¬ведите размеры пол€ (n, m, k): ";
            std::cin >> gs.n >> gs.m >> gs.k;
            std::cout << "¬ведите плотность и сид (p, s): ";
            std::cin >> gs.probability >> gs.seed;
            if (gs.n == 0 || gs.m == 0 || gs.k == 0)
            {
                std::cout << "Ќеверные размеры пол€." << '\n';
                system("pause");
                isOk = false;
            }
            else if (gs.probability < 0 || gs.probability > 1)
            {
                std::cout << "Ќеверна€ плотность." << '\n';
                system("pause");
                isOk = false;
            }
            system("cls");
        } while (!isOk);
        return gs;
    }

    void applyGameSettings(GameSettings gs)
    {
        game = new Game3D(gs.n, gs.m, gs.k);
        static_cast<Game3D*>(game)->setGame(gs.probability, gs.seed);
        static_cast<Game3D*>(game)->addObserver(*this);
        field = &(static_cast<Game3D*>(game)->field);
    }

    virtual void newEvent(GameEvent event) override
    {
        switch (event)
        {
        case FULL_FIELD:
            overMessage = "FULL_FIELD";
            currentState = OVER;
            break;
        case EMPTY_FIELD:
            overMessage = "EMPTY_FIELD";
            currentState = OVER;
            break;
        default:
            break;
        }
    }
};

int main()
{
    setlocale(LC_ALL, "ru");

    View view;
    view.start();

    return 0;
}
