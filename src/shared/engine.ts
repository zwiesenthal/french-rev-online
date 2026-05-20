import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const cardsJson = require("../../data/cards.json") as unknown;
const classesJson = require("../../data/classes.json") as unknown;

export type StatKey = "food" | "gold" | "weaponry" | "loyalty" | "assembly" | "foodPerTurn" | "goldPerTurn" | "foodRequired";

export type Stats = Record<StatKey, number>;

export type Card = {
  id: string;
  name: string;
  flavor: string;
  cost: Partial<Record<StatKey, number>>;
  requirements: Partial<Record<StatKey, number>>;
  reward: string;
  roles: string[];
  major: boolean;
  tactique: boolean;
  count: number;
  image: string;
};

export type ClassDef = {
  id: string;
  name: string;
  start: Stats;
  goals: { id: string; label: string }[];
  portrait: string;
};

export type Balance = {
  costDelta: Record<string, number>;
  rewardDelta: Record<string, number>;
  classStartDelta: Record<string, Partial<Record<StatKey, number>>>;
  goalDelta: Record<string, number>;
};

export type Player = {
  classId: string;
  name: string;
  human: boolean;
  enemiesKilled: number;
  goalsComplete: number;
  starvationCount: number;
  stats: Stats;
  majors: string[];
  flags: string[];
};

export type Move = { type: "gain_food" | "gain_gold" | "play"; cardId?: string };

export type GameState = {
  round: number;
  yearTick: number;
  active: number;
  nationalFreedom: number;
  civilWar: boolean;
  finished: boolean;
  winner: string;
  players: Player[];
  deck: string[];
  discard: string[];
  market: string[];
  completedMajors: string[];
  playedCards: { cardId: string; classId: string }[];
  goalDelta: Record<string, number>;
  log: string[];
  seed: number;
  rngCursor: number;
};

export type PublicGameState = Omit<GameState, "deck" | "discard" | "playedCards" | "seed" | "rngCursor"> & {
  legal: Move[];
};

export const cards = cardsJson as Card[];
export const classes = classesJson as ClassDef[];

const classOrder = ["peasants", "nobles", "committee", "bourgeoisie"];
const cardIndex = new Map(cards.map((card) => [card.id, card]));
const classIndex = new Map(classes.map((klass) => [klass.id, klass]));

export const playableBalance = (): Balance => ({
  costDelta: {
    abolish_feudalism: -1,
    appoint_general: 3,
    build_farm: -1,
    enter_enlightenment: 1,
    guillotine_king_louis_xvi: 2,
    habeas_corpus: 1,
    kill_enemy_of_revolution: 4,
    liberate: 1,
    nepotism: 1,
  },
  rewardDelta: {},
  classStartDelta: {
    bourgeoisie: { gold: 1, loyalty: 1 },
    committee: { gold: -1, weaponry: -1 },
    nobles: { gold: 1, weaponry: 1 },
    peasants: { food: 2, foodPerTurn: 1 },
  },
  goalDelta: {
    bourgeoisie_loyalty: 1,
    bourgeoisie_national_freedom: 1,
    committee_enemies: 2,
    nobles_gold: -2,
    nobles_national_freedom: 1,
    nobles_weaponry: -1,
    peasants_food: -1,
  },
});

const canonicalRole = (role: string) => {
  const r = role.toLowerCase();
  if (r.includes("peasant")) return "peasants";
  if (r.includes("noble")) return "nobles";
  if (r.includes("committee")) return "committee";
  if (r.includes("bourgeois")) return "bourgeoisie";
  return r;
};

const includes = (text: string, needle: string) => text.toLowerCase().includes(needle.toLowerCase());

const parseNumberAfter = (text: string, start: number) => {
  for (let i = start; i < text.length; i++) {
    if (/\d/.test(text[i])) {
      let end = i;
      while (end < text.length && /\d/.test(text[end])) end++;
      return Number(text.slice(i, end));
    }
  }
  return 1;
};

const clampStat = (stat: string, value: number) => {
  if (stat === "food" || stat === "gold") return Math.max(0, value);
  return Math.max(0, Math.min(30, value));
};

const clone = <T>(value: T): T => JSON.parse(JSON.stringify(value)) as T;

