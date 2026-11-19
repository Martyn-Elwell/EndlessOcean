---
name: eod
description: Write an end-of-day summary for EndlessOcean from git history and this machine's Claude Code session logs, saved to Docs/EndOfDay/<date>.md. Use when the user says /eod, "end of day", "wrap up the day" or asks for a daily summary.
---

# End-of-day summary

Write a Markdown summary of the day's work to `Docs/EndOfDay/<YYYY-MM-DD>.md`.

## 1. Work out the day and the time window
All times are **local time** (UK: GMT in winter, BST in summer). Get it with `Get-Date` / `date`.

- **A working day runs from 03:00 to 03:00 the next morning.** The user sometimes codes past midnight
  but never after 03:00. Anything from 00:00 to 02:59 belongs to the previous date: at 01:30 on
  20 Nov, the working day is **19 Nov**, and its file is `Docs/EndOfDay/<19 Nov>.md`.
- **Day boundaries:** `<day start>` = 03:00 on the working day; `<day end>` = 03:00 the next day.
- **The window** depends on whether that day's file already exists:
  - **No file yet:** from `<day start>` up to now.
  - **File exists (a later run on the same day):** from the time the existing file was last written up to now. Take
    that time from `git log -1 --format=%ad --date=iso-local -- <file>` if the file is committed, or from its
    last-modified time if it isn't, whichever is later. Add a `## Update (HH:MM)` section to the end of the file
    covering only that window. Never rewrite earlier sections.
- If the working day before has no summary, say so in one line ("No summary for <date>"). Don't backfill it unless asked.
- Use the working day's date in the heading and for the jam day count, not the calendar date the command was run on.

## 2. Gather the sources (read-only)
Run these in parallel where possible.

**Git, all branches, everyone's work:**
```bash
git fetch -q
git log --all --since="<window start>" --date=format-local:"%Y-%m-%d %H:%M" --pretty="%ad | %D | %h | %an | %s"
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
- **`timestamp` is UTC** (ISO 8601 ending in `Z`). Convert it to local time before comparing it with the window or
  showing it, e.g. PowerShell `([DateTimeOffset]$o.timestamp).ToLocalTime()`. Git's `--date=...-local` formats are
  already local. Comparing the raw strings looks right in winter (UK time = UTC) but is an hour out during BST.
- Use these logs for **intent and decisions** (what was asked, what was chosen, what was rejected),
  not for repeating tool output. Never quote anything sensitive from them (credentials, emails, personal details).
- The logs only cover this machine's sessions. Say so in the summary, and rely on git for teammates' work.

**Plans (`Docs/Plans/`, see "Plans (paper trail)" in CLAUDE.md):**
Check every branch, not only the current one. Plans are committed on their feature branch, so look at
`git log --all --since="<window start>" --name-status -- Docs/Plans` and `git ls-tree -r --name-only <branch> Docs/Plans`.
- **Activity:** which plan folders were created in the window, and which new versions (`vN.md`) were added. Read each
  new version's header (date, status, "Changes from") to get what changed and why.
- **Status of every open plan:** list plans whose latest version isn't `Done` or `Rejected`, with their status and
  branch. Also flag `Done` plans whose branch isn't merged into `main` yet.
- **Compliance with the paper-trail rule.** Flag any of these as open items:
  - Planning in the window (plan mode in the session logs: an approved plan, `ExitPlanMode`, a plan file under
    `~/.claude/plans/`, or a design breakdown in chat) with **no matching folder** in `Docs/Plans/`.
  - A plan folder with **no entry in `Docs/Plans/README.md`**, or an index row whose latest version or status doesn't
    match the folder's newest file.
  - A superseded version whose `Status` line wasn't updated (e.g. v1 still `Approved` when v2 exists).
  - A missing header, or gaps in version numbers.
  - Plan files committed on a different branch from the work they describe.
- Report findings only. Don't fix plans as part of `/eod`: list each fix under "Not done / open items" so the user can
  ask for it.

**Other project docs:**
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

## Plans
| Plan | Branch | Latest version | Status | Change today |
<one row per plan touched today or still open; then a short "Paper trail" line: "all in order", or the problems found>

## Not done / open items
<unmerged branches, untested work, uncommitted files, known breakages, rules not yet followed (including paper-trail gaps)>

## Next steps
<numbered, highest priority first; flag anything that puts the deadline at risk>
```

- **Jam day N:** day 1 is 18 November 2026. Count the days left up to and including 21 December 2026.
- Be honest about verification: separate "built" from "tested in-game" from "only smoke-tested".
- For each next step, say how big it is, following the CLAUDE.md guidance on feature size.

## 4. Finish
- Don't commit the file unless the user asks. It's a new file, so it can be created without asking.
- Tell the user the file path and give a 3-line recap in chat.
