#pragma once

struct inside {
  bool is_inside;
};


struct programstate {
  bool has_reached_A;
  bool has_reached_B;
  struct inside inside_state;
};