#include "engine.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <map>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace {
double rate(const fr::SimulationSummary& s, const std::string& id) {
  auto it = s.wins.find(id);
  return s.games ? static_cast<double>(it == s.wins.end() ? 0 : it->second) / s.games : 0.0;
}

bool inTargetBand(const fr::SimulationSummary& summary, const fr::Engine& engine) {
  for (const auto& klass : engine.classes()) {
    double r = rate(summary, klass.id);
    if (r < 0.22 || r > 0.28) return false;
  }
  return true;
}

std::string pct(double x) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(1) << x * 100.0 << "%";
  return out.str();
}

std::string cardName(const fr::Engine& engine, const std::string& id) {
  const fr::Card* card = engine.cardById(id);
  return card ? card->name : id;
}

std::string slugify(std::string value) {
  std::string out;
  for (char c : value) {
    if (std::isalnum(static_cast<unsigned char>(c))) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    else if (!out.empty() && out.back() != '_') out.push_back('_');
  }
  while (!out.empty() && out.back() == '_') out.pop_back();
  return out.empty() ? "profile" : out;
}

void recordGame(fr::SimulationSummary& summary, std::map<std::string, int>& roundTotals, const fr::GameState& state) {
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

void finalizeAverages(fr::SimulationSummary& summary, const fr::Engine& engine, const std::map<std::string, int>& roundTotals) {
  for (const auto& klass : engine.classes()) {
    int wins = summary.wins[klass.id];
    auto it = roundTotals.find(klass.id);
    summary.avgRounds[klass.id] = wins && it != roundTotals.end() ? static_cast<double>(it->second) / wins : 0.0;
  }
}

void writeCheckpoint(const fr::Engine& engine, const std::string& label, const fr::SimulationSummary& summary, const fr::Balance& balance, int completed, int total) {
  fs::create_directories("reports/checkpoints");
  std::ofstream out("reports/checkpoints/" + slugify(label) + ".json");
  out << "{\"label\":\"" << fr::escapeJson(label) << "\",\"completed\":" << completed << ",\"total\":" << total
      << ",\"summary\":" << engine.serializeSummary(summary, balance) << "}\n";
}

fr::SimulationSummary runProfile(const fr::Engine& engine, const std::string& label, int games, int depth, const fr::Balance& balance, uint32_t seed) {
  fr::SimulationSummary summary;
  summary.games = games;
  std::map<std::string, int> roundTotals;
  auto started = std::chrono::steady_clock::now();
  std::cerr << "[start] " << label << " games=" << games << " depth=" << depth << "\n";
  int checkpointEvery = std::max(1, games / 25);
  for (int i = 0; i < games; ++i) {
    fr::GameState state = engine.playGame(seed + i * 7919, depth, balance);
    recordGame(summary, roundTotals, state);
    finalizeAverages(summary, engine, roundTotals);
    if ((i + 1) % checkpointEvery == 0 || i + 1 == games) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - started).count();
      std::cerr << "[progress] " << label << " " << (i + 1) << "/" << games << " elapsed=" << elapsed << "s";
      for (const auto& klass : engine.classes()) std::cerr << " " << klass.id << "=" << pct(rate(summary, klass.id));
      std::cerr << "\n";
      writeCheckpoint(engine, label, summary, balance, i + 1, games);
    }
  }
  std::cerr << "[done] " << label << "\n";
  return summary;
}

