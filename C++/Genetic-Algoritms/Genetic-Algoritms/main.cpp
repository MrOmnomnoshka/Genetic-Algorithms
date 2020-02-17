#include <SFML/Graphics.hpp>
#include <iostream>

int born = 0;

using namespace std;

enum DrawMode { Default, Energy, Minerals };
enum Actions { Catch, Move, Share };
enum Entities { BOT = 2, DEAD, FREE };  // 2 - бот, 3 - труп, 4 - пусто.

// Параметры мира(можно менять)
const int width = 1280;
const int height = 720;
const int length = 20;
int fps = 60;
bool paint = true;
// /////////////////////////////////////

// Bots settings(changeable)
int baby_mutation_index = 1;  // every "N" bot's child mutates
int max_mut = 5;  // Максимальное кол - во мутаций в боте
int energy_limit = 255;
int minerals_limit = 51;
int age_limit = 50;  // energy -= age / age_limit
bool birth_to_die = false;  // die if there is no space to birth
bool bot_can_step_on = false;  // bot can move on another bot (Eat it)
bool forced_reproduction = true;  // if Energy is max, than give birth
// /////////////////////////////////////

// Параметры мира(не трогать)
const int world_size = (width * height) / (length * length);

int iteration = 0;
int generation = 0;

int map_height = height / length;
int map_width = width / length;

DrawMode draw_mode = Default;

bool game = true;

// Load Graphic Settings
sf::RenderWindow window(sf::VideoMode(width, height), "New World");
sf::Font font;
sf::Event event;
sf::Clock Clock;
sf::Clock Timer; // starts the clock
sf::RectangleShape rectangle;
// /////////////////////////////////////

class Bot;
Bot *bots[world_size];
void interact(Bot *active, int x, int y, int action);
void swap_bots(int a_index, int b_index);
int re_range(int old_max, int old_min, int new_max, int new_min, int value);


class Bot {
public:  // Странные типы переменных для меньшего пожирания памяти
		 //(возможно и быстродействия), можно поставить везде int и bool
	short x;
	short y;
	short energy = 35;
	int dna[64];
	int color[3];
	short age = 0;
	short amount_of_children = 0;
	char direction = 0;
	char minerals = 0;
	char counter = 0;
	char state = FREE;
	bool clone = false;
	bool was_calculated = false;

	short free_space[8 * 2 + 1];  // [x,y]*8 + length
	short new_x;
	short new_y;
	char new_direction;
	short new_entity;

	void CreateBot(int newX, int newY, int* newColor) {
		x = newX;
		y = newY;
		color[0] = newColor[0];
		color[1] = newColor[1];
		color[2] = newColor[2];
		(*bots[newY * map_width + newX]).state = BOT;
		was_calculated = true;  // can move only on next turn
	}

	void reset() {
		(*bots[y * map_width + x]).state = FREE;
		direction = 0;
		counter = 0;
		age = 0;
		minerals = 0;
		amount_of_children = 0;
		energy = 35;
		clone = false;
	}

	// ///////////////////////////////////// (вспомогательные)
	void translate(int way) {
		// Вверх, Вверх-вправо, Вправо, Вниз-вправо, Вниз, Вниз-влево, Влево, Вверх-влево
		int options[8][2] = { {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1} };

		new_x = x + options[way][0];
		new_y = y + options[way][1];
		new_direction = way;

		// Зацикливаем мир горизонтально
		if (new_x >= map_width)
			new_x = 0;
		else if (new_x < 0)
			new_x = map_width - 1;

		// Ограничиваем мир вертикально
		if (new_y >= map_height)
			new_y = map_height - 1;
		else if (new_y < 0)
			new_y = 0;
		// /////////////////////////////////////
	}

	void try_to_move(int entity, int x, int y) {
		if (entity == FREE)  // Если там пусто
			just_go(x, y);

		else if (entity == DEAD)  // Если там труп
		{
			(*bots[y * map_width + x]).state = FREE;
			add_red();
			add_energy(10);
			just_go(x, y);
		}

		else if (entity == BOT && bot_can_step_on)  // Another bot
			interact(this, x, y, Move);
	}

	void try_to_catch(int entity, int x, int y) {
		if (entity == DEAD) {  // Если там труп
			add_red();
			add_energy(10);

			(*bots[y * map_width + x]).state = FREE;
		}

		else if (entity == 2)  // Another bot
			interact(this, x, y, Catch);
	}

