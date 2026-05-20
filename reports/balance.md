# French Rev Balance Simulation

AI policy: max-n depth-6 lookahead for all simulated AI turns. Each AI optimizes its own future position while evaluating immediate win progress, opponent progress, resource growth, starvation risk, class-specific objective distance, and high-threat opponent states. The action market starts with 9 cards.

## Baseline: 250 All-AI Games, No Tweaks

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 36 | 14.4% | 20.2 |
| Nobles | 20 | 8.0% | 26.9 |
| Committee | 111 | 44.4% | 23.7 |
| Bourgeoisie | 83 | 33.2% | 25.2 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 340 | 94 | 27.6% |
| Build Farm | 307 | 67 | 21.8% |
| Build Trading Post | 278 | 91 | 32.7% |
| Beg | 263 | 48 | 18.3% |
| Pamper People | 256 | 78 | 30.5% |
| Buy Cannons | 255 | 83 | 32.5% |
| Kill Enemy of Revolution | 242 | 108 | 44.6% |
| Appoint General | 201 | 123 | 61.2% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 22 | 19 | 86.4% |
| Guillotine Marie Antionette | 41 | 33 | 80.5% |
| Storm Bastille | 49 | 34 | 69.4% |
| Guillotine King Louis XVI | 69 | 45 | 65.2% |
| Habeas Corpus | 23 | 15 | 65.2% |
| Appoint General | 201 | 123 | 61.2% |
| Committee of General Security | 22 | 12 | 54.5% |
| Women's March on Versailles | 13 | 7 | 53.8% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Abolish Feudalism | 22 | 19 | 86.4% |
| Nobles | Communism | 3 | 1 | 33.3% |
| Committee | Seize Manor | 3 | 3 | 100.0% |
| Bourgeoisie | Civil War | 5 | 4 | 80.0% |

Tweaks active: none. This is the initial unmodified balance from the imported spreadsheet/CSV data.

## Tweak 1: selected near-miss profile rerun: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 53 | 21.2% | 16.8 |
| Nobles | 52 | 20.8% | 24.1 |
| Committee | 91 | 36.4% | 22.9 |
| Bourgeoisie | 54 | 21.6% | 29.0 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Build Farm | 314 | 81 | 25.8% |
| Buy Weapons | 310 | 88 | 28.4% |
| Build Trading Post | 261 | 90 | 34.5% |
| Pamper People | 260 | 59 | 22.7% |
| Beg | 254 | 55 | 21.7% |
| Buy Cannons | 232 | 66 | 28.4% |
| Print Propaganda | 207 | 64 | 30.9% |
| Appoint General | 204 | 109 | 53.4% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 36 | 26 | 72.2% |
| Storm Bastille | 36 | 25 | 69.4% |
| Guillotine King Louis XVI | 96 | 58 | 60.4% |
| Mob Rule | 19 | 11 | 57.9% |
| Appoint General | 204 | 109 | 53.4% |
| Crop Rotation | 53 | 26 | 49.1% |
| Militia | 124 | 60 | 48.4% |
| Communism | 34 | 16 | 47.1% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Storm Bastille | 7 | 6 | 85.7% |
| Nobles | HMS Révolutionnaire | 22 | 10 | 45.5% |
| Committee | Skirmish | 11 | 9 | 81.8% |
| Bourgeoisie | Storm Bastille | 5 | 4 | 80.0% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +2
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +1
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +3
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +2
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +1
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -1

## Tweak 2: tax Committee execution and king tempo: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 71 | 28.4% | 19.4 |
| Nobles | 61 | 24.4% | 23.9 |
| Committee | 58 | 23.2% | 26.3 |
| Bourgeoisie | 60 | 24.0% | 28.9 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 288 | 75 | 26.0% |
| Build Farm | 284 | 95 | 33.5% |
| Pamper People | 253 | 78 | 30.8% |
| Build Trading Post | 249 | 54 | 21.7% |
| Beg | 235 | 77 | 32.8% |
| Buy Cannons | 226 | 71 | 31.4% |
| Print Propaganda | 186 | 62 | 33.3% |
| Pity Party | 181 | 79 | 43.6% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 44 | 40 | 90.9% |
| Separate Church and State | 13 | 9 | 69.2% |
| Communism | 23 | 15 | 65.2% |
| Habeas Corpus | 26 | 14 | 53.8% |
| Women's March on Versailles | 14 | 7 | 50.0% |
| Mob Rule | 17 | 8 | 47.1% |
| Tennis Court Oath | 37 | 17 | 45.9% |
| Fireworks Display | 68 | 30 | 44.1% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Storm Bastille | 4 | 4 | 100.0% |
| Nobles | Absolute Monarchy | 3 | 2 | 66.7% |
| Committee | Banquet Hall | 7 | 4 | 57.1% |
| Bourgeoisie | Communism | 19 | 14 | 73.7% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +2
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -1