const rng = (seed: number) => {
  let t = seed >>> 0;
  return () => {
    t += 0x6d2b79f5;
    let x = Math.imul(t ^ (t >>> 15), t | 1);
    x ^= x + Math.imul(x ^ (x >>> 7), x | 61);
    return ((x ^ (x >>> 14)) >>> 0) / 4294967296;
  };
};

const stateRng = (state: GameState) => rng((state.seed + state.rngCursor++ * 7919) >>> 0);

const shuffle = <T>(items: T[], random: () => number) => {
  for (let i = items.length - 1; i > 0; i--) {
    const j = Math.floor(random() * (i + 1));
    [items[i], items[j]] = [items[j], items[i]];
  }
};

const statValue = (stats: Stats, stat: string) => (stats as Record<string, number>)[stat] ?? 0;

const addStat = (stats: Stats, stat: string, amount: number) => {
  if (!(stat in stats)) return;
  (stats as Record<string, number>)[stat] = stat === "foodPerTurn" || stat === "goldPerTurn"
    ? Math.max(0, statValue(stats, stat) + amount)
    : stat === "foodRequired"
      ? Math.max(1, statValue(stats, stat) + amount)
      : clampStat(stat, statValue(stats, stat) + amount);
};

const target = (state: GameState, key: string, base: number) => Math.max(1, base + (state.goalDelta[key] ?? 0));

export const cardById = (id: string) => cardIndex.get(id);
export const classById = (id: string) => classIndex.get(id);

export const newGame = (humanClassIds: string[], seed = Date.now(), balance = playableBalance()): GameState => {
  const state: GameState = {
    round: 1,
    yearTick: 0,
    active: 0,
    nationalFreedom: 5,
    civilWar: false,
    finished: false,
    winner: "",
    players: [],
    deck: [],
    discard: [],
    market: [],
    completedMajors: [],
    playedCards: [],
    goalDelta: clone(balance.goalDelta),
    log: ["The Estates convene. Revolution begins."],
    seed,
    rngCursor: 1,
  };
  const humans = new Set(humanClassIds);
  for (const classId of classOrder) {
    const def = classById(classId);
    if (!def) continue;
    const player: Player = {
      classId: def.id,
      name: def.name,
      human: humans.has(def.id),
      enemiesKilled: 0,
      goalsComplete: 0,
      starvationCount: 0,
      stats: clone(def.start),
      majors: [],
      flags: [],
    };
    for (const [stat, delta] of Object.entries(balance.classStartDelta[def.id] ?? {})) addStat(player.stats, stat, delta ?? 0);
    state.players.push(player);
  }
  for (const card of cards) {
    for (let i = 0; i < Math.max(1, card.count); i++) state.deck.push(card.id);
  }
  shuffle(state.deck, stateRng(state));
  fillMarket(state);
  return state;
};

const collectIncome = (player: Player) => {
  if (!player.flags.includes("communism")) player.stats.gold += player.stats.goldPerTurn;
  player.stats.food += player.stats.foodPerTurn;
};

const adjustedCost = (cardId: string, stat: string, raw: number, balance: Balance) =>
  stat === "gold" || stat === "food" || stat === "weaponry" ? Math.max(0, raw + (balance.costDelta[cardId] ?? 0)) : raw;

const canAfford = (player: Player, card: Card, balance: Balance) =>
  Object.entries(card.cost).every(([stat, raw]) => statValue(player.stats, stat) >= adjustedCost(card.id, stat, raw ?? 0, balance));

const meetsRequirements = (player: Player, card: Card) =>
  Object.entries(card.requirements).every(([stat, amount]) => statValue(player.stats, stat) >= (amount ?? 0));

const payCost = (player: Player, card: Card, balance: Balance) => {
  for (const [stat, raw] of Object.entries(card.cost)) addStat(player.stats, stat, -adjustedCost(card.id, stat, raw ?? 0, balance));
};

export const legalMoves = (state: GameState, playerIndex = state.active, balance = playableBalance()): Move[] => {
  if (state.finished) return [];
  const player = clone(state.players[playerIndex]);
  collectIncome(player);
  const moves: Move[] = [{ type: "gain_food" }, { type: "gain_gold" }];
  const seen = new Set<string>();
  for (const id of state.market) {
    if (seen.has(id)) continue;
    seen.add(id);
    const card = cardById(id);
    if (!card) continue;
    if (!card.roles.some((role) => canonicalRole(role) === player.classId)) continue;
    if (card.major && state.completedMajors.includes(card.id)) continue;
    if (canAfford(player, card, balance) && meetsRequirements(player, card)) moves.push({ type: "play", cardId: id });
  }
  return moves;
};

