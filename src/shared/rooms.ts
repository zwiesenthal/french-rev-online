import {
  applyMove,
  autoAdvanceAi,
  chooseAiMove,
  classes,
  type GameState,
  newGame,
  playableBalance,
  publicState,
  type Move,
} from "./engine.js";
import { randomBytes } from "node:crypto";

export type TimerSeconds = 30 | 60 | 90 | 120 | 180 | null;

export type Seat = {
  classId: string;
  name: string;
  playerId: string;
  connected: boolean;
};

export type Room = {
  id: string;
  createdAt: number;
  updatedAt: number;
  timerSeconds: TimerSeconds;
  turnStartedAt: number;
  seats: Seat[];
  game: GameState | null;
};

const rooms = new Map<string, Room>();
const balance = playableBalance();

const classIds = classes.map((klass: { id: string }) => klass.id);

const randomId = (length = 8) => {
  const alphabet = "abcdefghijkmnopqrstuvwxyz23456789";
  let out = "";
  const bytes = randomBytes(length);
  for (const byte of bytes) out += alphabet[byte % alphabet.length];
  return out;
};

const now = () => Date.now();

const normalizeTimer = (value: unknown): TimerSeconds => {
  if (value === null || value === undefined || value === "none") return null;
  const timer = Number(value);
  return [30, 60, 90, 120, 180].includes(timer) ? (timer as TimerSeconds) : null;
};

const touch = (room: Room) => {
  room.updatedAt = now();
};

const serializeRoom = (room: Room) => ({
  id: room.id,
  createdAt: room.createdAt,
  updatedAt: room.updatedAt,
  timerSeconds: room.timerSeconds,
  turnStartedAt: room.turnStartedAt,
  seats: room.seats,
  classes,
  game: room.game ? publicState(room.game, balance) : null,
});

const claimedClassIds = (room: Room, exceptPlayerId = "") =>
  new Set(room.seats.filter((seat) => seat.playerId !== exceptPlayerId).map((seat) => seat.classId));

const ensureRoom = (id: string) => {
  const room = rooms.get(id);
  if (!room) throw new Error("Room not found.");
  expireTurnIfNeeded(room);
  return room;
};

const updateGamePlayersFromSeats = (room: Room) => {
  if (!room.game) return;
  for (const player of room.game.players) {
    const seat = room.seats.find((next) => next.classId === player.classId);
    player.human = Boolean(seat);
    player.name = seat?.name || player.name;
  }
};

const continueAiTurns = (room: Room) => {
  if (!room.game) return;
  room.game = autoAdvanceAi(room.game, balance, 6, 16);
  room.turnStartedAt = now();
  touch(room);
};

const expireTurnIfNeeded = (room: Room) => {
  if (!room.game || room.game.finished || !room.timerSeconds) return;
  const active = room.game.players[room.game.active];
  if (!active.human) {
    continueAiTurns(room);
    return;
  }
  if (now() - room.turnStartedAt < room.timerSeconds * 1000) return;
  const move = chooseAiMove(room.game, room.game.active, 6, balance);
  room.game.log.push(`${active.name}'s timer expired; AI made the play.`);
  room.game = applyMove(room.game, move, balance);
  continueAiTurns(room);
};

export const createRoom = (timerSeconds: unknown) => {
  let id = randomId();
  while (rooms.has(id)) id = randomId();
  const room: Room = {
    id,
    createdAt: now(),
    updatedAt: now(),
    timerSeconds: normalizeTimer(timerSeconds),
    turnStartedAt: now(),
    seats: [],
    game: null,
  };
  rooms.set(id, room);
  return serializeRoom(room);
};

export const getRoom = (id: string) => serializeRoom(ensureRoom(id));

export const joinRoom = (id: string, body: Record<string, unknown>) => {
  const room = ensureRoom(id);
  if (room.game) throw new Error("This game has already started.");
  const classId = String(body.classId || "");
  if (!classIds.includes(classId)) throw new Error("Pick one of the four classes.");
  const playerId = String(body.playerId || randomId(18));
  const taken = claimedClassIds(room, playerId);
  if (taken.has(classId)) throw new Error("That class is already claimed.");
  room.seats = room.seats.filter((seat) => seat.playerId !== playerId);
  room.seats.push({
    classId,
    playerId,
    connected: true,
    name: String(body.name || "").trim().slice(0, 32) || classes.find((klass: { id: string }) => klass.id === classId)?.name || "Player",
  });
  touch(room);
  return { ...serializeRoom(room), playerId };
};

export const startRoom = (id: string) => {
  const room = ensureRoom(id);
  const humans = room.seats.map((seat) => seat.classId);
  room.game = newGame(humans, now() >>> 0, balance);
  updateGamePlayersFromSeats(room);
  room.turnStartedAt = now();
  continueAiTurns(room);
  touch(room);
  return serializeRoom(room);
};

export const applyRoomAction = (id: string, body: Record<string, unknown>) => {
  const room = ensureRoom(id);
  if (!room.game) throw new Error("Start the game first.");
  if (room.game.finished) return serializeRoom(room);
  const active = room.game.players[room.game.active];
  const seat = room.seats.find((next) => next.classId === active.classId);
  if (active.human && seat?.playerId !== body.playerId) throw new Error("It is not your turn.");
  const move = body.move as Move | undefined;
  if (!move?.type) throw new Error("Missing move.");
  room.game = applyMove(room.game, move, balance);
  updateGamePlayersFromSeats(room);
  room.turnStartedAt = now();
  continueAiTurns(room);
  touch(room);
  return serializeRoom(room);
};

export const forceAiStep = (id: string) => {
  const room = ensureRoom(id);
  if (!room.game || room.game.finished) return serializeRoom(room);
  room.game = applyMove(room.game, chooseAiMove(room.game, room.game.active, 6, balance), balance);
  continueAiTurns(room);
  touch(room);
  return serializeRoom(room);
};