	void just_go(int n_x, int n_y) {
		swap_bots(y * map_width + x, n_y * map_width + n_x);
	}

	int get_next_dna(int dist) {
		return (counter + dist) % 64;
	}

	void add_energy(int en) {
		energy = min(energy_limit, energy + en + minerals);
	}

	void add_minerals(int mineral) {
		minerals = min(minerals_limit, minerals + mineral);
	}

	void increase_counter(int value) {
		counter = (counter + value) % 64;
	}

	void standard_actions() {
		int next_dna = get_next_dna(1);  // Check what to do
		int way = (dna[next_dna] + direction) % 8;  // Check way to do
		translate(way);  // Get new world X, Y and direction

		new_entity = (*bots[new_y * map_width + new_x]).state;  // Get an entity that was there
		next_dna = get_next_dna(new_entity);  // Depending on what was there...
		increase_counter(dna[next_dna]);  // ...move counter along
	}

	void add_red() {
		color[0] = min(255, color[0] + 5);  // red
		color[1] = max(0, color[1] - 1); // green
		color[2] = max(0, color[2] - 1); // blue
	}

	void add_green() {
		color[1] = min(255, color[1] + 5); // green
		color[0] = max(0, color[0] - 1); // red
		color[2] = max(0, color[2] - 1); // blue
	}

	void add_blue() {
		color[2] = min(255, color[2] + 5);  // blue
		color[1] = max(0, color[1] - 1);  // green
		color[0] = max(0, color[0] - 1);  // red
	}

	int* get_mode_color() {
		int new_color[3];
		if (draw_mode == Energy) {
			new_color[0] = 255;
			new_color[1] = 255 - energy;
			new_color[2] = 0;
		}
		else if (draw_mode == Minerals) {
			new_color[0] = 0;
			new_color[1] = 255 - minerals * 5;
			new_color[2] = 255;
		}
		else
			return color;
		return new_color;
	}

	void find_free_space() {
		int amount = 0;
		for (int i = 0; i < 8; i++)
		{
			translate(i);
			if ((*bots[new_y * map_width + new_x]).state == FREE) {
				free_space[amount++] = new_x;
				free_space[amount++] = new_y;
			}
		}
		free_space[16] = amount / 2;
	}
	// ///////////////////////////////////// (вспомогательные)
	// 2 - бот, 3 - труп, 4 - пусто.

	// 0
	void move() {
		energy -= 1 + age / age_limit;  // Decrease energy
		standard_actions();

		try_to_move(new_entity, new_x, new_y);  // Try to do some action
	}

	// 1
	void bot_catch() {
		energy -= 1 + age / age_limit;
		standard_actions();

		try_to_catch(new_entity, new_x, new_y);
	}

	// 2
	void look() {
		standard_actions();  // Just to move counter
	}

	// 3
	void turn() {  // 16-23
		int next_dna = get_next_dna(1);
		int way = (dna[next_dna] + direction) % 8;
		translate(way);

		direction = new_direction;
		increase_counter(2);
	}

	// 4
	void photosynthesis() {
		int before = energy;
		energy -= 1 + age / age_limit;
		if (y < map_height / 2) {
			int energy_to_add = re_range(0, map_height / 2, 20, 0, y);
			add_energy(energy_to_add);
		}
		if (energy > before)  // if get energy
			add_green();
		increase_counter(1);
	}

	// 5
	void give_birth() {
		energy -= 1 + age / age_limit;
		clone = true;
		increase_counter(1);
	}

	// 6
	void chemosynthesis() {
		int before = energy;
		energy -= 1 + age / age_limit;
		if (y > map_height / 2) {
			int energy_to_add = re_range(map_height, map_height / 2, 10, 0, y);
			add_energy(energy_to_add);
		}
		if (energy > before)  // if get energy
			add_blue();
		increase_counter(1);
	}

	// 7
	void collect_mineral() {
		energy -= 1 + age / age_limit;
		if (y > map_height / 2) {
			int minerals_to_add = re_range(map_height, map_height / 2, 10, 0, y);
			add_minerals(minerals_to_add);
			add_blue();
		}
		increase_counter(1);
	}

