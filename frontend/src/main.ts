import "./style.css";

type ClassDef = {
  id: string;
  name: string;
  portrait: string;
  goals: { id: string; label: string }[];
};

type Card = {
  id: string;
  name: string;
  flavor: string;
  reward: string;
  cost: Partial<Record<keyof Stats, number>>;
  requirements: Partial<Record<keyof Stats, number>>;
  major: boolean;
  tactique: boolean;
  image: string;
};

type Stats = {
  food: number;
  gold: number;
  weaponry: number;
  loyalty: number;
  assembly: number;
  foodPerTurn: number;
  goldPerTurn: number;
  foodRequired: number;
};

type Player = {
  classId: string;
  name: string;
  human: boolean;
  enemiesKilled: number;
  goalsComplete: number;
  stats: Stats;
  majors: string[];
};

type Move = { type: "gain_food" | "gain_gold" | "play"; cardId?: string };

type GameState = {
  round: number;
  yearTick: number;
  active: number;
  nationalFreedom: number;
  finished: boolean;
  winner: string;
  market: string[];
  players: Player[];
  legal: Move[];
  log: string[];
};

type Seat = {
  classId: string;
  name: string;
  playerId: string;
  connected: boolean;
};

type Room = {
  id: string;
  createdAt: number;
  updatedAt: number;
  timerSeconds: number | null;
  turnStartedAt: number;
  seats: Seat[];
  classes: ClassDef[];
  game: GameState | null;
};

type GameData = {
  classes: ClassDef[];
  cards: Card[];
  balance?: {
    costDelta?: Record<string, number>;
  };
};

const app = document.querySelector<HTMLDivElement>("#app")!;
let data: GameData;
let room: Room | null = null;
let playerId = "";
let playerName = localStorage.getItem("fr_player_name") || "";
let creating = false;
let message = "";
let pollTimer = 0;
let clockTimer = 0;

const roomIdFromPath = () => {
  const match = location.pathname.match(/^\/room\/([^/]+)/);
  return match?.[1] ?? "";
};

const api = async <T>(path: string, body?: unknown): Promise<T> => {
  const response = await fetch(path, {
    method: body ? "POST" : "GET",
    headers: body ? { "content-type": "application/json" } : undefined,
    body: body ? JSON.stringify(body) : undefined,
  });
  const value = await response.json();
  if (!response.ok) throw new Error(value.error || "Request failed");
  return value as T;
};

const setMessage = (value: string) => {
  message = value;
  render();
};

const cardById = (id: string): Card | undefined => data.cards.find((card) => card.id === id);
const classById = (id: string): ClassDef | undefined => data.classes.find((klass) => klass.id === id);
const seatFor = (classId: string) => room?.seats.find((seat) => seat.classId === classId);
const playerStorageKey = (id: string) => `fr_player_${id}`;
const shareUrl = () => `${location.origin}/room/${room?.id ?? roomIdFromPath()}`;

const isMyTurn = () => {
  if (!room?.game || !playerId) return false;
  const active = room.game.players[room.game.active];
  return seatFor(active.classId)?.playerId === playerId;
};

const secondsLeft = () => {
  if (!room?.game || !room.timerSeconds || room.game.finished) return null;
  const active = room.game.players[room.game.active];
  if (!active.human) return null;
  return Math.max(0, Math.ceil(room.timerSeconds - (Date.now() - room.turnStartedAt) / 1000));
};

const isLegal = (type: string, cardId = "") => room?.game?.legal.some((move) => move.type === type && (move.cardId ?? "") === cardId) ?? false;

const statPill = (label: string, value: number, title?: string) =>
  `<span class="stat" title="${title ?? label}"><b>${label}</b>${value}</span>`;

const statLabels: Record<string, string> = {
  food: "F",
  gold: "G",
  weaponry: "W",
  loyalty: "L",
  assembly: "A",
  foodPerTurn: "F/t",
  goldPerTurn: "G/t",
  foodRequired: "Eat",
};

const adjustedCost = (card: Card, stat: string, value: number) => {
  if (!["food", "gold", "weaponry"].includes(stat)) return value;
  return Math.max(0, value + (data.balance?.costDelta?.[card.id] ?? 0));
};

const formatStats = (entries: Partial<Record<keyof Stats, number>>, card?: Card) =>
  Object.entries(entries)
    .filter(([, value]) => Number(value) > 0)
    .map(([stat, value]) => `${statLabels[stat] ?? stat}:${card ? adjustedCost(card, stat, Number(value)) : Number(value)}`)
    .join(" ");

const renderCardMeta = (card: Card) => {
  const cost = formatStats(card.cost, card) || "Free";
  const requirements = formatStats(card.requirements);
  return `
    <div class="card-meta" aria-label="Cost and requirements">
      <span title="Cost">Cost ${cost}</span>
      ${requirements ? `<span title="Requirement">Req ${requirements}</span>` : ""}
    </div>
  `;
};