## Tweak 3: tax Committee plus trim starting weapon cushion: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 73 | 29.2% | 16.2 |
| Nobles | 71 | 28.4% | 24.6 |
| Committee | 58 | 23.2% | 27.1 |
| Bourgeoisie | 48 | 19.2% | 29.4 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 290 | 68 | 23.4% |
| Build Farm | 267 | 99 | 37.1% |
| Pamper People | 251 | 56 | 22.3% |
| Beg | 250 | 72 | 28.8% |
| Build Trading Post | 243 | 58 | 23.9% |
| Buy Cannons | 240 | 61 | 25.4% |
| Buy Food | 201 | 53 | 26.4% |
| Pity Party | 197 | 87 | 44.2% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 61 | 52 | 85.2% |
| Habeas Corpus | 20 | 14 | 70.0% |
| Mob Rule | 15 | 10 | 66.7% |
| Storm Bastille | 30 | 16 | 53.3% |
| HMS Révolutionnaire | 60 | 31 | 51.7% |
| Imperialism | 6 | 3 | 50.0% |
| Angry Mob | 104 | 49 | 47.1% |
| Women's March on Versailles | 13 | 6 | 46.2% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Abolish Feudalism | 61 | 52 | 85.2% |
| Nobles | HMS Révolutionnaire | 25 | 19 | 76.0% |
| Committee | Veto | 3 | 2 | 66.7% |
| Bourgeoisie | Ransack | 4 | 3 | 75.0% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `committee.gold` start delta -1
- `committee.weaponry` start delta -1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +2
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -1

## Tweak 4: Committee tax with light Bourgeoisie opening help: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 57 | 22.8% | 18.7 |
| Nobles | 67 | 26.8% | 23.6 |
| Committee | 56 | 22.4% | 25.1 |
| Bourgeoisie | 70 | 28.0% | 28.5 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 317 | 71 | 22.4% |
| Build Farm | 295 | 88 | 29.8% |
| Build Trading Post | 263 | 75 | 28.5% |
| Buy Cannons | 252 | 80 | 31.7% |
| Pamper People | 245 | 59 | 24.1% |
| Beg | 233 | 65 | 27.9% |
| Print Propaganda | 181 | 74 | 40.9% |
| Appoint General | 180 | 67 | 37.2% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 37 | 34 | 91.9% |
| Women's March on Versailles | 21 | 12 | 57.1% |
| Mob Rule | 14 | 8 | 57.1% |
| Habeas Corpus | 31 | 17 | 54.8% |
| Communism | 38 | 19 | 50.0% |
| Guillotine King Louis XVI | 67 | 33 | 49.3% |
| Tennis Court Oath | 49 | 24 | 49.0% |
| Storm Bastille | 44 | 20 | 45.5% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Abolish Feudalism | 37 | 34 | 91.9% |
| Nobles | Communism | 3 | 2 | 66.7% |
| Committee | Military Parade | 3 | 2 | 66.7% |
| Bourgeoisie | Storm Bastille | 19 | 12 | 63.2% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `bourgeoisie.gold` start delta +1
- `bourgeoisie.loyalty` start delta +1
- `committee.gold` start delta -1
- `committee.weaponry` start delta -1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +2
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -1

## Tweak 5: Committee tax with restored Bourgeoisie endgame: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 70 | 28.0% | 18.4 |
| Nobles | 64 | 25.6% | 24.3 |
| Committee | 54 | 21.6% | 24.6 |
| Bourgeoisie | 62 | 24.8% | 27.7 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Build Farm | 313 | 102 | 32.6% |
| Buy Weapons | 297 | 65 | 21.9% |
| Buy Cannons | 255 | 68 | 26.7% |
| Build Trading Post | 240 | 77 | 32.1% |
| Pamper People | 239 | 72 | 30.1% |
| Beg | 236 | 71 | 30.1% |
| Buy Food | 191 | 50 | 26.2% |
| Pity Party | 179 | 68 | 38.0% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 52 | 41 | 78.8% |
| Women's March on Versailles | 19 | 13 | 68.4% |
| HMS Révolutionnaire | 64 | 37 | 57.8% |
| Communism | 34 | 17 | 50.0% |
| Guillotine King Louis XVI | 64 | 31 | 48.4% |
| Habeas Corpus | 27 | 13 | 48.1% |
| Mob Rule | 15 | 7 | 46.7% |
| Tennis Court Oath | 31 | 14 | 45.2% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Print Propaganda | 5 | 4 | 80.0% |
| Nobles | Absolute Monarchy | 4 | 3 | 75.0% |
| Committee | HMS Révolutionnaire | 12 | 8 | 66.7% |
| Bourgeoisie | Women's March on Versailles | 13 | 9 | 69.2% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +0
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +0
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +0
- `nepotism` cost delta +1
- `committee.gold` start delta -1
- `committee.weaponry` start delta -1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +2
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +0
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -1

