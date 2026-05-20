import { cards, classes } from "../src/shared/engine.js";
import { type Req, type Res, send } from "./_shared.js";

export default function handler(_req: Req, res: Res) {
  return send(res, { cards, classes });
}