const renderHome = () => `
  <section class="setup">
    <div>
      <p class="eyebrow">Republic of Virtue</p>
      <h1>French Revolution</h1>
    </div>
    <form class="room-panel" data-create-room>
      <label>
        Your name
        <input name="name" maxlength="32" value="${playerName}" placeholder="Camille" required />
      </label>
      <label>
        Turn timer
        <select name="timer">
          <option value="none">No timer</option>
          <option value="30">30 seconds</option>
          <option value="60">60 seconds</option>
          <option value="90">90 seconds</option>
          <option value="120">120 seconds</option>
          <option value="180">180 seconds</option>
        </select>
      </label>
      <button class="primary" ${creating ? "disabled" : ""}>Create share link</button>
      ${message ? `<p class="notice">${message}</p>` : ""}
    </form>
  </section>
`;

const renderLobby = () => {
  if (!room) return "";
  const link = shareUrl();
  return `
    <section class="setup lobby">
      <div>
        <p class="eyebrow">Room ${room.id}</p>
        <h1>Choose an Estate</h1>
      </div>
      <div class="room-panel wide">
        <div class="share-row">
          <input readonly value="${link}" />
          <button type="button" data-copy-link>Copy link</button>
        </div>
        <label>
          Your name
          <input data-name maxlength="32" value="${playerName}" placeholder="Camille" />
        </label>
        <div class="timer-copy">${room.timerSeconds ? `Timer: ${room.timerSeconds}s. If a human times out, AI makes that turn.` : "No turn timer."}</div>
      </div>
      <div class="class-picker">
        ${room.classes
          .map((klass) => {
            const seat = seatFor(klass.id);
            const mine = seat?.playerId === playerId;
            return `
              <button class="class-choice ${seat ? "selected" : ""} ${mine ? "mine" : ""}" data-join="${klass.id}" ${seat && !mine ? "disabled" : ""}>
                <img src="${klass.portrait}" alt="" />
                <span>${klass.name}</span>
                <small>${seat ? `${mine ? "You" : seat.name}` : "Open"}</small>
              </button>
            `;
          })
          .join("")}
      </div>
      <div class="setup-actions">
        <button class="primary" data-start-game ${room.seats.length ? "" : "disabled"}>Start game</button>
        <button data-copy-link>Copy invite</button>
        <a class="button-link" href="/">New room</a>
      </div>
      ${message ? `<p class="notice">${message}</p>` : ""}
    </section>
  `;
};

const renderPlayer = (player: Player, index: number) => {
  const klass = classById(player.classId);
  const active = index === room?.game?.active;
  const seat = seatFor(player.classId);
  return `
    <article class="player ${active ? "active" : ""} ${seat?.playerId === playerId ? "mine" : ""}">
      <img class="portrait" src="${klass?.portrait ?? ""}" alt="" />
      <div class="player-head">
        <div>
          <h2>${player.name}</h2>
          <small>${seat ? "Human" : "AI"} · ${player.goalsComplete}/3 goals</small>
        </div>
        <span class="turn-chip">${active ? "Turn" : seat?.playerId === playerId ? "You" : ""}</span>
      </div>
      <div class="stats">
        ${statPill("F", player.stats.food, "Food")}
        ${statPill("G", player.stats.gold, "Gold")}
        ${statPill("W", player.stats.weaponry, "Weaponry")}
        ${statPill("L", player.stats.loyalty, "Loyalty")}
        ${statPill("A", player.stats.assembly, "Assembly")}
        ${statPill("F/t", player.stats.foodPerTurn, "Food per turn")}
        ${statPill("G/t", player.stats.goldPerTurn, "Gold per turn")}
        ${statPill("Eat", player.stats.foodRequired, "Food required")}
      </div>
      <ul class="goals">${(klass?.goals ?? []).map((goal) => `<li>${goal.label}</li>`).join("")}</ul>
    </article>
  `;
};

const renderCard = (id: string) => {
  const card = cardById(id);
  if (!card) return "";
  const legal = isLegal("play", id) && isMyTurn();
  const kind = card.major ? "Major" : card.tactique ? "Tactique" : "";
  return `
    <article class="card ${card.major ? "major" : ""} ${legal ? "" : "locked"}">
      <div class="card-art">
        <img src="${card.image}" alt="" loading="lazy" />
        ${renderCardMeta(card)}
      </div>
      <div class="card-body">
        <div class="card-title">
          <h3>${card.name}</h3>
          ${kind ? `<span>${kind}</span>` : ""}
        </div>
        <p class="flavor">${card.flavor || "&nbsp;"}</p>
        <p>${card.reward}</p>
      </div>
      <button data-play="${card.id}" ${legal ? "" : "disabled"}>Play</button>
    </article>
  `;
};