## Tweak 6: Committee tax with stronger Peasant floor: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 70 | 28.0% | 18.6 |
| Nobles | 67 | 26.8% | 23.8 |
| Committee | 63 | 25.2% | 26.6 |
| Bourgeoisie | 50 | 20.0% | 27.6 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 298 | 63 | 21.1% |
| Build Farm | 296 | 94 | 31.8% |
| Build Trading Post | 246 | 63 | 25.6% |
| Buy Cannons | 227 | 61 | 26.9% |
| Beg | 217 | 60 | 27.6% |
| Pamper People | 215 | 49 | 22.8% |
| Buy Food | 180 | 35 | 19.4% |
| Print Propaganda | 176 | 55 | 31.2% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 53 | 46 | 86.8% |
| Storm Bastille | 46 | 28 | 60.9% |
| Communism | 36 | 21 | 58.3% |
| Guillotine King Louis XVI | 60 | 31 | 51.7% |
| Separate Church and State | 12 | 6 | 50.0% |
| HMS Révolutionnaire | 55 | 27 | 49.1% |
| Habeas Corpus | 19 | 9 | 47.4% |
| Kill Enemy of Revolution | 17 | 8 | 47.1% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Abolish Feudalism | 53 | 46 | 86.8% |
| Nobles | HMS Révolutionnaire | 18 | 8 | 44.4% |
| Committee | Seize Manor | 4 | 3 | 75.0% |
| Bourgeoisie | Storm Bastille | 11 | 7 | 63.6% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `committee.gold` start delta -1
- `committee.weaponry` start delta -1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +3
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -2

## Tweak 7: Committee tax without extra Noble weaponry: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 54 | 21.6% | 18.4 |
| Nobles | 55 | 22.0% | 21.4 |
| Committee | 72 | 28.8% | 26.3 |
| Bourgeoisie | 69 | 27.6% | 29.7 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 302 | 67 | 22.2% |
| Build Farm | 263 | 72 | 27.4% |
| Beg | 261 | 72 | 27.6% |
| Buy Cannons | 249 | 57 | 22.9% |
| Build Trading Post | 219 | 60 | 27.4% |
| Pamper People | 217 | 56 | 25.8% |
| Buy Food | 174 | 52 | 29.9% |
| Pity Party | 172 | 58 | 33.7% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 35 | 27 | 77.1% |
| Mob Rule | 14 | 10 | 71.4% |
| Veto | 13 | 8 | 61.5% |
| Habeas Corpus | 20 | 12 | 60.0% |
| Women's March on Versailles | 10 | 6 | 60.0% |
| Persuasive Speech | 122 | 64 | 52.5% |
| Communism | 27 | 14 | 51.9% |
| Enter Enlightenment | 56 | 29 | 51.8% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Crop Rotation | 6 | 5 | 83.3% |
| Nobles | Absolute Monarchy | 3 | 2 | 66.7% |
| Committee | Veto | 5 | 4 | 80.0% |
| Bourgeoisie | Storm Bastille | 6 | 4 | 66.7% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `nobles.gold` start delta +1
- `peasants.food` start delta +2
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -3
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -1