const fillMarket = (state: GameState) => {
  while (state.market.length < 9 && (state.deck.length > 0 || state.discard.length > 0)) {
    if (state.deck.length === 0) {
      state.deck = state.discard;
      state.discard = [];
      shuffle(state.deck, stateRng(state));
    }
    const id = state.deck.pop();
    if (id) state.market.push(id);
  }
};

const hasAssemblyControl = (state: GameState, playerIndex: number) => {
  const value = state.players[playerIndex].stats.assembly;
  return state.players.every((player, i) => i === playerIndex || player.stats.assembly < value);
};

const hasMost = (state: GameState, playerIndex: number, stat: StatKey) => {
  const value = state.players[playerIndex].stats[stat];
  return state.players.every((player, i) => i === playerIndex || player.stats[stat] < value);
};

export const goalsComplete = (state: GameState, playerIndex: number) => {
  const player = state.players[playerIndex];
  let n = 0;
  if (player.classId === "peasants") {
    n += state.completedMajors.includes("abolish_feudalism") || player.flags.includes("feudalism_abolished") ? 1 : 0;
    const nobles = state.players.find((p) => p.classId === "nobles");
    n += nobles && player.stats.assembly > nobles.stats.assembly ? 1 : 0;
    n += player.stats.food >= target(state, "peasants_food", 15) ? 1 : 0;
  } else if (player.classId === "nobles") {
    n += hasAssemblyControl(state, playerIndex) ? 1 : 0;
    n += state.nationalFreedom <= target(state, "nobles_national_freedom", 1) ? 1 : 0;
    n += player.stats.gold >= target(state, "nobles_gold", 30) && player.stats.weaponry >= target(state, "nobles_weaponry", 10) ? 1 : 0;
  } else if (player.classId === "committee") {
    n += player.flags.includes("king_dead") || state.completedMajors.includes("guillotine_king_louis_xvi") ? 1 : 0;
    n += player.enemiesKilled >= target(state, "committee_enemies", 3) ? 1 : 0;
    n += hasMost(state, playerIndex, "weaponry") ? 1 : 0;
  } else if (player.classId === "bourgeoisie") {
    n += hasAssemblyControl(state, playerIndex) ? 1 : 0;
    n += (player.flags.includes("enlightenment") || state.completedMajors.includes("enter_enlightenment")) &&
      player.stats.loyalty >= target(state, "bourgeoisie_loyalty", 7) ? 1 : 0;
    n += state.nationalFreedom >= target(state, "bourgeoisie_national_freedom", 9) ? 1 : 0;
  }
  return n;
};

const strongestOpponent = (state: GameState, playerIndex: number) => {
  let best = playerIndex === 0 ? 1 : 0;
  let score = -Infinity;
  for (let i = 0; i < state.players.length; i++) {
    if (i === playerIndex) continue;
    const next = evaluate(state, i);
    if (next > score) {
      score = next;
      best = i;
    }
  }
  return best;
};

