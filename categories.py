# categories.py
# ─────────────────────────────────────────────────────────────────────────────
# SINGLE SOURCE OF TRUTH for all quiz categories.
#
# To ADD a category:
#   1. Append an entry to CATEGORIES below.
#      Required fields: name, day (0=Mon … 6=Sun), hour_utc, description, hashtags
#   2. counters.json will be updated automatically on the next run.
#   That's it. No other files need editing.
#
# To REMOVE or SUSPEND a category:
#   1. Delete or comment out its entry below.
#   2. Its day will simply post nothing. Its counter is preserved in counters.json.
#
# Places that read CATEGORIES (for your awareness — no manual edits needed):
#   - automate.py        : selects today's category, builds prompt, builds caption
#   - daily.yml          : cron schedule is derived from category hour_utc values
#   - counters.json      : auto-updated at runtime (never manually edit)
#
# NOTE: If you add a category with a new hour_utc value, also add a cron trigger
# for that hour in .github/workflows/daily.yml.
# ─────────────────────────────────────────────────────────────────────────────

CATEGORIES = [
    {
        "name": "Science",
        "day": 0,          # Monday
        "hour_utc": 14,
        "description": (
            "General science topics: physics, chemistry, biology, astronomy, "
            "earth science, human body, inventions, Nobel Prize discoveries, "
            "space exploration, natural phenomena."
        ),
        "hashtags": ["#sciencefacts", "#didyouknow"],
    },
    {
        "name": "History",
        "day": 1,          # Tuesday
        "hour_utc": 14,
        "description": (
            "World history: ancient civilisations, wars, empires, historical figures, "
            "revolutions, treaties, dates of major events, dynasties, explorers, "
            "and turning points in human history."
        ),
        "hashtags": ["#historyfacts", "#historybuff"],
    },
    {
        "name": "Geography",
        "day": 2,          # Wednesday
        "hour_utc": 14,
        "description": (
            "World geography: capitals, countries, continents, oceans, mountains, "
            "rivers, flags, populations, borders, landmarks, time zones, "
            "and geographic records (largest, smallest, highest, deepest)."
        ),
        "hashtags": ["#geographyquiz", "#aroundtheworld"],
    },
    {
        "name": "Sports",
        "day": 3,          # Thursday
        "hour_utc": 14,
        "description": (
            "Sports facts and records: Olympic history, world championships, "
            "famous athletes, team records, iconic moments, rules of sports, "
            "FIFA/NBA/NFL/tennis/F1 history, sports firsts and milestones."
        ),
        "hashtags": ["#sportsfacts", "#sportstrivia"],
    },
    {
        "name": "Movies",
        "day": 4,          # Friday
        "hour_utc": 14,
        "description": (
            "Cinema: Oscar winners, box office records, famous directors and actors, "
            "iconic quotes, sequel/prequel facts, production trivia, movie genres, "
            "film history, studio records, and cult classics."
        ),
        "hashtags": ["#moviefacts", "#cinephile"],
    },
    {
        "name": "Trends",
        "day": 5,          # Saturday
        "hour_utc": 14,
        "description": (
            "Recent events in media and culture (use your most recent knowledge): "
            "award show results (Oscars, Grammys, Emmys, BAFTAs, VMAs), "
            "viral social media creators and their milestones (subscribers, followers), "
            "trending shows and movies (streaming records, cancellations, renewals), "
            "tech product launches (phones, AI tools, apps, consoles), "
            "sports records broken recently, celebrity viral moments, "
            "internet and meme culture, viral challenges, cultural moments."
        ),
        "hashtags": ["#trending", "#viral"],
    },
    {
        "name": "Tech",
        "day": 6,          # Sunday
        "hour_utc": 14,
        "description": (
            "Technology: computer science fundamentals, programming languages, "
            "hardware history, internet history, famous tech companies and founders, "
            "software milestones, cybersecurity basics, AI/ML concepts, "
            "gadgets, gaming history, and tech innovations."
        ),
        "hashtags": ["#techfacts", "#technology"],
    },
]

# Derived lookup: day-of-week (0–6) → category entry (or None if day unassigned)
SCHEDULE = {cat["day"]: cat for cat in CATEGORIES}
