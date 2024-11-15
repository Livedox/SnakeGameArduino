#include <Arduino.h>

struct Coords {
  uint8_t x;
  uint8_t y;
};

struct IntCoords {
  int8_t x;
  int8_t y;
};

IntCoords get_from_joystick() {
  int x = analogRead(2);
  int y = analogRead(3);
  if (x < 250) {
    x = -1;
  } else if (x > 900) {
    x = 1;
  } else {
    x = 0;
  };
  if (y < 250) {
    y = -1;
  } else if (y > 900) {
    y = 1;
  } else {
    y = 0;
  };
  struct IntCoords c = {(int8_t)x, (int8_t)y};
  return c;
}

uint8_t field[8] = { };

uint8_t snake_array[64] = {};
uint8_t snake_array_left = 0;
uint8_t snake_array_right = 0;
uint8_t snake_len = 0;
struct Coords head = {0, 0};
struct IntCoords dir = {1, 0};
struct IntCoords prev_dir = {0, 0};
uint8_t apple = 14;
uint8_t game_state = 0;

Coords compressed_coords_to_coords(uint8_t c_coords) {
  Coords coords = { };
  coords.x = c_coords&0b111;
  coords.y = c_coords >> 3;
  return coords;
}

uint8_t coords_to_compressed_coords(Coords coords) {
  uint8_t cdc = 0;
  cdc |= coords.x;
  cdc |= coords.y << 3;
  return cdc;
}

void field_set(uint8_t xy) {
  Coords coords = compressed_coords_to_coords(xy);
  field[coords.y] |= 1 << coords.x;
}

void field_remove(uint8_t xy) {
  Coords coords = compressed_coords_to_coords(xy);
  field[coords.y] &= ~(1 << coords.x);
}

void snake_insert(uint8_t xy) {
  snake_array[snake_array_right] = xy;
  snake_array_right++;
  if (snake_array_right == 64) {
    snake_array_right = 0;
  }
  snake_len++;
}

uint8_t snake_remove() {
  uint8_t prev = snake_array[snake_array_left];
  snake_len--;
  snake_array_left++;
  if (snake_array_left == 64) {
    snake_array_left = 0;
  }
  return prev;
}

bool check_snake(uint8_t xy) {
  uint8_t i = snake_array_left;
  while (i != snake_array_right) {
    if (snake_array[i] == xy) {
      return true;
    };
    i++;
    if (i == 64) {i = 0;}
  }
  return false;
}

void init_game() {
  memset(field, 0, 8);
  snake_array[0] = 0b010000;
  field_set(0b010000);
  snake_array[1] = 0b010001;
  field_set(0b010001);
  snake_array[2] = 0b010010;
  field_set(0b010010);
  snake_len = 3;
  head.x = 2;
  head.y = 2;

  apple = random(3, 64);
  field_set(apple);

  game_state = 3;

  snake_array_left = 0;
  snake_array_right = 3;
}

void loss() {
  game_state = 1;
}

void win() {
  game_state = 2;
}

void tick() {
  struct IntCoords j = get_from_joystick();
  if (j.x != 0 || j.y != 0) {
    dir = j;
  };
  if (dir.x > 0) {
    head.x++;
  } else if (dir.x < 0) {
    head.x--;
  } else if (dir.y > 0) {
    head.y++;
  } else if (dir.y < 0) {
    head.y--;
  };

  if (head.x > 7 || head.y > 7) {
    loss();
    return;
  }
  auto xy = coords_to_compressed_coords(head);
  if (check_snake(xy)) {
    loss();
    return;
  }
  snake_insert(xy);
  field_set(xy);
  if (xy != apple) {
    uint8_t removed = snake_remove();
    field_remove(removed);
  } else {
    do {
      apple = random(64);
    } while (check_snake(apple));
    field_set(apple);
  }

  if (snake_len > 40) {
    win();
    return;
  }
}

const uint8_t colums[] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t rows[] = {8, 9, 10, 11, 12, 13, A0, A1};

const uint8_t trophy[8] = {
  0b01100110,
  0b11111111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000,
  0b00011000,
  0b00111100,
};

const uint8_t arrow[8] = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b11011011,
  0b10011001,
  0b00011000,
  0b00011000,
  0b00011000,
};

const uint8_t skull[8] = {
  0b00000000,
  0b01100110,
  0b01100110,
  0b01100110,
  0b00100100,
  0b00011000,
  0b00100100,
  0b00100100,
};

void displayLedPattern(const uint8_t * pixels){
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(colums[i], HIGH);
    for(uint8_t j = 0; j < 8; j++) {
      uint8_t pixel = pixels[j];
      digitalWrite(rows[j], !((pixel >> i) & 1));
      digitalWrite(rows[j], HIGH);
    }
    digitalWrite(colums[i], LOW);
  }
}

void setup() {
  for (int i = 0; i < 8; i++) {
    pinMode(colums[i], OUTPUT);
    pinMode(rows[i], OUTPUT);
  }
  randomSeed(analogRead(5));
}

uint32_t tmr1 = 0;
void loop() {
  if (game_state == 3) {
    if (millis() - tmr1 >= 800) {
      tmr1 = millis();
      tick();
    }
    // displayLedPattern((uint8_t*)trophy);
    displayLedPattern(field);
  } else {
    if (game_state == 0) {
      displayLedPattern((uint8_t*)arrow);
    } else if (game_state == 1) {
      displayLedPattern((uint8_t*)skull);
    } else {
      displayLedPattern((uint8_t*)trophy);
    }
    auto j = get_from_joystick();
    if (j.y < 0) {
      init_game();
    }
  }
}