const applyCard = (state: GameState, playerIndex: number, card: Card, balance: Balance) => {
  const player = state.players[playerIndex];
  const reward = card.reward;
  const rewardLower = reward.toLowerCase();
  const rewardBoost = balance.rewardDelta[card.id] ?? 0;

  if (card.id.includes("build_farm")) player.stats.foodPerTurn++;
  if (card.id.includes("build_trading") || card.id.includes("build_factory")) player.stats.goldPerTurn += card.id.includes("factory") ? 2 : 1;
  if (card.id === "crop_rotation") player.stats.foodPerTurn *= 2;
  if (card.id === "bank_loan") player.stats.goldPerTurn = Math.max(0, player.stats.goldPerTurn - 1);
  if (card.id === "communism") player.flags.push("communism");
  if (card.id === "civil_war") state.civilWar = true;
  if (card.id === "enter_enlightenment") player.flags.push("enlightenment");
  if (card.id === "enter_reign_of_terror") player.flags.push("terror");
  if (card.id === "committee_of_general_security") player.flags.push("security");
  if (card.id === "kill_enemy_of_revolution") player.enemiesKilled++;
  if (card.id === "guillotine_marie_antionette" && player.classId === "committee") player.enemiesKilled += 2;
  if (card.id === "guillotine_king_louis_xvi") player.flags.push("king_dead");
  if (card.id === "abolish_feudalism") player.flags.push("feudalism_abolished");

  for (const [label, stat] of [["food", "food"], ["gold", "gold"], ["weaponry", "weaponry"], ["loyalty", "loyalty"], ["assembly", "assembly"]] as const) {
    const token = `${label} +`;
    let pos = 0;
    while ((pos = rewardLower.indexOf(token, pos)) >= 0) {
      const lineStart = Math.max(rewardLower.lastIndexOf(".", pos) + 1, 0);
      const prefix = rewardLower.slice(lineStart, pos);
      const belongsToOther = ["nobles", "peasants", "committee", "bourgeoisie"].some((other) => prefix.includes(other));
      if (!belongsToOther) {
        let amount = parseNumberAfter(rewardLower, pos + token.length);
        if ((stat === "food" || stat === "gold" || stat === "weaponry") && rewardBoost) amount += rewardBoost;
        addStat(player.stats, stat, amount);
      }
      pos += token.length;
    }
  }

  for (const [token, stat] of [["food per turn +", "foodPerTurn"], ["gold per turn +", "goldPerTurn"]] as const) {
    let pos = 0;
    while ((pos = rewardLower.indexOf(token, pos)) >= 0) {
      addStat(player.stats, stat, parseNumberAfter(rewardLower, pos + token.length));
      pos += token.length;
    }
  }

  for (const targetLabel of ["nobles", "peasants", "committee", "bourgeoisie"]) {
    for (const [label, stat] of [["food", "food"], ["gold", "gold"], ["weaponry", "weaponry"], ["loyalty", "loyalty"], ["assembly", "assembly"]] as const) {
      for (const sign of ["-", "+"]) {
        const token = `${targetLabel} ${label} ${sign}`;
        const pos = rewardLower.indexOf(token);
        if (pos >= 0) {
          const delta = parseNumberAfter(rewardLower, pos + token.length) * (sign === "-" ? -1 : 1);
          const classId = canonicalRole(targetLabel);
          for (const targetPlayer of state.players) if (targetPlayer.classId === classId) addStat(targetPlayer.stats, stat, delta);
        }
      }
    }
  }

  if (includes(reward, "National Freedom")) {
    const nfPos = rewardLower.indexOf("national freedom");
    const amount = nfPos >= 0 ? parseNumberAfter(rewardLower, nfPos) : 1;
    const direction = includes(reward, "National Freedom -") || player.classId === "nobles" ? -1 : 1;
    state.nationalFreedom = Math.max(0, Math.min(12, state.nationalFreedom + direction * amount));
  }
  if (includes(reward, "Lower loyalty and assembly") || includes(reward, "Lower assembly") || includes(reward, "Lower loyalty")) {
    const targetPlayer = state.players[strongestOpponent(state, playerIndex)];
    if (includes(reward, "loyalty")) addStat(targetPlayer.stats, "loyalty", -1);
    if (includes(reward, "assembly")) addStat(targetPlayer.stats, "assembly", -1);
  }
  if (includes(reward, "Steal up to 2 food")) {
    const targetPlayer = state.players[strongestOpponent(state, playerIndex)];
    const stolen = Math.min(2, targetPlayer.stats.food);
    targetPlayer.stats.food -= stolen;
    player.stats.food += stolen;
  }
  if (includes(reward, "Target Class loses 2 weapons")) {
    const targetPlayer = state.players[strongestOpponent(state, playerIndex)];
    targetPlayer.stats.weaponry = Math.max(0, targetPlayer.stats.weaponry - (state.civilWar ? 4 : 2));
    if (targetPlayer.stats.weaponry === 0) addStat(targetPlayer.stats, "loyalty", -1);
  }
};

const advanceTurn = (state: GameState) => {
  const order = state.players.map((_, i) => i).sort((a, b) => {
    const x = state.players[a].stats;
    const y = state.players[b].stats;
    if (state.civilWar && x.weaponry !== y.weaponry) return y.weaponry - x.weaponry;
    if (x.assembly !== y.assembly) return y.assembly - x.assembly;
    if (x.weaponry !== y.weaponry) return y.weaponry - x.weaponry;
    return y.loyalty - x.loyalty;
  });
  const pos = order.indexOf(state.active);
  if (pos + 1 < order.length) state.active = order[pos + 1];
  else {
    endRound(state);
    state.active = order[0] ?? 0;
  }
};