	// 8
	void mine_to_energy() {
		energy -= 1 + age / age_limit;
		int needed_energy = energy_limit - energy;
		int minerals_to_convert = minerals - needed_energy / 5;
		if (minerals_to_convert < 0) {
			add_energy(minerals * 5);
			minerals = 0;
		}
		else {
			add_energy(minerals_to_convert * 5);
			minerals -= minerals_to_convert;
		}
	}

	// 9
	void how_much_energy() {
		int next_dna = get_next_dna(1);
		int energy_needed = re_range(64, 0, energy_limit, 0, dna[next_dna]);

		if (energy < energy_needed) {
			next_dna = get_next_dna(2);
			increase_counter(dna[next_dna]);
		}
		else {
			next_dna = get_next_dna(3);
			increase_counter(dna[next_dna]);
		}
	}

	// 10
	void surrounded() {
		find_free_space();
		if (free_space[16] != 0) {
			int next_dna = get_next_dna(1);
			increase_counter(dna[next_dna]);
		}
		else {
			int next_dna = get_next_dna(2);
			increase_counter(dna[next_dna]);
		}
	}

	// 11
	void share_energy() {
		energy -= 1 + age / age_limit;
		int next_dna = get_next_dna(1);
		int way = (dna[next_dna] + direction) % 8;

		translate(way);  // Get new world X, Y and direction

		if ((*bots[new_y * map_width + new_x]).state == BOT)
			interact(this, x, y, Share);
		increase_counter(2);
	}

	// 12
	void check_height() {
		int next_dna = get_next_dna(1);
		int required_height = re_range(64, 0, map_height, 0, dna[next_dna]);

		if (y >= required_height) {
			next_dna = get_next_dna(2);
			increase_counter(dna[next_dna]);
		}
		else {
			next_dna = get_next_dna(3);
			increase_counter(dna[next_dna]);
		}
	}

	// 13
	void how_much_minerals() {
		int next_dna = get_next_dna(1);
		int minerals_needed = re_range(64, 0, minerals_limit, 0, dna[next_dna]);

		if (minerals < minerals_needed) {
			next_dna = get_next_dna(2);
			increase_counter(dna[next_dna]);
		}
		else {
			next_dna = get_next_dna(3);
			increase_counter(dna[next_dna]);
		}
	}

	// ... - 63
	void jump() {  // увеличить указатель комманд
		counter = (counter + dna[counter]) % 64;
	}
};


void create_all_bots() {
	for (int i = 0; i < world_size; i++)
	{
		bots[i] = new Bot;
		(*bots[i]).y = i / map_width;
		(*bots[i]).x = i - i / map_width * map_width;
	}
}


void interact(Bot *active, int x, int y, int action) {
	// If passive exist
	int passive_index = y * map_width + x;
	if ((*bots[passive_index]).state == BOT) {
		// Attack
		if (action == Catch) {
			if ((*active).energy > (*bots[passive_index]).energy) {
				(*active).add_red();
				(*active).add_energy((*bots[passive_index]).energy / 4);
				(*bots[passive_index]).energy = 0;
			}
		}

		// Step on
		if (action == Move) {
			if ((*active).energy > (*bots[passive_index]).energy) {
				(*active).add_red();
				(*active).add_energy((*bots[passive_index]).energy / 4);
				(*bots[passive_index]).energy = 0;
				(*bots[passive_index]).reset();
				(*active).just_go(x, y);
			}
		}

		// Share
		if (action == Share) {
			int quarter = (*active).energy / 4;
			(*active).energy -= quarter;
			(*bots[passive_index]).add_energy(quarter);
		}
	}
}


void mutate_bot(Bot *bot) {
	for (int i = 0; i < rand() % max_mut; i++)
	{
		(*bot).dna[rand() % 64] = rand() % 64;
	}
}


void clone_bot(Bot *bot) {
	(*bot).find_free_space();								// TODO:  x x x  -  func returns(1,1,2,3,4,5,5)
	if ((*bot).free_space[16] != 0 && (*bot).energy > 35) {	//		  5 B 1		cause map is on end.
		(*bot).energy -= 35;						 		//		  4 3 2
		(*bot).amount_of_children += 1;

		int index = rand() % (*bot).free_space[16];
		int baby_x = (*bot).free_space[index * 2];
		int baby_y = (*bot).free_space[index * 2 + 1];
		int* baby_color = (*bot).color;

		int baby_index = baby_y * map_width + baby_x;
		(*bots[baby_index]).CreateBot(baby_x, baby_y, baby_color);

		// Copy parent DNA
		for (int i = 0; i < 64; i++)
			(*bots[baby_index]).dna[i] = (*bot).dna[i];
		// Mutate
		if ((*bot).amount_of_children % baby_mutation_index == baby_mutation_index - 1)
			mutate_bot(bots[baby_index]);
		born += 1;
	}
	else if (birth_to_die)
		(*bot).energy = 0;
}


