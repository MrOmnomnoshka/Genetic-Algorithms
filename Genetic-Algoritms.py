import pygame
import random
import datetime
from enum import Enum
import os
import numpy as np
from numba import jit, int32


class Actions(Enum):
    catch = 1
    move = 2
    share = 3


class DrawMode(Enum):
    default = 1
    energy = 2
    minerals = 3


class Bot:
    counter = 0
    direction = 0
    energy = 35
    minerals = 0
    dna = []
    age = 0
    amount_of_children = 0

    def __init__(self, x, y, color):
        self.x = x
        self.y = y
        self.color = color

    # ///////////////////////////////////// (вспомогательные)
    def translate(self, where):
        # Вверх, Вверх-вправо, Вправо, Вниз-вправо, Вниз, Вниз-влево, Влево, Вверх-влево
        options = ((0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1))

        new_x = self.x + options[where][0]
        new_y = self.y + options[where][1]
        new_direction = where

        # Зацикливаем мир горизонтально
        if new_x >= map_width:
            new_x = 0
        elif new_x < 0:
            new_x = map_width - 1

        # Ограничиваем мир вертикально
        if new_y >= map_height:
            new_y = map_height - 1
        elif new_y < 0:
            new_y = 0
        # /////////////////////////////////////
        return new_x, new_y, new_direction

    def try_to_move(self, entity, x, y):
        if entity == 4:  # Если там пусто
            self.just_go(x, y)

        elif entity == 3:  # Если там труп
            self.just_go(x, y)
            self.add_red()
            self.add_energy(10)

        elif entity == 2:  # Another bot
            interactions.append((self, (x, y), Actions.move))

    def try_to_catch(self, entity, x, y):
        if entity == 3:  # Если там труп
            self.add_red()
            self.add_energy(10)

        elif entity == 2:  # Another bot
            interactions.append((self, (x, y), Actions.catch))

    def just_go(self, x, y):
        world[self.y][self.x] = 4
        self.x, self.y = x, y
        world[y][x] = 2

    def get_next_dna(self, dist):
        return (self.counter + dist) % 64

    def add_energy(self, en):
        self.energy = min(energy_limit, self.energy + en + self.minerals)

    def add_minerals(self, mineral):
        self.minerals = min(minerals_limit, self.minerals + mineral)

    def increase_counter(self, value):
        self.counter = (self.counter + value) % 64

    def standard_actions(self):  # Повторялось много раз, сделал функцию
        next_dna = self.get_next_dna(1)  # Check what to do
        where = (self.dna[next_dna] + self.direction) % 8  # Check where to do
        x, y, new_direction = self.translate(where)  # Get new world X, Y and direction
        entity = world[y][x]  # Get an entity that was there
        next_dna = self.get_next_dna(entity)  # Depending on what was there...
        self.increase_counter(self.dna[next_dna])  # ...move counter along
        return entity, x, y

    def add_red(self):
        self.color[0] = min(255, self.color[0] + 5)  # red
        self.color[1] = max(0, self.color[1] - 1)  # green
        self.color[2] = max(0, self.color[2] - 1)  # blue

    def add_green(self):
        self.color[1] = min(255, self.color[1] + 5)  # green
        self.color[0] = max(0, self.color[0] - 1)  # red
        self.color[2] = max(0, self.color[2] - 1)  # blue

    def add_blue(self):
        self.color[2] = min(255, self.color[2] + 5)  # blue
        self.color[1] = max(0, self.color[1] - 1)  # green
        self.color[0] = max(0, self.color[0] - 1)  # red

    def get_mode_color(self):
        if draw_mode == DrawMode.energy:
            color = [255, 255 - self.energy, 0]
        elif draw_mode == DrawMode.minerals:
            color = [0, 255 - self.minerals * 5, 255]
        else:
            color = self.color
        return color

    def find_free_space(self):
        free_space = list()
        for sur in range(8):
            x, y, _ = self.translate(sur)
            if world[y][x] == 4:
                free_space.append((x, y))
        return free_space
    # ///////////////////////////////////// (вспомогательные)
    # 2 - бот, 3 - труп, 4 - пусто.

    # 0
    def move(self):
        self.energy -= 1 + self.age // age_limit  # Decrease energy
        entity, x, y = self.standard_actions()
        self.try_to_move(entity, x, y)  # Try to do some action

    # 1
    def catch(self):
        self.energy -= 1 + self.age // age_limit
        entity, x, y = self.standard_actions()
        self.try_to_catch(entity, x, y)

    # 2
    def look(self):
        self.standard_actions()  # Just to move counter

    # 3
    def turn(self):  # 16-23
        next_dna = self.get_next_dna(1)
        where = (self.dna[next_dna] + self.direction) % 8
        _, _, new_direction = self.translate(where)

        self.direction = new_direction
        self.increase_counter(2)

    # 4
    def photosynthesis(self):
        before = self.energy
        self.energy -= 1 + self.age // age_limit
        if self.y < map_height // 2:
            energy_to_add = re_range(0, map_height // 2, 20, 0, self.y)
            self.add_energy(energy_to_add)
        if self.energy - before > 30:  # if get energy
            self.add_green()
        self.increase_counter(1)

    # 5
    def give_birth(self):
        self.energy -= 1 + self.age // age_limit
        clone_bot(self)
        self.increase_counter(1)

    # 6
    def chemosynthesis(self):
        before = self.energy
        self.energy -= 1 + self.age // age_limit
        energy_to_add = re_range(0, map_height, 0, 10, self.y)
        self.add_energy(energy_to_add)
        if self.energy - before > 30:  # if get energy
            self.add_blue()
        self.increase_counter(1)

    # 7
    def collect_mineral(self):
        self.energy -= 1 + self.age // age_limit
        if self.y > map_height // 2:
            minerals_to_add = re_range(map_height // 2, map_height, 0, 15, self.y)
            self.add_minerals(minerals_to_add)
            self.add_blue()
        self.increase_counter(1)

    # 8
    def mine_to_energy(self):
        self.energy -= 1 + self.age // age_limit
        needed_energy = energy_limit - self.energy
        minerals_to_convert = self.minerals - needed_energy // 5
        if minerals_to_convert < 0:
            self.add_energy(self.minerals * 5)
            self.minerals = 0
        else:
            self.add_energy(minerals_to_convert * 5)
            self.minerals -= minerals_to_convert

    # 9
    def how_much_energy(self):
        next_dna = self.get_next_dna(1)
        energy_needed = re_range(64, 0, energy_limit, 0, self.dna[next_dna])

        if self.energy < energy_needed:
            next_dna = self.get_next_dna(2)
            self.increase_counter(self.dna[next_dna])
        else:
            next_dna = self.get_next_dna(3)
            self.increase_counter(self.dna[next_dna])

    # 10
    def surrounded(self):
        free_space = self.find_free_space()
        if free_space:
            next_dna = self.get_next_dna(1)
            self.increase_counter(self.dna[next_dna])
        else:
            next_dna = self.get_next_dna(2)
            self.increase_counter(self.dna[next_dna])

    # 11
    def share_energy(self):
        self.energy -= 1 + self.age // age_limit
        next_dna = self.get_next_dna(1)
        where = (self.dna[next_dna] + self.direction) % 8
        x, y, _ = self.translate(where)
        if world[y][x] == 2:
            interactions.append((self, (x, y), Actions.share))
        self.increase_counter(2)

    # 12
    def check_height(self):
        next_dna = self.get_next_dna(1)
        required_height = re_range(64, 0, map_height, 0, self.dna[next_dna])

        if self.y >= required_height:
            next_dna = self.get_next_dna(2)
            self.increase_counter(self.dna[next_dna])
        else:
            next_dna = self.get_next_dna(3)
            self.increase_counter(self.dna[next_dna])

    # 13
    def how_much_minerals(self):
        next_dna = self.get_next_dna(1)
        minerals_needed = re_range(64, 0, minerals_limit, 0, self.dna[next_dna])

        if self.minerals < minerals_needed:
            next_dna = self.get_next_dna(2)
            self.increase_counter(self.dna[next_dna])
        else:
            next_dna = self.get_next_dna(3)
            self.increase_counter(self.dna[next_dna])

    # ... - 63
    def jump(self):  # увеличить указатель комманд
        self.counter = (self.counter + self.dna[self.counter]) % 64


def bot_actions(bot_num):
    global bots

    actions_limit = 0
    bot_turn = True
    current_bot = bots[bot_num]

    while actions_limit < 15 and bot_turn and current_bot.energy > 0:
        code = current_bot.dna[current_bot.counter]

        # Движение (заверщающий)
        if code == 0:
            current_bot.move()
            bot_turn = False

        # Схватить (заверщающий)
        elif code == 1:
            current_bot.catch()
            bot_turn = False

        # Посмотреть
        elif code == 2:
            current_bot.look()
            actions_limit += 1

        # Повернуться
        elif code == 3:
            current_bot.turn()
            actions_limit += 1

        # Photosynthesis (end move)
        elif code == 4:
            current_bot.photosynthesis()
            bot_turn = False

        # Clone (end move)
        elif code == 5:
            current_bot.give_birth()
            bot_turn = False

        # Chemosynthesis (end move)
        elif code == 6:
            current_bot.chemosynthesis()
            bot_turn = False

        # Collect Minerals (end move)
        elif code == 7:
            current_bot.collect_mineral()
            bot_turn = False

        # Minerals to energy (end move)
        elif code == 8:
            current_bot.mine_to_energy()
            bot_turn = False

        # Is energy enough?
        elif code == 9:
            current_bot.how_much_energy()
            actions_limit += 1

        # Am I surrounded?
        elif code == 10:
            current_bot.surrounded()
            actions_limit += 1

        # Share 1/4 of energy (end move)
        elif code == 11:
            current_bot.share_energy()
            bot_turn = False

        # Checks bot's height (y position)
        elif code == 12:
            current_bot.check_height()
            actions_limit += 1

        # Checks bot's minerals
        elif code == 13:
            current_bot.how_much_minerals()
            actions_limit += 1

        # Безусловный переход
        else:
            current_bot.jump()
            actions_limit += 1

        # inactivity penalty
        if actions_limit > 14:
            current_bot.energy -= 1

        # Forced reproduction
        if forced_reproduction:
            if current_bot.energy >= energy_limit:
                clone_bot(current_bot)

    # Increase bot_num age
    current_bot.age += 1

    # Death check
    if current_bot.energy <= 0:
        world[current_bot.y][current_bot.x] = 3  # It becomes dead
        bots.pop(bot_num)


def create_bot():
    # global world, bots
    x = random.randint(0, map_width - 1)  # x - для удобства
    y = random.randint(0, map_height - 1)  # y - для удобства
    if world[y][x] == 4:  # Если пусто
        world[y][x] = 2
        bots.append(Bot(x, y, [0, 0, 255]))
    else:
        create_bot()


def create_map():
    # 2 - бот, 3 - труп, 4 - пусто.
    return np.full((map_height, map_width), 4).tolist()


def clone_bot(bot):
    parent = bot
    # parent.energy -= 35  # Стоимость попытки "родов"
    free_space = parent.find_free_space()
    if free_space and parent.energy > 35:
        parent.energy -= 35
        parent.amount_of_children += 1

        baby_x, baby_y = random.choice(free_space)
        baby_color = parent.color.copy()
        child = Bot(baby_x, baby_y, baby_color)
        bots.append(child)
        world[baby_y][baby_x] = 2

        child.dna = parent.dna.copy()
        if parent.amount_of_children // random.randint(1, baby_mutation_index):
            mutate_bot(child)

    elif birth_to_die:
        bot.energy = 0


def mutate_bot(bot):
    for i in range(random.randrange(max_mut)):
        bot.dna[random.randrange(64)] = random.randrange(64)


def world_clear(array):
    return np.full_like(array, 4).tolist()


def population_start():
    # Green
    bots.append(Bot(map_width // 2, 0, [0, 255, 0]))
    world[0][map_width // 2] = 2
    bots[0].dna = [4, 4, 4, 4, 4, 4, 5]  # photo and birth

    # Blue
    bots.append(Bot(map_width // 4, map_height-1, [0, 0, 255]))
    world[map_height-1][map_width // 4] = 2
    bots[1].dna = [6, 6, 6, 6, 6, 6, 5]  # chemo and birth

    # Add rest dna
    for bot in range(2):
        for i in range(64 - len(bots[bot].dna)):
            bots[bot].dna.append(random.randrange(64))


def re_range(old_max, old_min, new_max, new_min, value):
    """ Re-maps a number from one range to another. """
    old_range = (old_max - old_min)
    new_range = (new_max - new_min)
    return (((value - old_min) * new_range) // old_range) + new_min  # returns int


def dead_cells_fall():
    for i in reversed(range(map_height)):
        for j in reversed(range(map_width)):
            if world[i][j] == 3:  # if dead cell
                if i + 1 < map_height:
                    if world[i + 1][j] == 4:
                        world[i][j] = 4
                        world[i+1][j] = 3


def get_position_of_all_bots():
    coordinates = []
    for bot in bots:
        coordinates.append(bot.x)
        coordinates.append(bot.y)
    return coordinates


@jit(int32(int32[:], int32, int32))  # JIT ускрояет функцию во много раз
def find_bot_number_by_coordinates(coordinates, x, y):
    for i in range(0, len(coordinates) // 2):
        if (x, y) == (coordinates[i*2], coordinates[i*2 + 1]):
            return i
    return -1


def calculate_interactions():
    coordinates = np.array(get_position_of_all_bots())

    for interaction in interactions:  # (activator_bot, (x, y), Action)
        activator_bot, (x, y), action = interaction

        bot_index = find_bot_number_by_coordinates(coordinates, x, y)

        if bot_index != -1:
            victim = bots[bot_index]

            # Attack
            if action == Actions.catch:
                if activator_bot.energy > victim.energy > 0:
                    activator_bot.add_red()
                    activator_bot.add_energy(victim.energy // 4)
                    victim.energy = 0
                    world[y][x] = 4

            # Step on
            if action == Actions.move:
                if activator_bot.energy > victim.energy > 0:
                    activator_bot.add_red()
                    activator_bot.add_energy(victim.energy // 4)
                    victim.energy = 0
                    activator_bot.just_go(x, y)

            # Share
            if action == Actions.share:
                quarter = activator_bot.energy // 4
                activator_bot.energy -= quarter
                victim.add_energy(quarter)
    interactions.clear()


def draw():
    global fps
    screen.fill(background)
    # Отрисовка карты
    for i in range(map_height):
        for j in range(map_width):
            if world[i][j] == 3:  # if dead cell
                pygame.draw.rect(screen, gray, [j * length, i * length, length, length])

    for bot in bots:
        color = bot.get_mode_color()
        pygame.draw.rect(screen, color, [bot.x * length, bot.y * length, length, length])
    # //////////////////////////////////

    # Make screenshot of world
    if recording:
        pygame.image.save(screen, folder + "/screen_" + str(iteration) + ".png")

        if record_all_modes:
            global draw_mode

            # Energy mode screen
            draw_mode = DrawMode.energy
            for bot in bots:
                color = bot.get_mode_color()
                pygame.draw.rect(screen, color, [bot.x * length, bot.y * length, length, length])
            pygame.image.save(screen, folder + "/energy_" + str(iteration) + ".png")

            # Minerals mode screen
            draw_mode = DrawMode.minerals
            for bot in bots:
                color = bot.get_mode_color()
                pygame.draw.rect(screen, color, [bot.x * length, bot.y * length, length, length])
            pygame.image.save(screen, folder + "/minerals_" + str(iteration) + ".png")

            # Default mode screen
            draw_mode = DrawMode.default
            for bot in bots:
                color = bot.get_mode_color()
                pygame.draw.rect(screen, color, [bot.x * length, bot.y * length, length, length])
            pygame.image.save(screen, folder + "/default_" + str(iteration) + ".png")

    if paint:
        # Prints FPS
        clock.tick(fps)
        fps_text = font.render("fps: " + str(int(clock.get_fps())), 0, (120, 0, 120))
        fps_text.set_alpha(200)
        screen.blit(fps_text, (width - 120, height - 35))
        # //////////////////////////////////
        pygame.display.flip()


def event_handling():
    """ ==========   обработка событий   ========== """
    global paint, fps, draw_mode, max_mut, game, pause
    for event in pygame.event.get():  # Medium resource intensive!
        if event.type == pygame.QUIT:
            game = False

        keys = pygame.key.get_pressed()  # Any key that was pressed

        if keys[pygame.K_SPACE]:
            paint = not paint
        if keys[pygame.K_ESCAPE]:
            game = False
        if keys[pygame.K_RIGHTBRACKET]:
            fps += 10
        elif keys[pygame.K_LEFTBRACKET]:
            if fps > 0:
                fps -= 10

        if keys[pygame.K_e]:
            if draw_mode == DrawMode.energy:
                draw_mode = DrawMode.default
            else:
                draw_mode = DrawMode.energy
        if keys[pygame.K_m]:
            if draw_mode == DrawMode.minerals:
                draw_mode = DrawMode.default
            else:
                draw_mode = DrawMode.minerals
        if keys[pygame.K_p]:
            pause = True
            while pause:
                pygame.event.clear()
                event = pygame.event.wait()
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_p:
                        pause = False

        if keys[pygame.K_EQUALS]:
            if max_mut < 40:
                max_mut += 1
                print(max_mut)
        elif keys[pygame.K_MINUS]:
            if max_mut > 1:
                max_mut -= 1
                print(max_mut)


def repeat_selection():
    global generation, iteration, folder, world

    print("Поколение", generation, "Задержалось на -", iteration)
    iteration = 0
    generation += 1

    if recording:
        folder = str(datetime.datetime.today().strftime("%Y-%m-%d-%H.%M.%S"))  # 2020-01-17-03.58.27
        os.mkdir(folder)

    world = world_clear(world)  # Чистим карту
    interactions.clear()
    population_start()  # restart


# ЦВЕТА
background = (0, 0, 0)  # фон
gray = (168, 168, 168)  # Dead cell
# //////////////////////////////////

# Параметры мира (можно менять)
size = width, height = 1280, 720  # 1280, 720
length = 20
fps = 60
paint = True

recording = False
record_all_modes = False  # Energy, Minerals, Default
# /////////////////////////////////////

# Bots settings (changeable)
baby_mutation_index = 2  # every randrange(N) bot's child mutates
max_mut = 5  # Максимальное кол-во мутаций в боте
energy_limit = 255
minerals_limit = 51
age_limit = 50  # energy -= age / age_limit
birth_to_die = False  # die if there is no space to birth
forced_reproduction = True  # if Energy is max, than give birth
# /////////////////////////////////////

# Параметры мира (не трогать)
folder = str(datetime.datetime.today().strftime("%Y-%m-%d-%H.%M.%S"))  # 2020-01-17-03.58.27
iteration = 0
generation = 0

map_height = height // length
map_width = width // length

world = []
bots = []
interactions = []
draw_mode = DrawMode.default

pygame.init()
pygame.display.set_caption("New World")
screen = pygame.display.set_mode(size)
clock = pygame.time.Clock()
font = pygame.font.SysFont("Arial", 35)
game = True
pause = False
# /////////////////////////////////////


def main():
    global bots, world, iteration  # to Increase speed
    world = create_map()
    population_start()

    if recording:
        os.mkdir(folder)

    # ------------------ MAIN CYCLE ----------------------
    while game:
        # обработка событий
        event_handling()

        # Bot actions
        for bot_num in reversed(range(len(bots))):
            bot_actions(bot_num)

        # Calculate all interactions between bots
        calculate_interactions()

        # Gravity to dead bodies
        dead_cells_fall()

        # Repeat selection
        if len(bots) == 0:
            repeat_selection()

        # Painting
        if paint or recording:
            draw()

        # Increase iterations
        iteration += 1
    # ----------------------------------------------------
    pygame.quit()


if __name__ == '__main__':
    main()
