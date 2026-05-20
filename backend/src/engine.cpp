#include "engine.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <regex>
#include <sstream>

namespace fr {

namespace {
const std::vector<std::string> kClassOrder = {"peasants", "nobles", "committee", "bourgeoisie"};

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
  return value;
}

std::string canonicalRole(const std::string& role) {
  std::string r = lower(role);
  if (r.find("peasant") != std::string::npos) return "peasants";
  if (r.find("noble") != std::string::npos) return "nobles";
  if (r.find("committee") != std::string::npos) return "committee";
  if (r.find("bourgeois") != std::string::npos) return "bourgeoisie";
  return r;
}

bool contains(const std::string& text, const std::string& needle) {
  return lower(text).find(lower(needle)) != std::string::npos;
}

int parseNumberAfter(const std::string& text, size_t pos) {
  while (pos < text.size() && !std::isdigit(static_cast<unsigned char>(text[pos]))) pos++;
  int value = 0;
  while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
    value = value * 10 + (text[pos] - '0');
    pos++;
  }
  return value == 0 ? 1 : value;
}

int clampStat(const std::string& stat, int value) {
  if (stat == "food" || stat == "gold") return std::max(0, value);
  return std::max(0, std::min(30, value));
}

int target(const GameState& state, const std::string& key, int base) {
  auto it = state.goalDelta.find(key);
  return std::max(1, base + (it == state.goalDelta.end() ? 0 : it->second));
}
}  // namespace

std::string escapeJson(const std::string& value) {
  std::ostringstream out;
  for (char c : value) {
    switch (c) {
      case '\\': out << "\\\\"; break;
      case '"': out << "\\\""; break;
      case '\n': out << "\\n"; break;
      case '\r': break;
      case '\t': out << "\\t"; break;
      default: out << c; break;
    }
  }
  return out.str();
}

Engine::Engine() : cards_(loadCards()), classes_(loadClasses()) {
  for (int i = 0; i < static_cast<int>(cards_.size()); ++i) cardIndex_[cards_[i].id] = i;
  for (int i = 0; i < static_cast<int>(classes_.size()); ++i) classIndex_[classes_[i].id] = i;
}

const Card* Engine::cardById(const std::string& id) const {
  auto it = cardIndex_.find(id);
  return it == cardIndex_.end() ? nullptr : &cards_[it->second];
}

const ClassDef* Engine::classById(const std::string& id) const {
  auto it = classIndex_.find(id);
  return it == classIndex_.end() ? nullptr : &classes_[it->second];
}

GameState Engine::newGame(const std::vector<std::string>& humanClassIds, uint32_t seed, const Balance& balance) const {
  std::mt19937 rng(seed);
  GameState state;
  state.goalDelta = balance.goalDelta;
  std::set<std::string> humans(humanClassIds.begin(), humanClassIds.end());
  for (const auto& id : kClassOrder) {
    const ClassDef* def = classById(id);
    Player player;
    player.classId = def->id;
    player.name = def->name;
    player.stats = def->start;
    if (balance.classStartDelta.count(def->id)) {
      for (const auto& [stat, delta] : balance.classStartDelta.at(def->id)) addStat(player.stats, stat, delta);
    }
    player.human = humans.count(id) > 0;
    state.players.push_back(player);
  }
  for (const auto& card : cards_) {
    for (int i = 0; i < std::max(1, card.count); ++i) state.deck.push_back(card.id);
  }
  std::shuffle(state.deck.begin(), state.deck.end(), rng);
  fillMarket(state, rng);
  state.log.push_back("The Estates convene. Revolution begins.");
  return state;
}

void Engine::fillMarket(GameState& state, std::mt19937& rng) const {
  while (state.market.size() < 9 && (!state.deck.empty() || !state.discard.empty())) {
    if (state.deck.empty()) {
      state.deck.swap(state.discard);
      std::shuffle(state.deck.begin(), state.deck.end(), rng);
    }
    state.market.push_back(state.deck.back());
    state.deck.pop_back();
  }
}

void Engine::collectIncome(Player& player) const {
  if (!player.flags.count("communism")) player.stats.gold += player.stats.goldPerTurn;
  player.stats.food += player.stats.foodPerTurn;
}