void bot_actions(int bot_num) {
	int actions_limit = 0;
	bool bot_turn = true;
	int code;

	Bot* bot = bots[bot_num];  // current bot
	while (actions_limit < 15 && bot_turn && (*bot).energy > 0) {
		code = (*bot).dna[(*bot).counter];

		switch (code) {
		case 0:  // движение (заверщающий)
			(*bot).move();
			bot_turn = false;
			break;
		case 1:  // схватить (заверщающий)
			(*bot).bot_catch();
			bot_turn = false;
			break;
		case 2:  // посмотреть
			(*bot).look();
			actions_limit += 1;
			break;
		case 3:  // повернуться
			(*bot).turn();
			actions_limit += 1;
			break;
		case 4:  // photosynthesis (end move)
			(*bot).photosynthesis();
			bot_turn = false;
			break;
		case 5:  // clone (end move)
			(*bot).give_birth();
			bot_turn = false;
			break;
		case 6:  // chemosynthesis (end move)
			(*bot).chemosynthesis();
			bot_turn = false;
			break;
		case 7:  // collect minerals (end move)
			(*bot).collect_mineral();
			bot_turn = false;
			break;
		case 8:  // minerals to energy (end move)
			(*bot).mine_to_energy();
			bot_turn = false;
			break;
		case 9:  // is energy enough?
			(*bot).how_much_energy();
			actions_limit += 1;
			break;
		case 10:  // am i surrounded?
			(*bot).surrounded();
			actions_limit += 1;
			break;
		case 11:  // share 1/4 of energy (end move)
			(*bot).share_energy();
			bot_turn = false;
			break;
		case 12:  // checks bot's height (y position)
			(*bot).check_height();
			actions_limit += 1;
			break;
		case 13:  // checks bot's minerals
			(*bot).how_much_minerals();
			actions_limit += 1;
			break;
		default:  // безусловный переход
			(*bot).jump();
			actions_limit += 1;
			break;
		}

		// inactivity penalty
		if (actions_limit > 14) {
			(*bot).energy -= 1;
		}

		// ordinary reproduction
		if ((*bot).clone) {
			(*bot).clone = false;
			clone_bot(bots[bot_num]);
		}

		// forced reproduction
		if (forced_reproduction) {
			if ((*bot).energy >= energy_limit) {
				clone_bot(bots[bot_num]);
			}
		}

		// increase bot_num age
		(*bot).age += 1;

		// death check
		if ((*bot).energy <= 0) {
			(*bot).reset();
			(*bot).state = DEAD;  // it becomes dead
			break;
		}
	}

	(*bot).was_calculated = true;

	// Second death check
	if ((*bot).energy <= 0) {
		(*bot).reset();
		(*bot).state = DEAD;  // it becomes dead
	}
}


int re_range(int old_max, int old_min, int new_max, int new_min, int value) {
	int old_range = (old_max - old_min);
	int new_range = (new_max - new_min);
	return ((value - old_min) * new_range) / old_range + new_min;  // return int
}


void world_clear() {
	for (int i = 0; i < world_size; i++)
	{
		(*bots[i]).reset();
	}
}


void population_start() {
	// Green
	int index0 = map_width / 2;
	int new_color[3] = { 0, 255, 0 };
	(*bots[index0]).CreateBot(map_width / 2, 0, new_color);
	for (int i = 0; i < 64; i++)
		(*bots[index0]).dna[i] = 4;
}


void swap_bots(int a_index, int b_index)
{
	// Swap coordinates
	swap((*bots[a_index]).x, (*bots[b_index]).x);
	swap((*bots[a_index]).y, (*bots[b_index]).y);

	// Swap bots
	swap(bots[a_index], bots[b_index]);
}


