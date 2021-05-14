#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>

#include <vector>
#include <string>

#include <numeric>
#include <random>

#include <conio.h>
#include <stdlib.h>

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
    OVER,
    EXIT
};
enum GameEvent
{
    FULL_FIELD,
    EMPTY_FIELD,
    SINGLE_LOOP,
    MULTI_LOOP
};

const std::string gameSettingsNames[] = {
    "dimenshion",
    "n",
    "m",
    "k",
    "seed",
    "probability"
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
    Cell operator[](int i) const { return cells[i]; } // const вариант для cout
    friend ostream& operator<<(ostream& out, const Field1D& field)
    {
        for (int i = 0; i < field.n; i++)
            out << field[i];
        return out;
    }
    bool operator==(const Field1D& other)
    {
        for (size_t i = 0; i < cells.size(); ++i)
            if (!(cells[i].type == other.cells[i].type)) return false;
        return true;
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
    bool operator==(const Field2D& other) // using if n,m is equals
    {
        for (size_t i = 0; i < cells.size(); ++i)
            if (!(cells[i] == other.cells[i])) return false;
        return true;
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
    bool operator==(const Field3D& other) // using if n,m,k is equals
    {
        for (size_t i = 0; i < cells.size(); ++i)
            if (!(cells[i] == other.cells[i])) return false;
        return true;
    }
    virtual void show() override
    {
        std::cout << *this;
    }
};
struct GameSettings
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
};

class GameLoader
{
public:
    GameLoader() = delete;
    GameLoader(const GameLoader&) = delete;

    static void loadGameSettingsFromFile(const std::string path, GameSettings& settings)
    {
        std::ifstream input(path, std::ios_base::in);
        if (!input.is_open()) throw(std::string("Файл не смог открыться"));

        std::string currentLine;
        while (getline(input, currentLine, '\n'))
        {
            auto itRemove = std::remove(currentLine.begin(), currentLine.end(), ' ');
            currentLine.erase(itRemove, currentLine.end());
            std::transform(currentLine.begin(), currentLine.end(), currentLine.begin(), tolower);

            size_t it = currentLine.find('=');
            std::string paramName(&currentLine[0], it);
            std::string paramValue(&currentLine[0] + it + 1, currentLine.size() - it - 1);

            setParamValue(paramName, atof(paramValue.c_str()), settings);
        }
        input.close();
    }
    static void loadGameSettingsToFile(const std::string path, const GameSettings& gs)
    {
        std::ofstream output(path, std::ios_base::out | std::ios_base::trunc);
        if (!output.is_open()) throw(std::string("Файл не смог открыться."));

        output << gameSettingsNames[0] + '=' << gs.dimension << '\n';
        output << gameSettingsNames[1] + '=' << gs.n << '\n';
        output << gameSettingsNames[2] + '=' << gs.m << '\n';
        output << gameSettingsNames[3] + '=' << gs.k << '\n';
        output << gameSettingsNames[4] + '=' << gs.seed << '\n';
        std::string prob = std::to_string(gs.probability);
        std::replace(prob.begin(), prob.end(), '.', ',');
        output << gameSettingsNames[5] + '=' << prob << '\n';

        output.close();
    }

private:
    static void setParamValue(std::string param, double value, GameSettings& gs)
    {
        if (param == gameSettingsNames[0]) gs.dimension = (int)value;
        else if (param == gameSettingsNames[1]) gs.n = (int)value;
        else if (param == gameSettingsNames[2]) gs.m = (int)value;
        else if (param == gameSettingsNames[3]) gs.k = (int)value;
        else if (param == gameSettingsNames[4]) gs.seed = (int)value;
        else if (param == gameSettingsNames[5]) gs.probability = value;
    }
};

struct iGame : public GameSettings, Subject<GameEvent>
{
    virtual void setGame(double p, int s = 0) = 0;
    virtual void runGame(int numIt) = 0;
    virtual ~iGame() { ; }
};

struct Game2D : iGame
{
    Field2D field;
    Field2D fieldNext;
    Field2D fieldLoop;
    Field2D fieldLoopNext;
    unsigned long long stepCount = 0;
    Game2D() { dimension = 2; }
    Game2D(int n, int m){
        this->n = n;
        this->m = m;
        dimension = 2;
        field = fieldNext = fieldLoop = fieldLoopNext = Field2D(n, m);
    }
    virtual void setGame(double p, int s = 0) override
    {
        probability = p;
        seed = s;
        field = fieldNext = fieldLoopNext = Field2D(n, m);
        vector<int> tmp(n * m);
        iota(tmp.begin(), tmp.end(), 0);
        shuffle(tmp.begin(), tmp.end(), std::mt19937(seed));
        for (int i = 0; i < (int)(p * n * m + 0.5); i++)
        {
            int x = tmp[i] / m;
            int y = tmp[i] % m;
            field[x][y].type = TypeCell::alive;
        }
        fieldLoop = field;
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

                    if (stepCount % 2 == 1)
                    {
                        int count = fieldLoop.getNum(i, j);
                        fieldLoopNext[i][j].type = fieldLoop[i][j].type;
                        if (count <= loneliness || count >= overpopulation) fieldLoopNext[i][j].type = TypeCell::env;
                        else if (count >= birth_start && count <= birth_end) fieldLoopNext[i][j].type = TypeCell::alive;
                    }
                }
            }

            int aliveCount = 0;
            for (int i = 0; i < n; i++)
            {
                for (int j = 0; j < m; j++)
                {
                    aliveCount += (field[i][j].type == TypeCell::alive);
                }
            }
            if (aliveCount == 0) { sendEvent(EMPTY_FIELD); return; }
            else if (aliveCount == n * m) { sendEvent(FULL_FIELD); return; }

            if (field == fieldNext) { sendEvent(SINGLE_LOOP); return; }
            field = fieldNext;

            if (field == fieldLoop) { sendEvent(MULTI_LOOP); return; }
            if (stepCount % 2 == 1)
                fieldLoop = fieldLoopNext;

            ++stepCount;
        }
    }
};
struct Game3D : public iGame
{
    Field3D field;
    Field3D fieldNext;
    Field3D fieldLoop;
    Field3D fieldLoopNext;
    unsigned long long stepCount = 0;
    Game3D() { dimension = 3; }
    Game3D(int n, int m, int k) {
        this->n = n;
        this->m = m;
        this->k = k;
        dimension = 3;
        field = fieldNext = Field3D(n, m, k);
        fieldLoop = fieldLoopNext = field;
    }
    virtual void setGame(double p, int s = 0) override
    {
        stepCount = 0;
        probability = p;
        seed = s;
        field = fieldNext = fieldLoopNext = Field3D(n, m, k);
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
        fieldLoop = field;
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

                        if (stepCount % 2 == 1)
                        {
                            count = fieldLoop.getNum(l, i, j, TypeCell::alive, radius);
                            fieldLoopNext[l][i][j].type = fieldLoop[l][i][j].type;
                            if (count <= loneliness || count >= overpopulation) fieldLoopNext[l][i][j].type = TypeCell::env;
                            else if (count >= birth_start && count <= birth_end) fieldLoopNext[l][i][j].type = TypeCell::alive;
                        }
                    }
                }
            }
            double frac = getAliveFraction();
            double epsilon = 0.0001;
            if (frac >= 0.0 - epsilon && frac <= 0.0 + epsilon) { sendEvent(EMPTY_FIELD); return; }
            if (frac >= 1.0 - epsilon && frac <= 1.0 + epsilon) { sendEvent(FULL_FIELD);  return; }

            if (field == fieldNext) { sendEvent(SINGLE_LOOP); return; }
            field = fieldNext;

            if (field == fieldLoop) { sendEvent(MULTI_LOOP); return; }
            if (stepCount % 2 == 1)
                fieldLoop = fieldLoopNext;
            
            ++stepCount;
        }
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
private:
    iGame* game = nullptr;
    iField* field = nullptr;
    GameState currentState = SETUP;
    std::string overMessage = { 0 };
    int frameRate = 500;