bool Engine::canAfford(const Player& player, const std::map<std::string, int>& amounts, const Balance& balance, const std::string& cardId) const {
  for (const auto& [stat, raw] : amounts) {
    int amount = raw;
    if (stat == "gold" || stat == "food" || stat == "weaponry") amount = std::max(0, amount + (balance.costDelta.count(cardId) ? balance.costDelta.at(cardId) : 0));
    if (statValue(player.stats, stat) < amount) return false;
  }
  return true;
}

bool Engine::meetsRequirements(const Player& player, const std::map<std::string, int>& amounts) const {
  for (const auto& [stat, amount] : amounts) {
    if (statValue(player.stats, stat) < amount) return false;
  }
  return true;
}

void Engine::payCost(Player& player, const std::map<std::string, int>& amounts, const Balance& balance, const std::string& cardId) const {
  for (const auto& [stat, raw] : amounts) {
    int amount = raw;
    if (stat == "gold" || stat == "food" || stat == "weaponry") amount = std::max(0, amount + (balance.costDelta.count(cardId) ? balance.costDelta.at(cardId) : 0));
    addStat(player.stats, stat, -amount);
  }
}

std::vector<Move> Engine::legalMoves(const GameState& state, int playerIndex, const Balance& balance) const {
  if (state.finished) return {};
  Player player = state.players[playerIndex];
  collectIncome(player);
  std::vector<Move> moves = {Move{"gain_food", ""}, Move{"gain_gold", ""}};
  std::set<std::string> seen;
  for (const auto& id : state.market) {
    if (!seen.insert(id).second) continue;
    const Card* card = cardById(id);
    if (!card) continue;
    bool roleOk = false;
    for (const auto& role : card->roles) roleOk = roleOk || canonicalRole(role) == player.classId;
    if (!roleOk) continue;
    if (card->major && state.completedMajors.count(card->id)) continue;
    if (!canAfford(player, card->cost, balance, card->id) || !meetsRequirements(player, card->requirements)) continue;
    moves.push_back(Move{"play", id});
  }
  return moves;
}

bool Engine::applyMove(GameState& state, const Move& move, std::mt19937& rng, const Balance& balance) const {
  if (state.finished) return false;
  int playerIndex = state.active;
  Player& player = state.players[playerIndex];
  if (move.type == "gain_food") {
    collectIncome(player);
    player.stats.food += 1;
    state.log.push_back(player.name + " gathered food.");
  } else if (move.type == "gain_gold") {
    collectIncome(player);
    if (!player.flags.count("communism")) player.stats.gold += 1;
    state.log.push_back(player.name + " raised gold.");
  } else if (move.type == "play") {
    auto legal = legalMoves(state, playerIndex, balance);
    bool ok = std::any_of(legal.begin(), legal.end(), [&](const Move& m) { return m.type == "play" && m.cardId == move.cardId; });
    const Card* card = cardById(move.cardId);
    if (!ok || !card) return false;
    collectIncome(player);
    payCost(player, card->cost, balance, card->id);
    applyCard(state, playerIndex, *card, balance);
    auto it = std::find(state.market.begin(), state.market.end(), card->id);
    if (it != state.market.end()) state.market.erase(it);
    if (card->major) {
      state.completedMajors.insert(card->id);
      player.majorCards.insert(card->id);
    } else if (!card->tactique) {
      state.discard.push_back(card->id);
    }
    state.playedCards.push_back(CardPlay{card->id, player.classId});
    fillMarket(state, rng);
    state.log.push_back(player.name + " played " + card->name + ".");
  } else {
    return false;
  }
  int completed = goalsComplete(state, playerIndex);
  if (completed >= 2) {
    state.finished = true;
    state.winner = player.classId;
    state.log.push_back(player.name + " completed two objectives and won.");
    return true;
  }
  advanceTurn(state, rng);
  return true;
}

void Engine::advanceTurn(GameState& state, std::mt19937&) const {
  std::vector<int> order(state.players.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    const Stats& x = state.players[a].stats;
    const Stats& y = state.players[b].stats;
    if (state.civilWar && x.weaponry != y.weaponry) return x.weaponry > y.weaponry;
    if (x.assembly != y.assembly) return x.assembly > y.assembly;
    if (x.weaponry != y.weaponry) return x.weaponry > y.weaponry;
    return x.loyalty > y.loyalty;
  });
  auto it = std::find(order.begin(), order.end(), state.active);
  int pos = it == order.end() ? 0 : static_cast<int>(std::distance(order.begin(), it));
  if (pos + 1 < static_cast<int>(order.size())) {
    state.active = order[pos + 1];
  } else {
    endRound(state);
    state.active = order[0];
  }
}

