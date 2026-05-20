#pragma once

#include <map>
#include <string>
#include <vector>

namespace fr {

struct Stats {
  int food = 0;
  int gold = 0;
  int weaponry = 0;
  int loyalty = 0;
  int assembly = 0;
  int foodPerTurn = 0;
  int goldPerTurn = 0;
  int foodRequired = 0;
};

struct Goal {
  std::string id;
  std::string label;
};

struct ClassDef {
  std::string id;
  std::string name;
  Stats start;
  std::vector<Goal> goals;
  std::string portrait;
};

struct Card {
  std::string id;
  std::string name;
  std::string flavor;
  std::map<std::string, int> cost;
  std::map<std::string, int> requirements;
  std::string reward;
  std::vector<std::string> roles;
  bool major = false;
  bool tactique = false;
  int count = 1;
  std::string image;
};

std::vector<Card> loadCards();
std::vector<ClassDef> loadClasses();

}  // namespace fr