void appendCardTables(std::ostringstream& report, const fr::Engine& engine, const fr::SimulationSummary& summary) {
  std::vector<std::pair<std::string, int>> byPlays(summary.cardPlays.begin(), summary.cardPlays.end());
  std::sort(byPlays.begin(), byPlays.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) return a.second > b.second;
    return a.first < b.first;
  });

  report << "\nMost-played cards:\n\n";
  report << "| Card | Plays | Winner plays | Winner-play rate |\n";
  report << "|---|---:|---:|---:|\n";
  for (size_t i = 0; i < std::min<size_t>(8, byPlays.size()); ++i) {
    const auto& [id, plays] = byPlays[i];
    int winnerPlays = summary.winnerCardPlays.count(id) ? summary.winnerCardPlays.at(id) : 0;
    report << "| " << cardName(engine, id) << " | " << plays << " | " << winnerPlays << " | " << pct(static_cast<double>(winnerPlays) / plays) << " |\n";
  }

  int minPlays = std::max(6, summary.games / 40);
  std::vector<std::pair<std::string, double>> byLift;
  for (const auto& [id, plays] : summary.cardPlays) {
    if (plays < minPlays) continue;
    int winnerPlays = summary.winnerCardPlays.count(id) ? summary.winnerCardPlays.at(id) : 0;
    byLift.push_back({id, static_cast<double>(winnerPlays) / plays});
  }
  std::sort(byLift.begin(), byLift.end(), [&](const auto& a, const auto& b) {
    if (a.second != b.second) return a.second > b.second;
    return summary.cardPlays.at(a.first) > summary.cardPlays.at(b.first);
  });

  report << "\nStrongest cards by winner-play rate (minimum " << minPlays << " plays):\n\n";
  report << "| Card | Plays | Winner plays | Winner-play rate |\n";
  report << "|---|---:|---:|---:|\n";
  for (size_t i = 0; i < std::min<size_t>(8, byLift.size()); ++i) {
    const auto& [id, lift] = byLift[i];
    int plays = summary.cardPlays.at(id);
    int winnerPlays = summary.winnerCardPlays.count(id) ? summary.winnerCardPlays.at(id) : 0;
    report << "| " << cardName(engine, id) << " | " << plays << " | " << winnerPlays << " | " << pct(lift) << " |\n";
  }

  report << "\nStrongest card by class:\n\n";
  report << "| Class | Card | Plays by class | Winning plays | Winner-play rate |\n";
  report << "|---|---|---:|---:|---:|\n";
  for (const auto& klass : engine.classes()) {
    std::string bestId;
    double bestRate = -1.0;
    int bestPlays = 0;
    int bestWins = 0;
    auto classIt = summary.classCardPlays.find(klass.id);
    if (classIt != summary.classCardPlays.end()) {
      for (const auto& [id, plays] : classIt->second) {
        if (plays < 3) continue;
        int wins = 0;
        auto winnerClassIt = summary.classWinnerCardPlays.find(klass.id);
        if (winnerClassIt != summary.classWinnerCardPlays.end() && winnerClassIt->second.count(id)) wins = winnerClassIt->second.at(id);
        double r = static_cast<double>(wins) / plays;
        if (r > bestRate || (r == bestRate && plays > bestPlays)) {
          bestId = id;
          bestRate = r;
          bestPlays = plays;
          bestWins = wins;
        }
      }
    }
    report << "| " << klass.name << " | " << (bestId.empty() ? "n/a" : cardName(engine, bestId)) << " | " << bestPlays << " | " << bestWins << " | "
           << (bestRate < 0 ? "n/a" : pct(bestRate)) << " |\n";
  }
}

void appendSummary(std::ostringstream& report, const std::string& title, const fr::Engine& engine, const fr::SimulationSummary& summary, const fr::Balance& balance) {
  report << "## " << title << "\n\n";
  report << "| Class | Wins | Win rate | Avg winning round |\n";
  report << "|---|---:|---:|---:|\n";
  for (const auto& klass : engine.classes()) {
    int wins = summary.wins.count(klass.id) ? summary.wins.at(klass.id) : 0;
    report << "| " << klass.name << " | " << wins << " | " << pct(rate(summary, klass.id)) << " | "
           << std::fixed << std::setprecision(1) << summary.avgRounds.at(klass.id) << " |\n";
  }
  appendCardTables(report, engine, summary);
  if (balance.costDelta.empty() && balance.rewardDelta.empty() && balance.classStartDelta.empty()) {
    report << "\nTweaks active: none. This is the initial unmodified balance from the imported spreadsheet/CSV data.\n";
  } else {
    report << "\nTweaks active:\n";
    for (const auto& [id, delta] : balance.costDelta) report << "- `" << id << "` cost delta " << (delta >= 0 ? "+" : "") << delta << "\n";
    for (const auto& [id, delta] : balance.rewardDelta) report << "- `" << id << "` reward delta " << (delta >= 0 ? "+" : "") << delta << "\n";
    for (const auto& [klass, stats] : balance.classStartDelta) {
      for (const auto& [stat, delta] : stats) report << "- `" << klass << "." << stat << "` start delta " << (delta >= 0 ? "+" : "") << delta << "\n";
    }
    for (const auto& [goal, delta] : balance.goalDelta) report << "- `" << goal << "` goal delta " << (delta >= 0 ? "+" : "") << delta << "\n";
  }
  report << "\n";
}
}  // namespace