void Engine::endRound(GameState& state) const {
  for (auto& player : state.players) {
    if (player.stats.food >= player.stats.foodRequired) {
      player.stats.food -= player.stats.foodRequired;
    } else {
      player.stats.food = 0;
      player.starvationCount++;
      if (player.stats.loyalty > 1) player.stats.loyalty--;
      else if (player.stats.assembly > 0) player.stats.assembly--;
    }
  }
  state.round++;
  state.yearTick++;
  if (state.yearTick >= 4) {
    awardAssembly(state);
    state.yearTick = 0;
  }
}

void Engine::awardAssembly(GameState& state) const {
  for (const std::string& stat : {"gold", "weaponry", "loyalty"}) {
    int best = -1;
    int count = 0;
    int winner = -1;
    for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
      int value = statValue(state.players[i].stats, stat);
      if (value > best) {
        best = value;
        winner = i;
        count = 1;
      } else if (value == best) {
        count++;
      }
    }
    if (winner >= 0 && count == 1) state.players[winner].stats.assembly++;
  }
}

void Engine::addStat(Stats& stats, const std::string& stat, int amount) const {
  if (stat == "food") stats.food = clampStat(stat, stats.food + amount);
  else if (stat == "gold") stats.gold = clampStat(stat, stats.gold + amount);
  else if (stat == "weaponry") stats.weaponry = clampStat(stat, stats.weaponry + amount);
  else if (stat == "loyalty") stats.loyalty = clampStat(stat, stats.loyalty + amount);
  else if (stat == "assembly") stats.assembly = clampStat(stat, stats.assembly + amount);
  else if (stat == "foodPerTurn") stats.foodPerTurn = std::max(0, stats.foodPerTurn + amount);
  else if (stat == "goldPerTurn") stats.goldPerTurn = std::max(0, stats.goldPerTurn + amount);
  else if (stat == "foodRequired") stats.foodRequired = std::max(1, stats.foodRequired + amount);
}

int Engine::statValue(const Stats& stats, const std::string& stat) const {
  if (stat == "food") return stats.food;
  if (stat == "gold") return stats.gold;
  if (stat == "weaponry") return stats.weaponry;
  if (stat == "loyalty") return stats.loyalty;
  if (stat == "assembly") return stats.assembly;
  if (stat == "foodPerTurn") return stats.foodPerTurn;
  if (stat == "goldPerTurn") return stats.goldPerTurn;
  return 0;
}

int Engine::strongestOpponent(const GameState& state, int playerIndex) const {
  int best = playerIndex == 0 ? 1 : 0;
  double bestScore = -1e9;
  for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
    if (i == playerIndex) continue;
    double score = evaluate(state, i);
    if (score > bestScore) {
      bestScore = score;
      best = i;
    }
  }
  return best;
}