public:
    View() = default;
    ~View() { delete game; }

    void start()
    {
        bool isGameRun = true;
        while (isGameRun)
        {
            switch (currentState)
            {
            case SETUP:
                onSetup();
                currentState = READY;
                break;

            case READY:
                field->show();
                std::this_thread::sleep_for(std::chrono::milliseconds(frameRate));
                currentState = RUN;
                break;

            case RUN:
                onRun();
                break;

            case PAUSE:
                onPause();
                break;

            case OVER:
                onOver();
                break;

            case EXIT:
                isGameRun = false;
                break;

            default:
                std::cout << "Неверное состояние игры.\n";
                break;
            }
        }
    }

private:
    void onSetup()
    {
        system("cls");
        overMessage.clear();

        GameSettings settings;
        char answer;
        do {
            std::cout << "Начать новую игру или загрузить? (n - new game, l - load game): ";
            std::cin >> answer;
        } while (answer != 'l' && answer != 'n');

        if (answer == 'n')
            getUserGameSettings(settings);
        else
            loadGameSettings(settings);
        applyGameSettings(settings);
    }

    void onRun()
    {
        system("cls");

        game->runGame(1);
        field->show();
        std::this_thread::sleep_for(std::chrono::milliseconds(frameRate));

        int pressedKey = getPressedKey();
        if (pressedKey == 167 || pressedKey == 'p') currentState = PAUSE; // 167 - 'з'
    }

    void onPause()
    {
        std::cout << "Нажмите:\n"
            "R, чтобы вернуться в SETUP,\n"
            "S, чтобы сохранить игру\n"
            "или любую другую клавишу, чтобы продолжить.\n";
        int key = 0;
        while ((key = getPressedKey()) == -1);
        if (key == 'r' || key == 170) currentState = SETUP; // 170 - 'к'
        else if (key == 's' || key == 235) { saveGameSettings(); currentState = RUN; } // 235 - 'Ы'
        else currentState = RUN;
    }

    void onOver()
    {
        char answer = 0;
        std::cout << "GAME OVER (" << overMessage << ").\n";
        do {
            std::cout << "Сохранить настройки игры? (y/n): ";
            std::cin >> answer;
            std::cout << '\n';
            if (answer == 'y') saveGameSettings();
        } while (answer != 'y' && answer != 'n');
        do {
            std::cout << "Начать сначала? (y/n): ";
            std::cin >> answer;
            std::cout << '\n';
            if (answer == 'y') currentState = SETUP;
            if (answer == 'n') currentState = EXIT;
        } while (answer != 'y' && answer != 'n');
    }

    int getPressedKey()
    {
        if (_kbhit()) return _getch();
        return -1;
    }

    void getUserGameSettings(GameSettings& gs)
    {
        bool isOk;
        do {
            if (std::cin.fail())
            {
                cin.clear(); // на случай, если предыдущий ввод завершился с ошибкой
                cin.ignore(1000, '\n');
            }
            isOk = true;
            std::cout << "Введите размерность поля (dimenshion): ";
            std::cin >> gs.dimension;
            std::cout << "Введите размеры поля (n, m, k), если dimenshion = 2, то k - любое: ";
            std::cin >> gs.n >> gs.m >> gs.k;
            std::cout << "Введите плотность и сид (p, s): ";
            std::cin >> gs.probability >> gs.seed;

            if (gs.dimension != 2 && gs.dimension != 3)
            {
                std::cout << "Неверная размерность поля." << '\n';
                isOk = false;
            }
            else if (gs.n == 0 || gs.m == 0 || (gs.k == 0 && gs.dimension == 3))
            {
                std::cout << "Неверные размеры поля." << '\n';
                isOk = false;
            }
            else if (gs.probability < 0 || gs.probability > 1)
            {
                std::cout << "Неверная плотность." << '\n';
                isOk = false;
            }
            if (!isOk) system("pause");;
            system("cls");
        } while (!isOk);
    }

    void applyGameSettings(GameSettings gs)
    {
        if (gs.dimension != 2 && gs.dimension != 3) throw(std::string("лол, неверные настройки."));
        if (gs.dimension == 2)
        {
            Game2D* pGame = new Game2D();
            field = &(pGame->field);
            game = pGame;
        }
        else if (gs.dimension == 3)
        {
            Game3D* pGame = new Game3D();
            field = &(pGame->field);
            game = pGame;
        }
        *static_cast<GameSettings*>(game) = gs; // set user settings
        game->setGame(gs.probability, gs.seed);
        game->addObserver(*this);
    }

    void saveGameSettings()
    {
        std::string path;
        std::cout << "Введите название файла: ";
        std::cin >> path;
        GameSettings& settings = *static_cast<GameSettings*>(game);
        GameLoader::loadGameSettingsToFile(path, settings);
    }

    void loadGameSettings(GameSettings& settings)
    {
        bool isOk;
        std::string path;
        do {
            std::cout << "Введите название файла: ";
            std::cin >> path;

            isOk = true;
            try
            {
                GameLoader::loadGameSettingsFromFile(path, settings);
            }
            catch (const std::string& e)
            {
                std::cout << e << '\n';
                isOk = false;
            }
        } while (!isOk);
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
        case SINGLE_LOOP:
            overMessage = "SINGLE_LOOP";
            currentState = OVER;
            break;
        case MULTI_LOOP:
            overMessage = "MULTI_LOOP";
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
