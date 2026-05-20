# Republic of Virtue

A browser-playable French Revolution board game with AI opponents, online rooms, share links, class selection, and optional turn timers.

## Multiplayer Flow

- Create a room from the home screen.
- Pick a turn timer: no timer, 30, 60, 90, 120, or 180 seconds.
- Copy the generated `/room/:id` link and send it to friends.
- Each player enters a name and claims one class.
- Unclaimed classes are AI-controlled.
- If a human timer expires, the AI makes that player's turn.

## Development

```bash
npm install
npm --prefix frontend install
npm run check
```

The Vercel app is configured with `vercel.json`. The frontend builds from `frontend/`, while serverless multiplayer APIs live in `api/` and share game logic from `src/shared/`.

## Balance

The selected balance profile is documented in `reports/balance.md`. Its 250-game depth-6 validation hit the requested target band:

| Class | Win rate |
|---|---:|
| Peasants | 22.8% |
| Nobles | 26.8% |
| Committee | 22.4% |
| Bourgeoisie | 28.0% |

## Deployment Note

The current Vercel API stores live rooms in server memory, which is enough for a free-mode prototype and local testing. For longer public games, wire the room store in `src/shared/rooms.ts` to Vercel KV or another durable store so room state survives cold starts and region changes.