void dead_cells_fall() {
	for (int i = world_size - 1; i >= 0; i--)
	{
		if ((*bots[i]).state == DEAD)  // if dead cell
		{
			if ((*bots[i]).y + 1 < map_height)  // There is space
			{
				if ((*bots[i + map_width]).state == FREE)  // It cell under is free
				{
					swap_bots(i, i + map_width);
				}
			}
		}
	}
}


void repeat_selection() {
	cout << "Generation:" << generation << "\t Lasted: " << iteration << endl;
	iteration = 0;
	generation += 1;

	world_clear();  // Чистим карту
	population_start();  // restart
}


void all_bot_actions()
{
	for (int i = 0; i < world_size; i++)
	{
		if ((*bots[i]).state == BOT && !(*bots[i]).was_calculated) {  // If bot, and wasn't calculated yet
			bot_actions(i);
		}
	}
}


void event_handing() {
	if (event.type == sf::Event::Closed) game = false;

	if (event.type == sf::Event::KeyPressed)
	{
		// Получаем нажатую клавишу - выполняем соответствующее действие
		if (event.key.code == sf::Keyboard::Escape) game = false;
		if (event.key.code == sf::Keyboard::Space) paint = !paint;
		if (event.key.code == sf::Keyboard::RBracket) fps += 5;
		if (event.key.code == sf::Keyboard::LBracket) fps -= 5;
		if (event.key.code == sf::Keyboard::D)
			int debug = 1;
		if (event.key.code == sf::Keyboard::E) {
			if (draw_mode == Energy)
				draw_mode = Default;
			else
				draw_mode = Energy;
		}
		if (event.key.code == sf::Keyboard::M) {
			if (draw_mode == Minerals)
				draw_mode = Default;
			else
				draw_mode = Minerals;
		}
	}
}


void draw()
{
	// Отчистка экарана
	window.clear();

	// Отрисовка ботов и трупов
	for (int i = 0; i < world_size; i++) {
		if ((*bots[i]).state == BOT) {  // if bot
			int* c = (*bots[i]).get_mode_color();
			rectangle.setFillColor(sf::Color(c[0], c[1], c[2]));
			rectangle.setPosition((*bots[i]).x * length, (*bots[i]).y * length);
			window.draw(rectangle);
		}

		else if ((*bots[i]).state == DEAD) {  // if dead cell
			rectangle.setFillColor(sf::Color::White);
			rectangle.setPosition((*bots[i]).x * length, (*bots[i]).y * length);
			window.draw(rectangle);
		}
	}

	// Draw FPS
	float Framerate = 1.f / Clock.getElapsedTime().asSeconds();
	Clock.restart();

	// Create a text
	sf::Text text("FPS:" + to_string((int)Framerate), font);
	text.setFillColor(sf::Color(0, 160, 0, 220));
	text.setPosition(width - 120, height - 40);
	// Draw it
	window.draw(text);

	// Display Everything
	window.display();

	// Manage FPS
	window.setFramerateLimit(fps);
}


int main()
{
	// Graphics Settings
	font.loadFromFile("arial.ttf");
	rectangle.setSize(sf::Vector2f(length - 1, length - 1));
	// Bots Settings
	create_all_bots();
	// Random Settings
	srand(time(0));
	// Start simulation
	population_start();

	while ((game)) {
		///* ==========   events handing   ========== */
		while (window.pollEvent(event)) {
			event_handing();
		}

		// Bot actions
		all_bot_actions();

		// Gravity to dead bodies
		dead_cells_fall();

		// count amount of bots !and! make bot following calculating possible
		int game_object_bots = 0;
		for (int i = 0; i < world_size; i++) {
			if ((*bots[i]).state == BOT)
			{
				game_object_bots += 1;
				(*bots[i]).was_calculated = false;
			}
		}

		// Repeat selection
		if (game_object_bots == 0)
			repeat_selection();

		// Painting
		if (paint) {
			draw();
		}

		// Increase iterations
		iteration += 1;
		//cout << "Iteration:" << iteration << endl;
		//cout << "\t Was born:" << born << endl;
		// born = 0;

		/*if (iteration > 100000) {
			window.close();
			cout << "BOT size:" << sizeof(Bot) << endl;
			cout << "Time passed: " << Timer.getElapsedTime().asMilliseconds() << "\tIterations: " << iteration << endl;
			cout << "Alive bots now: " << game_object_bots << "\tWas born: " << born << endl;
			system("pause");
			game = false;
		}*/
	}

	// End of a programm
	window.close();
	return 0;
}