import { joinRoom } from "../../../src/shared/rooms.js";
import { bodyObject, fail, idFrom, type Req, type Res, send } from "../../_shared.js";

export default function handler(req: Req, res: Res) {
  try {
    if (req.method !== "POST") return send(res, { error: "Not found" }, 404);
    const id = idFrom(req);
    if (!id) throw new Error("Room not found.");
    return send(res, joinRoom(id, bodyObject(req.body)));
  } catch (error) {
    return fail(res, error);
  }
}
