#pragma once

#include "data.hpp"

#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace fr {

struct Balance {
  std::map<std::string, int> costDelta;
  std::map<std::string, int> rewardDelta;
  std::map<std::string, std::map<std::string, int>> classStartDelta;
  std::map<std::string, int> goalDelta;
};

struct Player {
  std::string classId;
  std::string name;
  Stats stats;
  bool human = false;
  int enemiesKilled = 0;
  int starvationCount = 0;
  std::set<std::string> majorCards;
  std::set<std::string> flags;
};

struct CardPlay {
  std::string cardId;
  std::string classId;
};

struct GameState {
  int round = 1;
  int yearTick = 0;
  int active = 0;
  int nationalFreedom = 5;
  bool civilWar = false;
  bool finished = false;
  std::string winner;
  std::vector<Player> players;
  std::vector<std::string> deck;
  std::vector<std::string> discard;
  std::vector<std::string> market;
  std::set<std::string> completedMajors;
  std::vector<CardPlay> playedCards;
  std::map<std::string, int> goalDelta;
  std::vector<std::string> log;
};

struct Move {
  std::string type;
  std::string cardId;
};

struct SimulationSummary {
  std::map<std::string, int> wins;
  std::map<std::string, double> avgRounds;
  std::map<std::string, int> cardPlays;
  std::map<std::string, int> winnerCardPlays;
  std::map<std::string, std::map<std::string, int>> classCardPlays;
  std::map<std::string, std::map<std::string, int>> classWinnerCardPlays;
  int games = 0;
};

class Engine {
 public:
  Engine();

  const std::vector<Card>& cards() const { return cards_; }
  const std::vector<ClassDef>& classes() const { return classes_; }
  const Card* cardById(const std::string& id) const;
  const ClassDef* classById(const std::string& id) const;

  GameState newGame(const std::vector<std::string>& humanClassIds, uint32_t seed, const Balance& balance = {}) const;
  std::vector<Move> legalMoves(const GameState& state, int playerIndex, const Balance& balance = {}) const;
  bool applyMove(GameState& state, const Move& move, std::mt19937& rng, const Balance& balance = {}) const;
  Move chooseAiMove(const GameState& state, int playerIndex, int depth, const Balance& balance = {}) const;
  GameState playGame(uint32_t seed, int depth, const Balance& balance = {}) const;
  SimulationSummary simulate(int games, int depth, const Balance& balance = {}, uint32_t seed = 1789) const;

  std::string serializeData() const;
  std::string serializeState(const GameState& state, const Balance& balance = {}) const;
  std::string serializeSummary(const SimulationSummary& summary, const Balance& balance) const;

 private:
  std::vector<Card> cards_;
  std::vector<ClassDef> classes_;
  std::map<std::string, int> cardIndex_;
  std::map<std::string, int> classIndex_;

  void fillMarket(GameState& state, std::mt19937& rng) const;
  void advanceTurn(GameState& state, std::mt19937& rng) const;
  void endRound(GameState& state) const;
  void awardAssembly(GameState& state) const;
  void collectIncome(Player& player) const;
  void applyCard(GameState& state, int playerIndex, const Card& card, const Balance& balance) const;
  bool canAfford(const Player& player, const std::map<std::string, int>& amounts, const Balance& balance, const std::string& cardId) const;
  bool meetsRequirements(const Player& player, const std::map<std::string, int>& amounts) const;
  void payCost(Player& player, const std::map<std::string, int>& amounts, const Balance& balance, const std::string& cardId) const;
  void addStat(Stats& stats, const std::string& stat, int amount) const;
  int statValue(const Stats& stats, const std::string& stat) const;
  int goalsComplete(const GameState& state, int playerIndex) const;
  bool hasAssemblyControl(const GameState& state, int playerIndex) const;
  bool hasMost(const GameState& state, int playerIndex, const std::string& stat) const;
  int strongestOpponent(const GameState& state, int playerIndex) const;
  double goalProgress(const GameState& state, int playerIndex) const;
  double evaluate(const GameState& state, int playerIndex) const;
  std::vector<double> evaluateAll(const GameState& state) const;
  std::vector<double> search(GameState state, int depth, const Balance& balance, uint32_t seed) const;
};

std::string escapeJson(const std::string& value);

}  // namespace fr