void Engine::applyCard(GameState& state, int playerIndex, const Card& card, const Balance& balance) const {
  Player& player = state.players[playerIndex];
  std::string reward = card.reward;
  int rewardBoost = balance.rewardDelta.count(card.id) ? balance.rewardDelta.at(card.id) : 0;

  if (contains(card.id, "build_farm")) player.stats.foodPerTurn++;
  if (contains(card.id, "build_trading") || contains(card.id, "build_factory")) player.stats.goldPerTurn += contains(card.id, "factory") ? 2 : 1;
  if (card.id == "crop_rotation") player.stats.foodPerTurn *= 2;
  if (card.id == "bank_loan") player.stats.goldPerTurn = std::max(0, player.stats.goldPerTurn - 1);
  if (card.id == "communism") player.flags.insert("communism");
  if (card.id == "civil_war") state.civilWar = true;
  if (card.id == "enter_enlightenment") player.flags.insert("enlightenment");
  if (card.id == "enter_reign_of_terror") player.flags.insert("terror");
  if (card.id == "committee_of_general_security") player.flags.insert("security");
  if (card.id == "kill_enemy_of_revolution") player.enemiesKilled++;
  if (card.id == "guillotine_marie_antionette" && player.classId == "committee") player.enemiesKilled += 2;
  if (card.id == "guillotine_king_louis_xvi") player.flags.insert("king_dead");
  if (card.id == "abolish_feudalism") player.flags.insert("feudalism_abolished");

  std::string rewardLower = lower(reward);
  for (const auto& [label, stat] : std::vector<std::pair<std::string, std::string>>{
           {"food", "food"}, {"gold", "gold"}, {"weaponry", "weaponry"}, {"loyalty", "loyalty"}, {"assembly", "assembly"}}) {
    std::string token = label + " +";
    size_t pos = 0;
    while ((pos = rewardLower.find(token, pos)) != std::string::npos) {
      bool belongsToOther = false;
      size_t lineStart = rewardLower.rfind('.', pos);
      lineStart = lineStart == std::string::npos ? 0 : lineStart + 1;
      std::string prefix = rewardLower.substr(lineStart, pos - lineStart);
      for (const auto& other : {"nobles", "peasants", "committee", "bourgeoisie"}) {
        if (prefix.find(other) != std::string::npos) belongsToOther = true;
      }
      if (!belongsToOther) {
        int amount = parseNumberAfter(rewardLower, pos + token.size());
        if ((stat == "food" || stat == "gold" || stat == "weaponry") && rewardBoost != 0) amount += rewardBoost;
        addStat(player.stats, stat, amount);
      }
      pos += token.size();
    }
  }
  for (const auto& [token, stat] : std::vector<std::pair<std::string, std::string>>{{"food per turn +", "foodPerTurn"}, {"gold per turn +", "goldPerTurn"}}) {
    size_t pos = 0;
    while ((pos = rewardLower.find(token, pos)) != std::string::npos) {
      addStat(player.stats, stat, parseNumberAfter(rewardLower, pos + token.size()));
      pos += token.size();
    }
  }
  for (const auto& targetLabel : {"nobles", "peasants", "committee", "bourgeoisie"}) {
    for (const auto& [label, stat] : std::vector<std::pair<std::string, std::string>>{
             {"food", "food"}, {"gold", "gold"}, {"weaponry", "weaponry"}, {"loyalty", "loyalty"}, {"assembly", "assembly"}}) {
      for (const auto sign : {'-', '+'}) {
        std::string token = std::string(targetLabel) + " " + label + " " + sign;
        size_t pos = rewardLower.find(token);
        if (pos != std::string::npos) {
          int delta = parseNumberAfter(rewardLower, pos + token.size()) * (sign == '-' ? -1 : 1);
          std::string targetClass = canonicalRole(targetLabel);
          for (auto& target : state.players) {
            if (target.classId == targetClass) addStat(target.stats, stat, delta);
          }
        }
      }
    }
  }
  if (contains(reward, "National Freedom")) {
    int amount = 1;
    size_t nfPos = rewardLower.find("national freedom");
    if (nfPos != std::string::npos) amount = parseNumberAfter(rewardLower, nfPos);
    int direction = player.classId == "nobles" ? -1 : 1;
    if (contains(reward, "National Freedom -")) direction = -1;
    state.nationalFreedom = std::max(0, std::min(12, state.nationalFreedom + direction * amount));
  }
  if (contains(reward, "Lower loyalty and assembly") || contains(reward, "Lower assembly") || contains(reward, "Lower loyalty")) {
    int target = strongestOpponent(state, playerIndex);
    if (contains(reward, "loyalty")) addStat(state.players[target].stats, "loyalty", -1);
    if (contains(reward, "assembly")) addStat(state.players[target].stats, "assembly", -1);
  }
  if (contains(reward, "Steal up to 2 food")) {
    int target = strongestOpponent(state, playerIndex);
    int stolen = std::min(2, state.players[target].stats.food);
    state.players[target].stats.food -= stolen;
    player.stats.food += stolen;
  }
  if (contains(reward, "Target Class loses 2 weapons")) {
    int target = strongestOpponent(state, playerIndex);
    int loss = state.civilWar ? 4 : 2;
    state.players[target].stats.weaponry = std::max(0, state.players[target].stats.weaponry - loss);
    if (state.players[target].stats.weaponry == 0) addStat(state.players[target].stats, "loyalty", -1);
  }
}

bool Engine::hasAssemblyControl(const GameState& state, int playerIndex) const {
  int value = state.players[playerIndex].stats.assembly;
  for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
    if (i != playerIndex && state.players[i].stats.assembly >= value) return false;
  }
  return true;
}

