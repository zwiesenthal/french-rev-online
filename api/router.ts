import { cards, classes, playableBalance } from "../src/shared/engine.js";
import { applyRoomAction, createRoom, forceAiStep, getRoom, joinRoom, startRoom, updatePresence } from "../src/shared/rooms.js";

type Req = {
  method?: string;
  query?: Record<string, string | string[]>;
  body?: unknown;
  url?: string;
};

type Res = {
  status: (code: number) => Res;
  json: (value: unknown) => void;
  setHeader: (key: string, value: string) => void;
};

const bodyObject = (body: unknown): Record<string, unknown> => {
  if (!body) return {};
  if (typeof body === "string") {
    try {
      return JSON.parse(body) as Record<string, unknown>;
    } catch {
      return {};
    }
  }
  return body as Record<string, unknown>;
};

const normalizeParts = (raw: string[]) => {
  const parts = raw.flatMap((part) => part.split("/")).filter(Boolean);
  return parts[0] === "api" ? parts.slice(1) : parts;
};

const partsFrom = (req: Req) => {
  const raw = req.query?.path ?? [];
  const parts = normalizeParts(Array.isArray(raw) ? raw : [raw]);
  const urlParts = normalizeParts((req.url ?? "").split("?")[0].split("/"));
  return ["data", "rooms"].includes(parts[0] ?? "") ? parts : urlParts;
};

const send = (res: Res, value: unknown, code = 200) => {
  res.setHeader("cache-control", "no-store");
  res.status(code).json(value);
};

export default function handler(req: Req, res: Res) {
  try {
    const method = req.method ?? "GET";
    const parts = partsFrom(req);
    const body = bodyObject(req.body);

    if (method === "GET" && parts[0] === "data") return send(res, { cards, classes, balance: playableBalance() });
    if (method === "POST" && parts[0] === "rooms" && parts.length === 1) return send(res, createRoom(body));
    if (method === "GET" && parts[0] === "rooms" && parts[1]) return send(res, getRoom(parts[1]));
    if (method === "POST" && parts[0] === "rooms" && parts[1] && parts[2] === "join") return send(res, joinRoom(parts[1], body));
    if (method === "POST" && parts[0] === "rooms" && parts[1] && parts[2] === "presence") return send(res, updatePresence(parts[1], body));
    if (method === "POST" && parts[0] === "rooms" && parts[1] && parts[2] === "start") return send(res, startRoom(parts[1]));
    if (method === "POST" && parts[0] === "rooms" && parts[1] && parts[2] === "action") return send(res, applyRoomAction(parts[1], body));
    if (method === "POST" && parts[0] === "rooms" && parts[1] && parts[2] === "ai") return send(res, forceAiStep(parts[1]));

    return send(res, { error: "Not found" }, 404);
  } catch (error) {
    return send(res, { error: error instanceof Error ? error.message : String(error) }, 400);
  }
}