const renderBoard = () => {
  const game = room?.game;
  if (!room || !game) return renderLobby();
  const active = game.players[game.active];
  const clock = secondsLeft();
  return `
    <section class="topbar">
      <div>
        <p class="eyebrow">Room ${room.id} · Round ${game.round} · Year token ${game.yearTick}/4</p>
        <h1>${game.finished ? `${classById(game.winner)?.name ?? game.winner} wins` : `${active.name}'s turn`}</h1>
      </div>
      <div class="freedom"><span>National Freedom</span><b>${game.nationalFreedom}</b></div>
      <div class="freedom"><span>Timer</span><b>${clock === null ? "∞" : clock}</b></div>
      <button data-copy-link>Invite</button>
    </section>
    <section class="board">
      <div class="players">${game.players.map(renderPlayer).join("")}</div>
      <section class="market">
        <div class="market-head">
          <h2>Market</h2>
          <div class="quick-actions">
            <button data-action="gain_food" ${isMyTurn() && isLegal("gain_food") && !game.finished ? "" : "disabled"}>Gain food</button>
            <button data-action="gain_gold" ${isMyTurn() && isLegal("gain_gold") && !game.finished ? "" : "disabled"}>Gain gold</button>
            <button data-ai-step ${game.finished ? "disabled" : ""}>AI step</button>
          </div>
        </div>
        <div class="cards">${game.market.map(renderCard).join("")}</div>
      </section>
      <aside class="log">
        <h2>Chronicle</h2>
        ${message ? `<p class="notice">${message}</p>` : ""}
        ${game.log.map((line) => `<p>${line}</p>`).join("")}
      </aside>
    </section>
  `;
};

const render = () => {
  if (!roomIdFromPath()) app.innerHTML = renderHome();
  else app.innerHTML = room?.game ? renderBoard() : renderLobby();
};

const loadRoom = async () => {
  const id = roomIdFromPath();
  if (!id) return;
  playerId = localStorage.getItem(playerStorageKey(id)) || playerId;
  room = await api<Room>(`/api/rooms/${id}`);
  render();
};

const startPolling = () => {
  window.clearInterval(pollTimer);
  window.clearInterval(clockTimer);
  pollTimer = window.setInterval(() => {
    if (roomIdFromPath()) loadRoom().catch((error) => setMessage(error instanceof Error ? error.message : String(error)));
  }, 2500);
  clockTimer = window.setInterval(render, 1000);
};

const postRoom = async (suffix: string, body?: unknown) => {
  const id = roomIdFromPath();
  room = await api<Room>(`/api/rooms/${id}/${suffix}`, body);
  render();
};

const bind = () => {
  app.addEventListener("submit", async (event) => {
    const form = event.target as HTMLFormElement;
    if (!form.matches("[data-create-room]")) return;
    event.preventDefault();
    const fields = new FormData(form);
    playerName = String(fields.get("name") || "").trim();
    localStorage.setItem("fr_player_name", playerName);
    creating = true;
    render();
    try {
      const created = await api<Room>("/api/rooms", { timerSeconds: fields.get("timer") });
      history.pushState(null, "", `/room/${created.id}`);
      room = created;
      startPolling();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    } finally {
      creating = false;
      render();
    }
  });

  app.addEventListener("input", (event) => {
    const target = event.target as HTMLInputElement;
    if (target.matches("[data-name]")) {
      playerName = target.value;
      localStorage.setItem("fr_player_name", playerName);
    }
  });

  app.addEventListener("click", async (event) => {
    const target = event.target as HTMLElement;
    try {
      const joinClass = target.closest<HTMLElement>("[data-join]")?.dataset.join;
      if (joinClass && room) {
        const joined = await api<Room & { playerId: string }>(`/api/rooms/${room.id}/join`, { classId: joinClass, name: playerName, playerId });
        playerId = joined.playerId;
        localStorage.setItem(playerStorageKey(room.id), playerId);
        room = joined;
        message = "";
        render();
        return;
      }
      if (target.closest("[data-start-game]")) {
        await postRoom("start");
        return;
      }
      if (target.closest("[data-copy-link]")) {
        await navigator.clipboard.writeText(shareUrl());
        setMessage("Invite link copied.");
        return;
      }
      const action = target.closest<HTMLElement>("[data-action]")?.dataset.action as Move["type"] | undefined;
      if (action) {
        await postRoom("action", { playerId, move: { type: action } });
        return;
      }
      const cardId = target.closest<HTMLElement>("[data-play]")?.dataset.play;
      if (cardId) {
        await postRoom("action", { playerId, move: { type: "play", cardId } });
        return;
      }
      if (target.closest("[data-ai-step]")) await postRoom("ai");
    } catch (error) {
      setMessage(error instanceof Error ? error.message : String(error));
    }
  });

  window.addEventListener("popstate", () => {
    room = null;
    loadRoom().catch(() => render());
  });
};

const main = async () => {
  data = await api<GameData>("/api/data");
  bind();
  if (roomIdFromPath()) {
    await loadRoom();
    startPolling();
  } else render();
};

main().catch((error) => {
  app.innerHTML = `<pre class="error">${error instanceof Error ? error.message : String(error)}</pre>`;
});