bool Engine::hasMost(const GameState& state, int playerIndex, const std::string& stat) const {
  int value = statValue(state.players[playerIndex].stats, stat);
  for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
    if (i != playerIndex && statValue(state.players[i].stats, stat) >= value) return false;
  }
  return true;
}

int Engine::goalsComplete(const GameState& state, int playerIndex) const {
  const Player& p = state.players[playerIndex];
  int n = 0;
  if (p.classId == "peasants") {
    n += state.completedMajors.count("abolish_feudalism") || p.flags.count("feudalism_abolished");
    const Player* nobles = nullptr;
    for (const auto& x : state.players) if (x.classId == "nobles") nobles = &x;
    n += nobles && p.stats.assembly > nobles->stats.assembly;
    n += p.stats.food >= target(state, "peasants_food", 15);
  } else if (p.classId == "nobles") {
    n += hasAssemblyControl(state, playerIndex);
    n += state.nationalFreedom <= target(state, "nobles_national_freedom", 1);
    n += p.stats.gold >= target(state, "nobles_gold", 30) && p.stats.weaponry >= target(state, "nobles_weaponry", 10);
  } else if (p.classId == "committee") {
    n += p.flags.count("king_dead") || state.completedMajors.count("guillotine_king_louis_xvi");
    n += p.enemiesKilled >= target(state, "committee_enemies", 3);
    n += hasMost(state, playerIndex, "weaponry");
  } else if (p.classId == "bourgeoisie") {
    n += hasAssemblyControl(state, playerIndex);
    n += (p.flags.count("enlightenment") || state.completedMajors.count("enter_enlightenment")) && p.stats.loyalty >= target(state, "bourgeoisie_loyalty", 7);
    n += state.nationalFreedom >= target(state, "bourgeoisie_national_freedom", 9);
  }
  return n;
}

double Engine::goalProgress(const GameState& state, int playerIndex) const {
  const Player& p = state.players[playerIndex];
  double complete = static_cast<double>(goalsComplete(state, playerIndex));
  double partial = 0.0;
  if (p.classId == "peasants") {
    const Player* nobles = nullptr;
    for (const auto& x : state.players) if (x.classId == "nobles") nobles = &x;
    partial += (state.completedMajors.count("abolish_feudalism") || p.flags.count("feudalism_abolished")) ? 1.0 : (p.stats.weaponry >= 3 && p.stats.assembly >= 2 ? 0.45 : 0.15);
    if (nobles) partial += std::clamp(0.5 + (p.stats.assembly - nobles->stats.assembly) / 6.0, 0.0, 1.0);
    partial += std::clamp(static_cast<double>(p.stats.food) / target(state, "peasants_food", 15), 0.0, 1.0);
  } else if (p.classId == "nobles") {
    partial += hasAssemblyControl(state, playerIndex) ? 1.0 : std::clamp(0.45 + p.stats.assembly / 10.0, 0.0, 0.9);
    partial += std::clamp((5.0 - state.nationalFreedom) / std::max(1.0, 5.0 - target(state, "nobles_national_freedom", 1)), 0.0, 1.0);
    partial += std::min(std::clamp(static_cast<double>(p.stats.gold) / target(state, "nobles_gold", 30), 0.0, 1.0), std::clamp(static_cast<double>(p.stats.weaponry) / target(state, "nobles_weaponry", 10), 0.0, 1.0));
  } else if (p.classId == "committee") {
    partial += (p.flags.count("king_dead") || state.completedMajors.count("guillotine_king_louis_xvi")) ? 1.0 : std::clamp((p.stats.weaponry + p.stats.assembly) / 10.0, 0.0, 0.85);
    partial += std::clamp(static_cast<double>(p.enemiesKilled) / target(state, "committee_enemies", 3), 0.0, 1.0);
    partial += hasMost(state, playerIndex, "weaponry") ? 1.0 : std::clamp(0.4 + p.stats.weaponry / 12.0, 0.0, 0.9);
  } else if (p.classId == "bourgeoisie") {
    partial += hasAssemblyControl(state, playerIndex) ? 1.0 : std::clamp(0.35 + p.stats.assembly / 10.0, 0.0, 0.9);
    partial += std::min((p.flags.count("enlightenment") || state.completedMajors.count("enter_enlightenment")) ? 1.0 : 0.45, std::clamp(static_cast<double>(p.stats.loyalty) / target(state, "bourgeoisie_loyalty", 7), 0.0, 1.0));
    partial += std::clamp((state.nationalFreedom - 5.0) / std::max(1.0, target(state, "bourgeoisie_national_freedom", 9) - 5.0), 0.0, 1.0);
  }
  return complete + std::max(0.0, partial - complete);
}

