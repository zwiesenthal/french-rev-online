import csv
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CARDS_CSV = Path("/Users/zach/Downloads/French Rev - Cards.csv")


def key_name(name: str) -> str:
    cleaned = re.sub(r"<[^>]+>", " ", name or "")
    cleaned = cleaned.replace("é", "e").replace("É", "E")
    cleaned = re.sub(r"[^a-zA-Z0-9]+", "_", cleaned).strip("_").lower()
    return cleaned or "card"


def parse_amounts(text: str) -> dict[str, int]:
    if not text:
        return {}
    out: dict[str, int] = {}
    aliases = {"G": "gold", "F": "food", "W": "weaponry", "L": "loyalty", "A": "assembly"}
    for raw_key, raw_value in re.findall(r"\b([GFWLA])\s*:\s*([+-]?\d+)\b", text):
        out[aliases[raw_key]] = int(raw_value)
    return out


def parse_roles(text: str) -> list[str]:
    if not text or text.strip().lower() == "any":
        return ["Peasants", "Nobles", "Committee", "Bourgeoisie"]
    roles = []
    for piece in re.split(r",|/", text):
        role = piece.strip()
        if role == "Committee of Public Safety":
            role = "Committee"
        if role:
            roles.append(role)
    return roles


def parse_cards() -> list[dict]:
    cards = []
    with CARDS_CSV.open(newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        for row in reader:
            name = (row.get("Action") or "").replace("<br>", " ").strip()
            if not name:
                continue
            count = int(float(row.get("Count") or 1))
            card = {
                "id": key_name(name),
                "name": name,
                "flavor": (row.get("Flavor") or "").strip().strip('"'),
                "cost": parse_amounts(row.get("Real Cost") or ""),
                "requirements": parse_amounts(row.get("Requirement (cost)") or ""),
                "reward": (row.get("Reward") or "").strip(),
                "roles": parse_roles(row.get("Roles Able") or "Any"),
                "major": bool(str(row.get("Major") or "").strip() not in ("", "0", "0.0")),
                "tactique": bool(str(row.get("Tactique") or "").strip() not in ("", "0", "0.0")),
                "count": count,
                "image": (row.get("Image") or row.get("brokenImage") or "").strip(),
            }
            cards.append(card)
    return cards


CLASSES = [
    {
        "id": "peasants",
        "name": "Peasants",
        "start": {"goldPerTurn": 1, "foodPerTurn": 1, "foodRequired": 3, "weaponry": 3, "loyalty": 2, "gold": 1, "food": 3, "assembly": 1},
        "goals": [
            {"id": "abolish_feudalism", "label": "Abolish Feudalism"},
            {"id": "more_assembly_than_nobles", "label": "Have more assembly than the Nobles"},
            {"id": "food_15", "label": "Reach 15 food"},
        ],
        "portrait": "https://cdn.midjourney.com/56bfe2bb-9cc8-4495-ba80-1f5e13d39eae/grid_0.png",
    },
    {
        "id": "nobles",
        "name": "Nobles",
        "start": {"goldPerTurn": 1, "foodPerTurn": 1, "foodRequired": 1, "weaponry": 3, "loyalty": 2, "gold": 6, "food": 6, "assembly": 4},
        "goals": [
            {"id": "assembly_control", "label": "Assembly control"},
            {"id": "national_freedom_1", "label": "Lower National Freedom to 1"},
            {"id": "gold_30_weaponry_10", "label": "Reach 30 gold and 10 weaponry"},
        ],
        "portrait": "https://cdn.midjourney.com/64cbbe7c-c28c-4de4-8ae5-bb2149eba155/grid_0.png",
    },
    {
        "id": "committee",
        "name": "Committee",
        "start": {"goldPerTurn": 1, "foodPerTurn": 1, "foodRequired": 1, "weaponry": 4, "loyalty": 2, "gold": 3, "food": 3, "assembly": 2},
        "goals": [
            {"id": "guillotine_king_louis_xvi", "label": "Guillotine King Louis XVI"},
            {"id": "kill_3_enemies", "label": "Kill 3 enemies of revolution"},
            {"id": "most_weaponry", "label": "Have the most weaponry"},
        ],
        "portrait": "https://cdn.midjourney.com/0ffafd87-7cd2-46ba-abc2-4e460f8377ef/grid_0.png",
    },
    {
        "id": "bourgeoisie",
        "name": "Bourgeoisie",
        "start": {"goldPerTurn": 1, "foodPerTurn": 1, "foodRequired": 1, "weaponry": 2, "loyalty": 2, "gold": 4, "food": 5, "assembly": 3},
        "goals": [
            {"id": "assembly_control", "label": "Assembly control"},
            {"id": "enlightenment_loyalty_7", "label": "Enter Enlightenment and reach 7 loyalty"},
            {"id": "national_freedom_9", "label": "Raise National Freedom to 9"},
        ],
        "portrait": "https://cdn.midjourney.com/7de808de-68c9-4ad1-b6fa-8e5191b5d4a6/grid_0.png",
    },
]


def cxx_str(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def cxx_amounts(amounts: dict[str, int]) -> str:
    parts = []
    for key in ["food", "gold", "weaponry", "loyalty", "assembly"]:
        if key in amounts:
            parts.append(f'{{"{key}", {amounts[key]}}}')
    return "{" + ", ".join(parts) + "}"


def write_cpp(cards: list[dict]):
    lines = [
        '#include "data.hpp"',
        "",
        "namespace fr {",
        "std::vector<Card> loadCards() {",
        "  return {",
    ]
    for c in cards:
        roles = "{" + ", ".join(cxx_str(r) for r in c["roles"]) + "}"
        lines.append(
            "    Card{"
            f"{cxx_str(c['id'])}, {cxx_str(c['name'])}, {cxx_str(c['flavor'])}, "
            f"{cxx_amounts(c['cost'])}, {cxx_amounts(c['requirements'])}, "
            f"{cxx_str(c['reward'])}, {roles}, "
            f"{str(c['major']).lower()}, {str(c['tactique']).lower()}, {c['count']}, {cxx_str(c['image'])}"
            "},"
        )
    lines += ["  };", "}", "", "std::vector<ClassDef> loadClasses() {", "  return {"]
    for klass in CLASSES:
        s = klass["start"]
        goals = "{" + ", ".join(f"Goal{{{cxx_str(g['id'])}, {cxx_str(g['label'])}}}" for g in klass["goals"]) + "}"
        lines.append(
            f"    ClassDef{{{cxx_str(klass['id'])}, {cxx_str(klass['name'])}, "
            f"Stats{{{s['food']}, {s['gold']}, {s['weaponry']}, {s['loyalty']}, {s['assembly']}, {s['foodPerTurn']}, {s['goldPerTurn']}, {s['foodRequired']}}}, "
            f"{goals}, {cxx_str(klass['portrait'])}}},"
        )
    lines += ["  };", "}", "}"]
    (ROOT / "backend/src/generated_data.cpp").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    cards = parse_cards()
    (ROOT / "data/cards.json").write_text(json.dumps(cards, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    (ROOT / "data/classes.json").write_text(json.dumps(CLASSES, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    write_cpp(cards)
    print(f"Imported {len(cards)} card definitions.")


if __name__ == "__main__":
    main()
