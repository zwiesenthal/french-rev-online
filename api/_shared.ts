export type Req = {
  method?: string;
  query?: Record<string, string | string[]>;
  body?: unknown;
};

export type Res = {
  status: (code: number) => Res;
  json: (value: unknown) => void;
  setHeader: (key: string, value: string) => void;
};

export const bodyObject = (body: unknown): Record<string, unknown> => {
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

export const idFrom = (req: Req) => {
  const raw = req.query?.id;
  return Array.isArray(raw) ? raw[0] : raw;
};

export const send = (res: Res, value: unknown, code = 200) => {
  res.setHeader("cache-control", "no-store");
  res.status(code).json(value);
};

export const fail = (res: Res, error: unknown) => {
  send(res, { error: error instanceof Error ? error.message : String(error) }, 400);
};