double Engine::evaluate(const GameState& state, int playerIndex) const {
  const Player& p = state.players[playerIndex];
  if (state.finished) return state.winner == p.classId ? 100000.0 : -100000.0;
  double progress = goalProgress(state, playerIndex);
  double score = progress * 850.0 + goalsComplete(state, playerIndex) * 420.0;
  score += p.stats.food * 1.5 + p.stats.gold * 1.3 + p.stats.weaponry * 4.2 + p.stats.loyalty * 5.5 + p.stats.assembly * 7.0;
  score += p.stats.foodPerTurn * 13.0 + p.stats.goldPerTurn * 12.0;
  score -= p.starvationCount * 22.0;
  int foodSafety = p.stats.food + p.stats.foodPerTurn - p.stats.foodRequired;
  if (foodSafety < 0) score += foodSafety * 35.0;
  if (goalsComplete(state, playerIndex) == 1) score += 180.0 * std::max(0.0, progress - 1.0);
  if (p.classId == "peasants") {
    score += std::min(15, p.stats.food) * 14.0;
    for (const auto& x : state.players) if (x.classId == "nobles") score += (p.stats.assembly - x.stats.assembly) * 42.0;
    if (!state.completedMajors.count("abolish_feudalism")) score += (p.stats.weaponry >= 3 ? 45.0 : 0.0) + (p.stats.assembly >= 3 ? 45.0 : 0.0);
  } else if (p.classId == "nobles") {
    score += (12 - state.nationalFreedom) * 43.0 + p.stats.gold * 2.4 + p.stats.weaponry * 6.2;
    if (hasAssemblyControl(state, playerIndex)) score += 120.0;
  } else if (p.classId == "committee") {
    score += p.enemiesKilled * 155.0 + (p.flags.count("king_dead") ? 360.0 : 0.0) + p.stats.weaponry * 9.0;
  } else if (p.classId == "bourgeoisie") {
    score += state.nationalFreedom * 47.0 + p.stats.loyalty * 10.0 + (p.flags.count("enlightenment") ? 330.0 : 0.0);
  }
  for (int i = 0; i < static_cast<int>(state.players.size()); ++i) {
    if (i != playerIndex) {
      score -= goalsComplete(state, i) * 310.0;
      score -= std::max(0.0, goalProgress(state, i) - 1.35) * 95.0;
    }
  }
  return score;
}

std::vector<double> Engine::evaluateAll(const GameState& state) const {
  std::vector<double> values(state.players.size(), 0.0);
  for (int i = 0; i < static_cast<int>(state.players.size()); ++i) values[i] = evaluate(state, i);
  return values;
}

std::vector<double> Engine::search(GameState state, int depth, const Balance& balance, uint32_t seed) const {
  if (depth <= 0 || state.finished) return evaluateAll(state);
  std::mt19937 rng(seed + state.round * 31 + state.active * 997 + depth);
  auto moves = legalMoves(state, state.active, balance);
  if (moves.empty()) return evaluateAll(state);
  std::vector<std::pair<double, Move>> ranked;
  for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
    GameState next = state;
    applyMove(next, moves[i], rng, balance);
    ranked.push_back({evaluate(next, state.active), moves[i]});
  }
  std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
  size_t beam = depth >= 2 ? 2 : 3;
  if (ranked.size() > beam) ranked.resize(beam);
  std::vector<double> bestValues;
  double best = -1e18;
  for (const auto& [_, move] : ranked) {
    GameState next = state;
    applyMove(next, move, rng, balance);
    std::vector<double> values = search(next, depth - 1, balance, seed + 17);
    double activeValue = values[state.active];
    if (activeValue > best) {
      best = activeValue;
      bestValues = std::move(values);
    }
  }
  return bestValues.empty() ? evaluateAll(state) : bestValues;
}

