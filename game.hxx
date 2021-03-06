#pragma once
#ifndef __GAME__
#define __GAME__
#include<vector>
#include<random>
#include<string>
#include<fstream>
#include"maze.hxx"
#include"ghost.hxx"
#include"random.hxx"
#include"pacman.hxx"
#include"graphics.hxx"
#define x first
#define y second 
using std::vector;

static const int GHOST_RESPAWN_DELAY = 500;
static const int GHOST_EDIBLE_DURATION = 500;
class Cell {
public:
    char type;
    static const char BISCUIT = 0;
    static const char EMPTY = 1;
    static const char FRUIT = 2;
    static const char WALL = 3;
    Cell(char type): type(type) {}
    inline const bool operator == (const Cell& rhs) const {
        return type == rhs.type;
    }
    inline const bool operator != (const Cell& rhs) const {
        return type != rhs.type;
    }
};

class Game {
protected:
    typedef std::uniform_real_distribution<> RealDistribution;
    typedef std::uniform_int_distribution<> IntDistribution;

    int height, width, score, sight, nghosts, nbiscuits, nfruits;
    vector<vector<Cell>> field;
    vector<Ghost> ghosts;
    PacMan pacman;
    Random<IntDistribution, int> rand4;

public:
    static const enum {
        ESC = -1, NONE = 0,
        LEFT = 1, RIGHT = 2, UP = 3, DOWN = 4,
    } ControlSet;


