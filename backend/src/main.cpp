#include "engine.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace fs = std::filesystem;

namespace {
fr::Engine engine;
fr::Balance playableBalance() {
  fr::Balance b;
  b.costDelta["abolish_feudalism"] = -1;
  b.costDelta["appoint_general"] = 3;
  b.costDelta["build_farm"] = -1;
  b.costDelta["enter_enlightenment"] = 1;
  b.costDelta["guillotine_king_louis_xvi"] = 2;
  b.costDelta["habeas_corpus"] = 1;
  b.costDelta["kill_enemy_of_revolution"] = 4;
  b.costDelta["liberate"] = 1;
  b.costDelta["nepotism"] = 1;
  b.classStartDelta["bourgeoisie"] = {{"gold", 1}, {"loyalty", 1}};
  b.classStartDelta["committee"] = {{"gold", -1}, {"weaponry", -1}};
  b.classStartDelta["nobles"] = {{"gold", 1}, {"weaponry", 1}};
  b.classStartDelta["peasants"] = {{"food", 2}, {"foodPerTurn", 1}};
  b.goalDelta["bourgeoisie_loyalty"] = 1;
  b.goalDelta["bourgeoisie_national_freedom"] = 1;
  b.goalDelta["committee_enemies"] = 2;
  b.goalDelta["nobles_gold"] = -2;
  b.goalDelta["nobles_national_freedom"] = 1;
  b.goalDelta["nobles_weaponry"] = -1;
  b.goalDelta["peasants_food"] = -1;
  return b;
}
fr::Balance balance = playableBalance();
fr::GameState game;
std::mt19937 rng(1789);

std::string readFile(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return "";
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string contentType(const std::string& path) {
  if (path.ends_with(".js")) return "application/javascript";
  if (path.ends_with(".css")) return "text/css";
  if (path.ends_with(".json")) return "application/json";
  if (path.ends_with(".svg")) return "image/svg+xml";
  return "text/html";
}

bool bodyHas(const std::string& body, const std::string& token) {
  return body.find(token) != std::string::npos;
}

std::string findJsonString(const std::string& body, const std::string& key) {
  std::string marker = "\"" + key + "\"";
  size_t pos = body.find(marker);
  if (pos == std::string::npos) return "";
  pos = body.find(':', pos);
  if (pos == std::string::npos) return "";
  pos = body.find('"', pos);
  if (pos == std::string::npos) return "";
  size_t end = body.find('"', pos + 1);
  if (end == std::string::npos) return "";
  return body.substr(pos + 1, end - pos - 1);
}

std::vector<std::string> humanClassesFromBody(const std::string& body) {
  std::vector<std::string> humans;
  for (const auto& id : {"peasants", "nobles", "committee", "bourgeoisie"}) {
    if (bodyHas(body, "\"" + std::string(id) + "\"")) humans.push_back(id);
  }
  return humans;
}

std::string ok(const std::string& body, const std::string& type = "application/json") {
  std::ostringstream out;
  out << "HTTP/1.1 200 OK\r\nContent-Type: " << type << "\r\nAccess-Control-Allow-Origin: *\r\n"
      << "Access-Control-Allow-Headers: content-type\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;
  return out.str();
}

std::string notFound() {
  std::string body = "Not found";
  return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

std::string handle(const std::string& request) {
  std::istringstream lines(request);
  std::string method, path, version;
  lines >> method >> path >> version;
  size_t bodyAt = request.find("\r\n\r\n");
  std::string body = bodyAt == std::string::npos ? "" : request.substr(bodyAt + 4);
  if (method == "OPTIONS") return ok("{}");
  if (path == "/api/data") return ok(engine.serializeData());
  if (path == "/api/state") return ok(engine.serializeState(game, balance));
  if (path == "/api/new") {
    auto humans = humanClassesFromBody(body);
    if (humans.empty()) humans.push_back("peasants");
    game = engine.newGame(humans, static_cast<uint32_t>(std::random_device{}()), balance);
    return ok(engine.serializeState(game, balance));
  }
  if (path == "/api/action") {
    fr::Move move{findJsonString(body, "type"), findJsonString(body, "cardId")};
    bool applied = engine.applyMove(game, move, rng, balance);
    if (applied) {
      while (!game.finished && !game.players[game.active].human) {
        fr::Move ai = engine.chooseAiMove(game, game.active, 6, balance);
        engine.applyMove(game, ai, rng, balance);
      }
    }
    return ok(engine.serializeState(game, balance));
  }
  if (path == "/api/ai") {
    if (!game.finished) {
      fr::Move ai = engine.chooseAiMove(game, game.active, 6, balance);
      engine.applyMove(game, ai, rng, balance);
    }
    return ok(engine.serializeState(game, balance));
  }
  if (path == "/api/simulate") {
    auto summary = engine.simulate(100, 2, balance);
    return ok(engine.serializeSummary(summary, balance));
  }

  fs::path webRoot = fs::current_path() / "frontend" / "dist";
  std::string clean = path == "/" ? "/index.html" : path;
  fs::path target = webRoot / clean.substr(1);
  if (!fs::exists(target)) target = webRoot / "index.html";
  std::string file = readFile(target);
  if (file.empty()) return notFound();
  return ok(file, contentType(target.string()));
}
}  // namespace

int main(int argc, char** argv) {
  int port = argc > 1 ? std::stoi(argv[1]) : 8787;
  game = engine.newGame({"peasants"}, 1789, balance);
  int server = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if (bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || listen(server, 16) < 0) {
    std::cerr << "Failed to bind port " << port << ": " << std::strerror(errno) << "\n";
    return 1;
  }
  std::cout << "Republic of Virtue server on http://localhost:" << port << "\n";
  while (true) {
    int client = accept(server, nullptr, nullptr);
    if (client < 0) continue;
    char buffer[65536];
    ssize_t n = read(client, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      buffer[n] = 0;
      std::string response = handle(buffer);
      write(client, response.data(), response.size());
    }
    close(client);
  }
}