Move Engine::chooseAiMove(const GameState& state, int playerIndex, int depth, const Balance& balance) const {
  auto moves = legalMoves(state, playerIndex, balance);
  if (moves.empty()) return Move{"gain_food", ""};
  std::vector<std::pair<double, Move>> ranked;
  for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
    GameState next = state;
    std::mt19937 rng(8000 + state.round * 31 + playerIndex * 97 + i);
    applyMove(next, moves[i], rng, balance);
    ranked.push_back({evaluate(next, playerIndex), moves[i]});
  }
  std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
  size_t beam = depth >= 5 ? 3 : 4;
  if (ranked.size() > beam) ranked.resize(beam);
  Move bestMove = moves.front();
  double best = -1e18;
  for (int i = 0; i < static_cast<int>(ranked.size()); ++i) {
    GameState next = state;
    std::mt19937 rng(9000 + state.round * 53 + playerIndex * 101 + i);
    applyMove(next, ranked[i].second, rng, balance);
    std::vector<double> values = search(next, depth - 1, balance, 7000 + i);
    double opponentThreat = -1e18;
    for (int p = 0; p < static_cast<int>(values.size()); ++p) {
      if (p != playerIndex) opponentThreat = std::max(opponentThreat, values[p]);
    }
    double value = values[playerIndex] - opponentThreat * 0.08;
    if (value > best) {
      best = value;
      bestMove = ranked[i].second;
    }
  }
  return bestMove;
}

GameState Engine::playGame(uint32_t seed, int depth, const Balance& balance) const {
  GameState state = newGame({}, seed, balance);
  std::mt19937 rng(seed);
  int guard = 120;
  while (!state.finished && guard-- > 0) {
    Move move = chooseAiMove(state, state.active, depth, balance);
    applyMove(state, move, rng, balance);
  }
  if (!state.finished) {
    int best = 0;
    for (int i = 1; i < static_cast<int>(state.players.size()); ++i) {
      if (evaluate(state, i) > evaluate(state, best)) best = i;
    }
    state.finished = true;
    state.winner = state.players[best].classId;
    state.log.push_back("Turn limit reached; highest objective pressure wins.");
  }
  return state;
}

SimulationSummary Engine::simulate(int games, int depth, const Balance& balance, uint32_t seed) const {
  SimulationSummary summary;
  summary.games = games;
  std::map<std::string, int> roundTotals;
  for (int i = 0; i < games; ++i) {
    GameState state = playGame(seed + i * 7919, depth, balance);
    summary.wins[state.winner]++;
    roundTotals[state.winner] += state.round;
    for (const auto& play : state.playedCards) {
      summary.cardPlays[play.cardId]++;
      summary.classCardPlays[play.classId][play.cardId]++;
      if (play.classId == state.winner) {
        summary.winnerCardPlays[play.cardId]++;
        summary.classWinnerCardPlays[play.classId][play.cardId]++;
      }
    }
  }
  for (const auto& klass : classes_) {
    int wins = summary.wins[klass.id];
    summary.avgRounds[klass.id] = wins ? static_cast<double>(roundTotals[klass.id]) / wins : 0.0;
  }
  return summary;
}