    Game(int height, int width, int sight = 10, int nghosts = 5,
         const std::string& saved_game = "NONE"):
        height(height), width(width), score(0),
        pacman(), sight(sight), rand4(0, 3),
        nghosts(nghosts), nbiscuits(0), nfruits(0), ghosts()
    {
        bool null_filename = saved_game == "NONE";
        bool load_success = ImportGame(saved_game);

        if (null_filename || !load_success) {
            MazeFactory<Cell> factory;
            field = factory.GenerateMaze(width, height);
        }

        static const int colors[] = {GraphicsToolkit::RED, GraphicsToolkit::PINK, GraphicsToolkit::CYAN,
            GraphicsToolkit::YELLOW, GraphicsToolkit::GREEN};
        bool escape_loop = false;
        for (int i = (height + 1) / 2; i >= 0 && !escape_loop; i--) {
            for (int j = (width + 1) / 2; j >= 0 && !escape_loop; j--) {
                if (field[i][j] != Cell::WALL) {
                    int sz_ghost = ghosts.size();
                    ghosts.emplace_back(std::move(Ghost(j, i, colors[sz_ghost % 5])));
                    if (ghosts.size() == nghosts)
                        escape_loop = true;
                }
            }
        }

        if (null_filename || !load_success) {
            Random<RealDistribution, double> real(0., 1.);
            static const double fruit_dist = 0.90;
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    if (field[i][j] == Cell::EMPTY) {
                        field[i][j] = Cell::BISCUIT;
                        ++nbiscuits;
                    } else continue;
                    if (i-1 >= 0 && j-1 >= 0 && i+1 < height && j+1 < width &&
                            field[i+1][j] != Cell::WALL &&
                            field[i-1][j] != Cell::WALL &&
                            field[i][j+1] != Cell::WALL &&
                            field[i][j-1] != Cell::WALL &&
                            real.GetRandom() > fruit_dist) {
                        field[i][j] = Cell::FRUIT;
                        ++nfruits;
                        --nbiscuits;
                    }
                }
            }
        }
    }

    inline bool ImportGame(const std::string& filename) {
        std::fstream fs(filename, std::ios::in);
        if (!fs.is_open()) {
            return false;
        }

        fs >> width >> height;
        fs >> nbiscuits >> nfruits;
        fs >> sight >> nghosts;
        field.resize(height, vector<Cell>(width, Cell::WALL));

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int _type;
                fs >> _type;
                field[i][j].type = _type;
            }
        }

        fs.close();
        return true;
    }


    inline void DrawLegends(void)
    {
        GraphicsToolkit& pen = GraphicsToolkit::GetInstance();
        pen.DrawText(10, height + 1, GraphicsToolkit::RED, "$");
        pen.DrawText(11, height + 1, GraphicsToolkit::CYAN, "$");
        pen.DrawText(12, height + 1, GraphicsToolkit::PINK, "$");
        pen.DrawText(13, height + 1, GraphicsToolkit::GREEN, "$");
        pen.DrawText(14, height + 1, GraphicsToolkit::YELLOW, "$");
        pen.DrawText(15, height + 1, GraphicsToolkit::YELLOW, ": ghosts");

        pen.DrawText(10, height + 2, GraphicsToolkit::GRAY, "$: dead ghosts");
        pen.DrawText(10, height + 3, GraphicsToolkit::WHITE, "$: edible ghosts");
        pen.DrawText(10, height + 4, GraphicsToolkit::LEMON, "@: you");
    }

    inline void Draw(long long current_time)
    {
        GraphicsToolkit& pen = GraphicsToolkit::GetInstance();
        int x = pacman.position().x,
            y = pacman.position().y;
        vector<pair<int,int>> gp;
        vector<int> gc;
        for (auto& ghost: ghosts) {
            gp.emplace_back(ghost.position());
            gc.emplace_back(ghost.color(current_time));
        }
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                bool gfound = false;
                for (int k = 0; k < nghosts; k++) {
                    if (gp[k].x == j && gp[k].y == i) {
                        gfound = true;
                        pen.DrawPoint(j, i, gc[k], '$');
                        break;
                    }
                }
                if (gfound) continue;
                if (j == x && i == y) {
                    pen.DrawPoint(j, i, GraphicsToolkit::LEMON, '@');
                } else if (field[i][j] == Cell::BISCUIT) {
                    pen.DrawPoint(j, i, GraphicsToolkit::YELLOW, '.');
                } else if (field[i][j] == Cell::FRUIT) {
                    pen.DrawPoint(j, i, GraphicsToolkit::GREEN, '*');
                } else if (field[i][j] == Cell::WALL) {
                    pen.DrawPoint(j, i, GraphicsToolkit::BLUE, '#');
                } else {
                    pen.DrawPoint(j, i, GraphicsToolkit::BLACK, ' ');
                }
            }
        }
        char score_buffer[100] = {};
        char life_buffer[100] = {};
        sprintf(score_buffer, "Score: %d", score);
        sprintf(life_buffer, "Life: %d", pacman.lives_left());
        pen.DrawText(0, height+1, GraphicsToolkit::YELLOW, life_buffer);
        pen.DrawText(0, height+2, GraphicsToolkit::PINK, score_buffer);
        if (current_time < 10) {
            DrawLegends();
        }
    }

    inline bool Blocked(int x, int y, bool block_overlap = false) const {
        bool chk = !(x >= 0 && y >= 0 && x < width && y < height &&
                    field[y][x] != Cell::WALL);
        if (chk || !block_overlap) return chk;
        for (const auto& ghost: ghosts) {
            auto gp = ghost.position();
            if (gp.x == x && gp.y == y)
                return true;
        }
        return chk;
    }

    static inline int Distance(int x, int y, int nx, int ny) {
        return abs(x - nx) + abs(y - ny);
    }

    template<typename Function>
    inline void Process(long long current_time, const Function& input_source)
    {
        static const int dx[4] = {-1, 0, 1, 0},
                         dy[4] = {0, 1, 0, -1};
        GraphicsToolkit& pen = GraphicsToolkit::GetInstance();
        int x = pacman.position().x,
            y = pacman.position().y,
            gpx[ghosts.size()],
            gpy[ghosts.size()],
            ged[ghosts.size()],
            gdd[ghosts.size()],
            ptr;

        for (auto& ghost: ghosts) {
            if (ghost.dead(current_time)) continue;
            if (ghost.position() == pacman.position()) {
                if (ghost.edible(current_time)) {
                    ghost.set_dead(current_time + GHOST_RESPAWN_DELAY);
                    score += 100;
                } else {
                    if (pacman.lives_left()) {
                        pacman.set_dead();
                    } else {
                        EndGame();
                    }
                }
            }
        }

        ptr = 0;
        for (auto& ghost: ghosts) {
            int gx = ghost.position().x,
                gy = ghost.position().y,
                ge = ghost.edible(current_time),
                gd = ghost.dead(current_time),
                cur_distance = Distance(gx, gy, x, y);
            std::tie(gpx[ptr], gpy[ptr], ged[ptr], gdd[ptr]) = 
                std::tie(gx, gy, ge, gd);
            bool moved = false, is_random = true;;
            if (cur_distance <= sight) {
                for (int k = 0; k < 4; k++) {
                    int nx = gx + dx[k],
                        ny = gy + dy[k],
                        nxt_distance = Distance(nx, ny, x, y);
                    if (!Blocked(nx, ny, true) &&
                        (ghost.edible(current_time) ? nxt_distance > cur_distance :
                                                      nxt_distance < cur_distance)) {
                        if (ghost.move(current_time, dx[k], dy[k], false)) {
                            moved = true;
                            break;
                        }
                    }
                }
                if (!moved) {
                    is_random = false;
                }
            }
            if (!moved) {
                for (int t = 10; t --> 0;) {
                    int k = rand4.GetRandom();
                    int nx = gx + dx[k],
                        ny = gy + dy[k];
                    if (!Blocked(nx, ny, true)) {
                        if (ghost.move(current_time, dx[k], dy[k], is_random)) {
                            break;
                        }
                    }
                }
            }
            ++ptr;
        }

        if (field[y][x] == Cell::BISCUIT) {
            field[y][x] = Cell::EMPTY;
            score += 1;
            if (--nbiscuits == 0) {
                // Victory!
                score += (1 + pacman.lives_left()) << 11;
                score += nfruits << 8;
                EndGame();
            }
        } else if (field[y][x] == Cell::FRUIT) {
            field[y][x] = Cell::EMPTY;
            score += 50;
            for (auto& ghost: ghosts) {
                ghost.set_edible(current_time + GHOST_EDIBLE_DURATION);
            }
        }

        switch(input_source(field, ghosts, pacman)) {
            case LEFT:
                if (!Blocked(x-1, y))
                    pacman.move(-1, 0);
                break;
            case RIGHT:
                if (!Blocked(x+1, y))
                    pacman.move(1, 0);
                break;
            case UP:
                if (!Blocked(x, y-1))
                    pacman.move(0, -1);
                break;
            case DOWN:
                if (!Blocked(x, y+1))
                    pacman.move(0, 1);
                break;
            case ESC:
                EndGame();
        }

        for (int i = 0; i < ghosts.size(); i++) {
            if (ghosts[i].position() == std::make_pair(x, y) &&
                pacman.position() == std::make_pair(gpx[i], gpy[i])) {
                if (ged[i]) {
                    ghosts[i].set_dead(current_time + GHOST_RESPAWN_DELAY);
                } else if (!gdd[i]) {
                    if (pacman.lives_left()) {
                        pacman.set_dead();
                    } else {
                        EndGame();
                    }
                }
            }
        }

        Draw(current_time);
    }

    inline void EndGame(void) {
        GraphicsToolkit& pen = GraphicsToolkit::GetInstance();
        char score_buffer[64] = {};
        int ptr = sprintf(score_buffer, "Your final score: %d", score);
        sprintf(score_buffer + ptr, "Press any key to exit.");
        pen.DrawMessageBox(0, 0, GraphicsToolkit::LEMON, score_buffer);
        getchar();
        pen.EndGraphics();
        printf("[PacMan] Final Score: %d\n", score); 
        exit(0);
    }
};
#undef x
#undef y
#endif
