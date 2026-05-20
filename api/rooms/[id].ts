import { getRoom } from "../../src/shared/rooms.js";
import { fail, idFrom, type Req, type Res, send } from "../_shared.js";

export default function handler(req: Req, res: Res) {
  try {
    if (req.method !== "GET") return send(res, { error: "Not found" }, 404);
    const id = idFrom(req);
    if (!id) throw new Error("Room not found.");
    return send(res, getRoom(id));
  } catch (error) {
    return fail(res, error);
  }
}