## Tweak 8: final parity candidate with Committee tax and civic lift: 250 Games

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 66 | 26.4% | 17.5 |
| Nobles | 38 | 15.2% | 27.0 |
| Committee | 44 | 17.6% | 24.9 |
| Bourgeoisie | 102 | 40.8% | 27.1 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 298 | 72 | 24.2% |
| Build Farm | 288 | 92 | 31.9% |
| Build Trading Post | 283 | 96 | 33.9% |
| Buy Cannons | 245 | 63 | 25.7% |
| Pamper People | 245 | 107 | 43.7% |
| Beg | 222 | 51 | 23.0% |
| Buy Food | 194 | 47 | 24.2% |
| Pity Party | 185 | 74 | 40.0% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 41 | 34 | 82.9% |
| Enter Enlightenment | 77 | 48 | 62.3% |
| Tennis Court Oath | 49 | 29 | 59.2% |
| Habeas Corpus | 41 | 24 | 58.5% |
| Storm Bastille | 47 | 27 | 57.4% |
| Mob Rule | 18 | 10 | 55.6% |
| Communism | 51 | 28 | 54.9% |
| Women's March on Versailles | 25 | 13 | 52.0% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Women's March on Versailles | 4 | 4 | 100.0% |
| Nobles | Absolute Monarchy | 5 | 2 | 40.0% |
| Committee | Skirmish | 3 | 2 | 66.7% |
| Bourgeoisie | HMS Révolutionnaire | 18 | 14 | 77.8% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `bourgeoisie.gold` start delta +1
- `bourgeoisie.loyalty` start delta +1
- `committee.gold` start delta -1
- `committee.weaponry` start delta -1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +3
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +0
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -2

## Selected Final Profile: Tweak 4 Committee Tax With Light Bourgeoisie Opening Help (250 Games)

| Class | Wins | Win rate | Avg winning round |
|---|---:|---:|---:|
| Peasants | 57 | 22.8% | 18.7 |
| Nobles | 67 | 26.8% | 23.6 |
| Committee | 56 | 22.4% | 25.1 |
| Bourgeoisie | 70 | 28.0% | 28.5 |

Most-played cards:

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Buy Weapons | 317 | 71 | 22.4% |
| Build Farm | 295 | 88 | 29.8% |
| Build Trading Post | 263 | 75 | 28.5% |
| Buy Cannons | 252 | 80 | 31.7% |
| Pamper People | 245 | 59 | 24.1% |
| Beg | 233 | 65 | 27.9% |
| Print Propaganda | 181 | 74 | 40.9% |
| Appoint General | 180 | 67 | 37.2% |

Strongest cards by winner-play rate (minimum 6 plays):

| Card | Plays | Winner plays | Winner-play rate |
|---|---:|---:|---:|
| Abolish Feudalism | 37 | 34 | 91.9% |
| Women's March on Versailles | 21 | 12 | 57.1% |
| Mob Rule | 14 | 8 | 57.1% |
| Habeas Corpus | 31 | 17 | 54.8% |
| Communism | 38 | 19 | 50.0% |
| Guillotine King Louis XVI | 67 | 33 | 49.3% |
| Tennis Court Oath | 49 | 24 | 49.0% |
| Storm Bastille | 44 | 20 | 45.5% |

Strongest card by class:

| Class | Card | Plays by class | Winning plays | Winner-play rate |
|---|---|---:|---:|---:|
| Peasants | Abolish Feudalism | 37 | 34 | 91.9% |
| Nobles | Communism | 3 | 2 | 66.7% |
| Committee | Military Parade | 3 | 2 | 66.7% |
| Bourgeoisie | Storm Bastille | 19 | 12 | 63.2% |

Tweaks active:
- `abolish_feudalism` cost delta -1
- `appoint_general` cost delta +3
- `build_farm` cost delta -1
- `enter_enlightenment` cost delta +1
- `guillotine_king_louis_xvi` cost delta +2
- `habeas_corpus` cost delta +1
- `kill_enemy_of_revolution` cost delta +4
- `liberate` cost delta +1
- `nepotism` cost delta +1
- `bourgeoisie.gold` start delta +1
- `bourgeoisie.loyalty` start delta +1
- `committee.gold` start delta -1
- `committee.weaponry` start delta -1
- `nobles.gold` start delta +1
- `nobles.weaponry` start delta +1
- `peasants.food` start delta +2
- `peasants.foodPerTurn` start delta +1
- `bourgeoisie_loyalty` goal delta +1
- `bourgeoisie_national_freedom` goal delta +1
- `committee_enemies` goal delta +2
- `nobles_gold` goal delta -2
- `nobles_national_freedom` goal delta +1
- `nobles_weaponry` goal delta -1
- `peasants_food` goal delta -1

## Recommendation

Selected Tweak 4 because it is the completed 250-game depth-6 profile that hits the requested 22-28% target band for all four classes. The key balance levers were taxing Committee execution tempo (`appoint_general` +3, `kill_enemy_of_revolution` +4, `guillotine_king_louis_xvi` +2), trimming Committee's starting weapon/gold cushion, giving Bourgeoisie a small opening boost, and keeping the Peasant food floor plus modest Noble objective relief.