std::string Engine::serializeData() const {
  std::ostringstream out;
  out << "{\"classes\":[";
  for (size_t i = 0; i < classes_.size(); ++i) {
    const auto& c = classes_[i];
    if (i) out << ",";
    out << "{\"id\":\"" << c.id << "\",\"name\":\"" << c.name << "\",\"portrait\":\"" << escapeJson(c.portrait) << "\",\"goals\":[";
    for (size_t g = 0; g < c.goals.size(); ++g) {
      if (g) out << ",";
      out << "{\"id\":\"" << c.goals[g].id << "\",\"label\":\"" << escapeJson(c.goals[g].label) << "\"}";
    }
    out << "]}";
  }
  out << "],\"cards\":[";
  for (size_t i = 0; i < cards_.size(); ++i) {
    const auto& c = cards_[i];
    if (i) out << ",";
    out << "{\"id\":\"" << c.id << "\",\"name\":\"" << escapeJson(c.name) << "\",\"flavor\":\"" << escapeJson(c.flavor)
        << "\",\"reward\":\"" << escapeJson(c.reward) << "\",\"major\":" << (c.major ? "true" : "false")
        << ",\"tactique\":" << (c.tactique ? "true" : "false") << ",\"image\":\"" << escapeJson(c.image) << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string Engine::serializeState(const GameState& state, const Balance& balance) const {
  std::ostringstream out;
  out << "{\"round\":" << state.round << ",\"yearTick\":" << state.yearTick << ",\"active\":" << state.active
      << ",\"nationalFreedom\":" << state.nationalFreedom << ",\"finished\":" << (state.finished ? "true" : "false")
      << ",\"winner\":\"" << state.winner << "\",\"market\":[";
  for (size_t i = 0; i < state.market.size(); ++i) {
    if (i) out << ",";
    out << "\"" << state.market[i] << "\"";
  }
  out << "],\"players\":[";
  for (size_t i = 0; i < state.players.size(); ++i) {
    const auto& p = state.players[i];
    if (i) out << ",";
    out << "{\"classId\":\"" << p.classId << "\",\"name\":\"" << p.name << "\",\"human\":" << (p.human ? "true" : "false")
        << ",\"enemiesKilled\":" << p.enemiesKilled << ",\"goalsComplete\":" << goalsComplete(state, i)
        << ",\"stats\":{\"food\":" << p.stats.food << ",\"gold\":" << p.stats.gold << ",\"weaponry\":" << p.stats.weaponry
        << ",\"loyalty\":" << p.stats.loyalty << ",\"assembly\":" << p.stats.assembly << ",\"foodPerTurn\":" << p.stats.foodPerTurn
        << ",\"goldPerTurn\":" << p.stats.goldPerTurn << ",\"foodRequired\":" << p.stats.foodRequired << "},\"majors\":[";
    bool first = true;
    for (const auto& m : p.majorCards) {
      if (!first) out << ",";
      first = false;
      out << "\"" << m << "\"";
    }
    out << "]}";
  }
  out << "],\"legal\":[";
  auto moves = legalMoves(state, state.active, balance);
  for (size_t i = 0; i < moves.size(); ++i) {
    if (i) out << ",";
    out << "{\"type\":\"" << moves[i].type << "\",\"cardId\":\"" << moves[i].cardId << "\"}";
  }
  out << "],\"log\":[";
  size_t start = state.log.size() > 16 ? state.log.size() - 16 : 0;
  for (size_t i = start; i < state.log.size(); ++i) {
    if (i != start) out << ",";
    out << "\"" << escapeJson(state.log[i]) << "\"";
  }
  out << "]}";
  return out.str();
}

std::string Engine::serializeSummary(const SimulationSummary& summary, const Balance& balance) const {
  std::ostringstream out;
  out << "{\"games\":" << summary.games << ",\"wins\":{";
  bool first = true;
  for (const auto& klass : classes_) {
    if (!first) out << ",";
    first = false;
    int wins = summary.wins.count(klass.id) ? summary.wins.at(klass.id) : 0;
    double rate = summary.games ? static_cast<double>(wins) / summary.games : 0.0;
    out << "\"" << klass.id << "\":{\"count\":" << wins << ",\"rate\":" << rate << ",\"avgRounds\":" << summary.avgRounds.at(klass.id) << "}";
  }
  out << "},\"balance\":{\"costDelta\":{";
  first = true;
  for (const auto& [id, delta] : balance.costDelta) {
    if (!first) out << ",";
    first = false;
    out << "\"" << id << "\":" << delta;
  }
  out << "},\"rewardDelta\":{";
  first = true;
  for (const auto& [id, delta] : balance.rewardDelta) {
    if (!first) out << ",";
    first = false;
    out << "\"" << id << "\":" << delta;
  }
  out << "},\"classStartDelta\":{";
  first = true;
  for (const auto& [klass, stats] : balance.classStartDelta) {
    if (!first) out << ",";
    first = false;
    out << "\"" << klass << "\":{";
    bool firstStat = true;
    for (const auto& [stat, delta] : stats) {
      if (!firstStat) out << ",";
      firstStat = false;
      out << "\"" << stat << "\":" << delta;
    }
    out << "}";
  }
  out << "},\"goalDelta\":{";
  first = true;
  for (const auto& [goal, delta] : balance.goalDelta) {
    if (!first) out << ",";
    first = false;
    out << "\"" << goal << "\":" << delta;
  }
  out << "}}}";
  return out.str();
}

}  // namespace fr
