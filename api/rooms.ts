import { createRoom } from "../src/shared/rooms.js";
import { bodyObject, fail, type Req, type Res, send } from "./_shared.js";

export default function handler(req: Req, res: Res) {
  try {
    if (req.method !== "POST") return send(res, { error: "Not found" }, 404);
    return send(res, createRoom(bodyObject(req.body).timerSeconds));
  } catch (error) {
    return fail(res, error);
  }
}