int main(int argc, char** argv) {
  fr::Engine engine;
  fs::create_directories("reports");
  int gamesPerRun = argc > 1 ? std::atoi(argv[1]) : 250;
  int searchDepth = argc > 2 ? std::atoi(argv[2]) : 6;

  std::ostringstream report;
  report << "# French Rev Balance Simulation\n\n";
  report << "AI policy: max-n depth-" << searchDepth << " lookahead for all simulated AI turns. "
            "Each AI optimizes its own future position while evaluating immediate win progress, opponent progress, resource growth, starvation risk, class-specific objective distance, and high-threat opponent states. "
            "The action market starts with 9 cards.\n\n";

  fr::Balance baselineBalance;
  auto baseline = runProfile(engine, "Baseline", gamesPerRun, searchDepth, baselineBalance, 178900);
  appendSummary(report, "Baseline: " + std::to_string(gamesPerRun) + " All-AI Games, No Tweaks", engine, baseline, baselineBalance);
  {
    std::ofstream out("reports/balance.in_progress.md");
    out << report.str();
  }

  std::vector<std::pair<std::string, fr::Balance>> trials;

  fr::Balance t1;
  t1.costDelta["kill_enemy_of_revolution"] = 1;
  t1.costDelta["guillotine_king_louis_xvi"] = 1;
  t1.costDelta["appoint_general"] = 1;
  trials.push_back({"Tweak 1: slow Committee executions and weapon burst", t1});

  fr::Balance t2 = t1;
  t2.costDelta["build_farm"] = -1;
  t2.costDelta["abolish_feudalism"] = -1;
  t2.rewardDelta["beg"] = 1;
  trials.push_back({"Tweak 2: improve Peasant food engine", t2});

  fr::Balance t3 = t2;
  t3.costDelta["enter_enlightenment"] = -1;
  t3.costDelta["habeas_corpus"] = -1;
  t3.costDelta["liberate"] = -1;
  trials.push_back({"Tweak 3: accelerate Bourgeoisie freedom path", t3});

  fr::Balance t4 = t3;
  t4.costDelta["absolute_monarchy"] = -1;
  t4.costDelta["hms_revolutionnaire"] = 1;
  t4.rewardDelta["tithe_peasants"] = 1;
  trials.push_back({"Tweak 4: give Nobles a sharper monarchy/economy path", t4});

  fr::Balance t5 = t3;
  t5.classStartDelta["peasants"] = {{"assembly", 2}, {"food", 4}, {"foodPerTurn", 1}, {"foodRequired", -1}, {"gold", 2}};
  t5.classStartDelta["nobles"] = {{"assembly", -2}, {"gold", -3}, {"food", -2}, {"weaponry", -1}};
  t5.costDelta["nepotism"] = 2;
  t5.costDelta["absolute_monarchy"] = 2;
  t5.costDelta["tithe_peasants"] = 1;
  trials.push_back({"Tweak 5: rebalance starting estates and slow Noble assembly", t5});

  fr::Balance t6 = t5;
  t6.classStartDelta["bourgeoisie"] = {{"assembly", -1}, {"loyalty", 1}};
  t6.classStartDelta["committee"] = {{"weaponry", -1}, {"gold", -1}};
  t6.rewardDelta["pity_party"] = 1;
  t6.rewardDelta["build_farm"] = 1;
  trials.push_back({"Tweak 6: compress all class openings toward parity", t6});

  fr::Balance t7 = t3;
  t7.costDelta["abolish_feudalism"] = -2;
  t7.costDelta["build_farm"] = -2;
  t7.costDelta["nepotism"] = 1;
  t7.costDelta["absolute_monarchy"] = 1;
  t7.rewardDelta["beg"] = 2;
  t7.classStartDelta["peasants"] = {{"food", 3}, {"foodPerTurn", 1}, {"foodRequired", -1}, {"gold", 1}};
  t7.classStartDelta["nobles"] = {{"assembly", -1}, {"gold", -2}, {"food", -1}};
  trials.push_back({"Tweak 7: moderate Peasant economy and remove Noble free control", t7});

  fr::Balance t8 = t7;
  t8.classStartDelta["peasants"] = {{"food", 2}, {"foodPerTurn", 1}, {"foodRequired", -1}, {"gold", 1}};
  t8.classStartDelta["nobles"] = {{"assembly", -1}, {"gold", -3}, {"food", -2}, {"weaponry", -1}};
  t8.classStartDelta["bourgeoisie"] = {{"loyalty", 1}};
  t8.costDelta["hms_revolutionnaire"] = 1;
  trials.push_back({"Tweak 8: parity pass with Nobles slowed and Bourgeoisie steadied", t8});

  fr::Balance t9 = t3;
  t9.costDelta["abolish_feudalism"] = -1;
  t9.costDelta["build_farm"] = -1;
  t9.costDelta["enter_enlightenment"] = -2;
  t9.costDelta["liberate"] = -2;
  t9.rewardDelta["beg"] = 1;
  t9.classStartDelta["peasants"] = {{"food", 1}, {"foodPerTurn", 1}, {"gold", 1}};
  t9.classStartDelta["nobles"] = {{"assembly", -1}, {"gold", -1}};
  t9.classStartDelta["bourgeoisie"] = {{"loyalty", 1}, {"gold", 1}};
  trials.push_back({"Tweak 9: narrow parity pass after Peasant overshoot", t9});

  fr::Balance t10 = t9;
  t10.classStartDelta["peasants"] = {{"food", 2}, {"foodPerTurn", 1}, {"gold", 1}};
  t10.classStartDelta["nobles"] = {{"assembly", -1}, {"gold", -1}, {"food", -1}};
  t10.costDelta["nepotism"] = 1;
  t10.rewardDelta["political_convention"] = 1;
  trials.push_back({"Tweak 10: final small Peasant/Noble/Bourgeoisie nudge", t10});

  fr::Balance t11 = t3;
  t11.costDelta["abolish_feudalism"] = -2;
  t11.costDelta["build_farm"] = -1;
  t11.rewardDelta["beg"] = 1;
  t11.classStartDelta["peasants"] = {{"food", 2}, {"foodPerTurn", 1}, {"gold", 1}};
  t11.classStartDelta["nobles"] = {{"assembly", -1}, {"gold", -1}};
  trials.push_back({"Tweak 11: selected midpoint toward 25 percent", t11});

  fr::Balance t12 = t3;
  t12.costDelta["abolish_feudalism"] = -1;
  t12.costDelta["build_farm"] = -1;
  t12.costDelta["nepotism"] = 1;
  t12.rewardDelta["beg"] = 1;
  t12.classStartDelta["peasants"] = {{"food", 2}, {"foodPerTurn", 1}, {"gold", 1}};
  t12.classStartDelta["nobles"] = {{"assembly", -1}, {"gold", -1}, {"food", -1}};
  trials.push_back({"Tweak 12: balanced card-only midpoint", t12});

  fr::Balance t13 = t12;
  t13.costDelta["appoint_general"] = 2;
  t13.costDelta["kill_enemy_of_revolution"] = 2;
  t13.costDelta["nepotism"] = 2;
  t13.costDelta["enter_enlightenment"] = -2;
  t13.classStartDelta["bourgeoisie"] = {{"loyalty", 1}};
  trials.push_back({"Tweak 13: lift Bourgeoisie while trimming Noble and Committee spikes", t13});

  fr::Balance t14 = t12;
  t14.costDelta["appoint_general"] = 3;
  t14.costDelta["kill_enemy_of_revolution"] = 3;
  t14.costDelta["abolish_feudalism"] = -2;
  t14.classStartDelta["peasants"] = {{"food", 2}, {"foodPerTurn", 1}, {"foodRequired", -1}, {"gold", 1}};
  t14.classStartDelta["nobles"] = {{"assembly", -1}, {"gold", -1}};
  trials.push_back({"Tweak 14: Peasant lift with Committee brake and no Bourgeoisie bonus", t14});

  trials.clear();
  fr::Balance a1;
  a1.costDelta["appoint_general"] = 2;
  a1.costDelta["abolish_feudalism"] = -1;
  a1.costDelta["build_farm"] = -1;
  a1.costDelta["guillotine_king_louis_xvi"] = 1;
  a1.costDelta["kill_enemy_of_revolution"] = 3;
  a1.costDelta["enter_enlightenment"] = 1;
  a1.costDelta["habeas_corpus"] = 1;
  a1.costDelta["liberate"] = 1;
  a1.costDelta["nepotism"] = 1;
  a1.classStartDelta["peasants"] = {{"food", 2}, {"foodPerTurn", 1}};
  a1.classStartDelta["nobles"] = {{"gold", 1}, {"weaponry", 1}};
  a1.goalDelta["peasants_food"] = -1;
  a1.goalDelta["committee_enemies"] = 1;
  a1.goalDelta["bourgeoisie_national_freedom"] = 1;
  a1.goalDelta["bourgeoisie_loyalty"] = 1;
  a1.goalDelta["nobles_national_freedom"] = 1;
  a1.goalDelta["nobles_gold"] = -2;
  a1.goalDelta["nobles_weaponry"] = -1;
  trials.push_back({"Tweak 1: selected near-miss profile rerun", a1});

  fr::Balance a2 = a1;
  a2.costDelta["appoint_general"] = 3;
  a2.costDelta["kill_enemy_of_revolution"] = 4;
  a2.costDelta["guillotine_king_louis_xvi"] = 2;
  a2.goalDelta["committee_enemies"] = 2;
  trials.push_back({"Tweak 2: tax Committee execution and king tempo", a2});

  fr::Balance a3 = a1;
  a3.costDelta["appoint_general"] = 3;
  a3.costDelta["kill_enemy_of_revolution"] = 4;
  a3.costDelta["guillotine_king_louis_xvi"] = 2;
  a3.goalDelta["committee_enemies"] = 2;
  a3.classStartDelta["committee"] = {{"weaponry", -1}, {"gold", -1}};
  trials.push_back({"Tweak 3: tax Committee plus trim starting weapon cushion", a3});

  fr::Balance a4 = a3;
  a4.classStartDelta["bourgeoisie"] = {{"loyalty", 1}, {"gold", 1}};
  trials.push_back({"Tweak 4: Committee tax with light Bourgeoisie opening help", a4});

  fr::Balance a5 = a3;
  a5.costDelta["enter_enlightenment"] = 0;
  a5.costDelta["habeas_corpus"] = 0;
  a5.costDelta["liberate"] = 0;
  a5.goalDelta["bourgeoisie_national_freedom"] = 0;
  trials.push_back({"Tweak 5: Committee tax with restored Bourgeoisie endgame", a5});

  fr::Balance a6 = a3;
  a6.classStartDelta["peasants"] = {{"food", 3}, {"foodPerTurn", 1}};
  a6.goalDelta["peasants_food"] = -2;
  trials.push_back({"Tweak 6: Committee tax with stronger Peasant floor", a6});

  fr::Balance a7 = a2;
  a7.classStartDelta["nobles"] = {{"gold", 1}};
  a7.goalDelta["nobles_gold"] = -3;
  trials.push_back({"Tweak 7: Committee tax without extra Noble weaponry", a7});

  fr::Balance a8 = a4;
  a8.classStartDelta["peasants"] = {{"food", 3}, {"foodPerTurn", 1}};
  a8.goalDelta["peasants_food"] = -2;
  a8.goalDelta["bourgeoisie_loyalty"] = 0;
  trials.push_back({"Tweak 8: final parity candidate with Committee tax and civic lift", a8});

  fr::Balance best = baselineBalance;
  fr::SimulationSummary bestSummary = baseline;
  std::string bestLabel = "Baseline";
  bool bestInBand = inTargetBand(baseline, engine);
  double bestError = 0.0;
  for (const auto& klass : engine.classes()) {
    double r = rate(baseline, klass.id);
    bestError += std::abs(r - 0.25);
  }

  for (size_t i = 0; i < trials.size(); ++i) {
    auto summary = runProfile(engine, trials[i].first, gamesPerRun, searchDepth, trials[i].second, 220000 + static_cast<int>(i) * 1000);
    appendSummary(report, trials[i].first + ": " + std::to_string(gamesPerRun) + " Games", engine, summary, trials[i].second);
    {
      std::ofstream out("reports/balance.in_progress.md");
      out << report.str();
    }
    double error = 0.0;
    for (const auto& klass : engine.classes()) error += std::abs(rate(summary, klass.id) - 0.25);
    bool hitTarget = inTargetBand(summary, engine);
    if ((hitTarget && !bestInBand) || (hitTarget == bestInBand && error < bestError)) {
      bestError = error;
      best = trials[i].second;
      bestSummary = summary;
      bestLabel = trials[i].first;
      bestInBand = hitTarget;
    }
  }

  auto finalSummary = bestSummary;
  appendSummary(report, "Selected Final Profile: " + bestLabel + " (" + std::to_string(gamesPerRun) + " Games)", engine, finalSummary, best);

  report << "## Recommendation\n\n";
  report << "Selected the tweak set that hits the 22-28% target band; if no candidate hits the band, the simulator falls back to the lowest total distance from a 25% win rate. "
            "The selected profile " << (bestInBand ? "hit" : "did not hit") << " the target band after a " << gamesPerRun << "-game depth-" << searchDepth << " simulation. "
            "The most important levers were objective thresholds, class starts, and the cost of explosive objective cards rather than generic economy cards alone.\n";

  std::ofstream out("reports/balance.md");
  out << report.str();
  std::ofstream json("reports/final_simulation.json");
  json << engine.serializeSummary(finalSummary, best) << "\n";
  std::cout << report.str();
}
