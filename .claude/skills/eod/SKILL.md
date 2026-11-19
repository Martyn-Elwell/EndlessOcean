---
name: eod
description: Write an end-of-day summary for EndlessOcean from git history and this machine's Claude Code session logs, saved to Docs/EndOfDay/<date>.md. Use when the user says /eod, "end of day", "wrap up the day" or asks for a daily summary.
---

# End-of-day summary

Write a Markdown summary of the day's work to `Docs/EndOfDay/<YYYY-MM-DD>.md`.

## 1. Work out the day and the time window
- The **working day** is today's date. If it's before about 05:00 and the session started the
  previous evening, use the previous day's date (a late-night session belongs to the day it started).
- The window runs from the end of the previous summary (the newest file in `Docs/EndOfDay/`, by its
  date) or, if there isn't one, from 00:00 on the working day, up to now.
- If a file for that date already exists, add a `## Update (HH:MM)` section to it rather than overwriting it.

## 2. Gather the sources (read-only)
Run these in parallel where possible.

**Git, all branches, everyone's work:**
```bash
git fetch -q
git log --all --since="<window start>" --date=format:"%Y-%m-%d %H:%M" --pretty="%ad | %D | %h | %an | %s"
git branch -vv
git status --short
```
- For large content commits, `git show --stat --format="%h %an %s" <sha>` gives the file count.
- Note which branches are **not merged into `main`** (`git branch --no-merged main`).
- Show authors by display name only (no email addresses).

**Claude Code session logs (this machine only):**
- Location: `~/.claude/projects/<project path with ':', '\' and '/' replaced by '-'>/*.jsonl`
  (for example `C--Users-<user>-Documents-Unreal-Projects-EndlessOcean`). Pick the files whose last-write time falls inside the window.
- Each line is JSON. The user's requests are entries with `type == "user"` and
  `message.role == "user"`. Their text is `message.content` (a string, or a list of parts with
  `type == "text"`). Skip text that starts with `<` (system and tool content). Use `timestamp` to stay inside the window.
- Use these logs for **intent and decisions** (what was asked, what was chosen, what was rejected),
  not for repeating tool output. Never quote anything sensitive from them (credentials, emails, personal details).
- The logs only cover this machine's sessions. Say so in the summary, and rely on git for teammates' work.

**Project docs:**
- New or changed plans under `Docs/Plans/` in the window, and their status.
- Rule changes in `CLAUDE.md` (from the git log).

## 3. Write the file
Use this structure. Keep it scannable: short bullets, with names of classes, assets and branches in backticks.

```markdown
# End of day: <Weekday DD Month YYYY>

Jam day <N> of 34 (Month Of Horror Jam 3, ends 21 December 2026, <days> days left).
Sources: <which git branches and which Claude sessions were used>.

## Summary
<3-5 bullets: the most important outcomes, including teammates' work from git>

## Commits
| Time | Branch | Author | Commit |

## Work in detail
### <system or area>
<what was built or changed, and why; key classes and assets>

### Decisions made today
<choices made, with the reason; alternatives that were rejected>

## Not done / open items
<unmerged branches, untested work, uncommitted files, known breakages, rules not yet followed>

## Next steps
<numbered, highest priority first; flag anything that puts the deadline at risk>
```

- **Jam day N:** day 1 is 18 November 2026. Count the days left up to and including 21 December 2026.
- Be honest about verification: separate "built" from "tested in-game" from "only smoke-tested".
- For each next step, say how big it is, following the CLAUDE.md guidance on feature size.

## 4. Finish
- Don't commit the file unless the user asks. It's a new file, so it can be created without asking.
- Tell the user the file path and give a 3-line recap in chat.