const endRound = (state: GameState) => {
  for (const player of state.players) {
    if (player.stats.food >= player.stats.foodRequired) player.stats.food -= player.stats.foodRequired;
    else {
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
};

const awardAssembly = (state: GameState) => {
  for (const stat of ["gold", "weaponry", "loyalty"] as StatKey[]) {
    const values = state.players.map((player) => player.stats[stat]);
    const best = Math.max(...values);
    if (values.filter((value) => value === best).length === 1) state.players[values.indexOf(best)].stats.assembly++;
  }
};

export const applyMove = (input: GameState, move: Move, balance = playableBalance()): GameState => {
  const state = clone(input);
  if (state.finished) return state;
  const player = state.players[state.active];
  if (move.type === "gain_food") {
    collectIncome(player);
    player.stats.food += 1;
    state.log.push(`${player.name} gathered food.`);
  } else if (move.type === "gain_gold") {
    collectIncome(player);
    if (!player.flags.includes("communism")) player.stats.gold += 1;
    state.log.push(`${player.name} raised gold.`);
  } else if (move.type === "play") {
    const card = move.cardId ? cardById(move.cardId) : undefined;
    const ok = card && legalMoves(state, state.active, balance).some((legal) => legal.type === "play" && legal.cardId === move.cardId);
    if (!card || !ok) throw new Error("That move is not legal right now.");
    collectIncome(player);
    payCost(player, card, balance);
    applyCard(state, state.active, card, balance);
    const marketIndex = state.market.indexOf(card.id);
    if (marketIndex >= 0) state.market.splice(marketIndex, 1);
    if (card.major) {
      if (!state.completedMajors.includes(card.id)) state.completedMajors.push(card.id);
      if (!player.majors.includes(card.id)) player.majors.push(card.id);
    } else if (!card.tactique) state.discard.push(card.id);
    state.playedCards.push({ cardId: card.id, classId: player.classId });
    fillMarket(state);
    state.log.push(`${player.name} played ${card.name}.`);
  }
  if (goalsComplete(state, state.active) >= 2) {
    state.finished = true;
    state.winner = player.classId;
    state.log.push(`${player.name} completed two objectives and won.`);
  } else advanceTurn(state);
  state.players.forEach((next, index) => {
    next.goalsComplete = goalsComplete(state, index);
  });
  state.log = state.log.slice(-24);
  return state;
};

const goalProgress = (state: GameState, playerIndex: number) => {
  const p = state.players[playerIndex];
  const complete = goalsComplete(state, playerIndex);
  let partial = 0;
  if (p.classId === "peasants") {
    const nobles = state.players.find((x) => x.classId === "nobles");
    partial += state.completedMajors.includes("abolish_feudalism") || p.flags.includes("feudalism_abolished") ? 1 : p.stats.weaponry >= 3 && p.stats.assembly >= 2 ? 0.45 : 0.15;
    if (nobles) partial += Math.max(0, Math.min(1, 0.5 + (p.stats.assembly - nobles.stats.assembly) / 6));
    partial += Math.max(0, Math.min(1, p.stats.food / target(state, "peasants_food", 15)));
  } else if (p.classId === "nobles") {
    partial += hasAssemblyControl(state, playerIndex) ? 1 : Math.max(0, Math.min(0.9, 0.45 + p.stats.assembly / 10));
    partial += Math.max(0, Math.min(1, (5 - state.nationalFreedom) / Math.max(1, 5 - target(state, "nobles_national_freedom", 1))));
    partial += Math.min(
      Math.max(0, Math.min(1, p.stats.gold / target(state, "nobles_gold", 30))),
      Math.max(0, Math.min(1, p.stats.weaponry / target(state, "nobles_weaponry", 10))),
    );
  } else if (p.classId === "committee") {
    partial += p.flags.includes("king_dead") || state.completedMajors.includes("guillotine_king_louis_xvi") ? 1 : Math.max(0, Math.min(0.85, (p.stats.weaponry + p.stats.assembly) / 10));
    partial += Math.max(0, Math.min(1, p.enemiesKilled / target(state, "committee_enemies", 3)));
    partial += hasMost(state, playerIndex, "weaponry") ? 1 : Math.max(0, Math.min(0.9, 0.4 + p.stats.weaponry / 12));
  } else {
    partial += hasAssemblyControl(state, playerIndex) ? 1 : Math.max(0, Math.min(0.9, 0.35 + p.stats.assembly / 10));
    partial += Math.min(p.flags.includes("enlightenment") || state.completedMajors.includes("enter_enlightenment") ? 1 : 0.45, Math.max(0, Math.min(1, p.stats.loyalty / target(state, "bourgeoisie_loyalty", 7))));
    partial += Math.max(0, Math.min(1, (state.nationalFreedom - 5) / Math.max(1, target(state, "bourgeoisie_national_freedom", 9) - 5)));
  }
  return complete + Math.max(0, partial - complete);
};

const evaluate = (state: GameState, playerIndex: number) => {
  const p = state.players[playerIndex];
  if (state.finished) return state.winner === p.classId ? 100000 : -100000;
  let score = goalProgress(state, playerIndex) * 850 + goalsComplete(state, playerIndex) * 420;
  score += p.stats.food * 7 + p.stats.gold * 7 + p.stats.weaponry * 13 + p.stats.loyalty * 16 + p.stats.assembly * 34;
  score += p.stats.foodPerTurn * 30 + p.stats.goldPerTurn * 34 - p.stats.foodRequired * 18 - p.starvationCount * 80;
  for (let i = 0; i < state.players.length; i++) {
    if (i === playerIndex) continue;
    score -= goalsComplete(state, i) * 310;
    score -= Math.max(0, goalProgress(state, i) - 1.35) * 95;
  }
  return score;
};

const search = (state: GameState, depth: number, balance: Balance): number[] => {
  if (depth <= 0 || state.finished) return state.players.map((_, i) => evaluate(state, i));
  const moves = legalMoves(state, state.active, balance);
  const ranked = moves
    .map((move) => {
      try {
        const next = applyMove(state, move, balance);
        return { move, score: evaluate(next, state.active) };
      } catch {
        return { move, score: -Infinity };
      }
    })
    .sort((a, b) => b.score - a.score)
    .slice(0, depth >= 2 ? 2 : 3);
  let bestValues: number[] | undefined;
  let best = -Infinity;
  for (const { move } of ranked) {
    const next = applyMove(state, move, balance);
    const values = search(next, depth - 1, balance);
    if (values[state.active] > best) {
      best = values[state.active];
      bestValues = values;
    }
  }
  return bestValues ?? state.players.map((_, i) => evaluate(state, i));
};

export const chooseAiMove = (state: GameState, playerIndex = state.active, depth = 5, balance = playableBalance()): Move => {
  const moves = legalMoves(state, playerIndex, balance);
  if (!moves.length) return { type: "gain_food" };
  const ranked = moves
    .map((move) => {
      try {
        const next = applyMove(state, move, balance);
        return { move, score: evaluate(next, playerIndex) };
      } catch {
        return { move, score: -Infinity };
      }
    })
    .sort((a, b) => b.score - a.score)
    .slice(0, depth >= 5 ? 3 : 4);
  let best = -Infinity;
  let bestMove = moves[0];
  for (const { move } of ranked) {
    const next = applyMove(state, move, balance);
    const values = search(next, depth - 1, balance);
    const opponentThreat = Math.max(...values.filter((_, i) => i !== playerIndex));
    const value = values[playerIndex] - opponentThreat * 0.08;
    if (value > best) {
      best = value;
      bestMove = move;
    }
  }
  return bestMove;
};

export const publicState = (state: GameState, balance = playableBalance()): PublicGameState => {
  const { deck: _deck, discard: _discard, playedCards: _playedCards, seed: _seed, rngCursor: _rngCursor, ...rest } = state;
  return { ...clone(rest), legal: legalMoves(state, state.active, balance) };
};

export const autoAdvanceAi = (state: GameState, balance = playableBalance(), depth = 5, maxSteps = 12) => {
  let next = state;
  let steps = 0;
  while (!next.finished && steps < maxSteps && !next.players[next.active].human) {
    next = applyMove(next, chooseAiMove(next, next.active, depth, balance), balance);
    steps++;
  }
  return next;
};
