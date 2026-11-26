You are implementing "Folder Stacks for every pinned drawer, no divider" in DockAwesome
(/Users/reinedelwall/GitRepos/DockAwesome). This is PR 1 of 1 (the FINAL PR) of the plan
tasks/plan-folder-stacks.md. Quality bar: the plan's numbered decisions are implemented exactly,
nothing is claimed that was not built, tested on the host and run on the Vampire, and the first
open of any stack and hover/zoom are measured against 1.0 beta 5, not assumed. You work
autonomously through to one push and an open PR. Merging is NOT authorized: never merge.

READ FIRST, IN THIS ORDER (binding)
1. /Users/reinedelwall/.claude/CLAUDE.md and /Users/reinedelwall/AGENTS.md - workflow, CI cost
   discipline (push once), verification before done, keep existing UI, reply to the user in Swedish.
2. tasks/plan-folder-stacks.md - the implementation lock. §2 facts (do not re-derive), §3 decisions
   D1-D17 (frozen), §4 obligations, §5 review, §6 delivery, §7 constraints, §9 handover.
3. tasks/lessons.md and every file in
   /Users/reinedelwall/.claude/projects/-Users-reinedelwall-GitRepos-DockAwesome/memory/ - binding
   for all work on the Amiga (transfers, installing, stopping the dock, cleanup, agent gestures).
4. docs/architecture.md, docs/verification.md, DESIGN.md, PRODUCT.md - module roles and UI rules.
5. Source whose patterns you join:
   - src/amiga_main.c: stack_index and its ten uses, append_item, pin_path, save_config, draw,
     stack_button_build, stack_morph, apply_gesture, open_stack_view, open_menu,
     apply_preferences, the idle prefetch branch of the event loop, startup.
   - src/amiga_app.h: Item, App (new fields go after stack_close_press, before surface),
     item_path_index, end_gap(app) (the REORDER gap: keep it).
   - src/amiga_ui.c + include/amiga_ui.h: drawer_items, dock_ui_open_popup, show_folder,
     FolderCachedIcon and its two functions, dock_ui_prefetch_folder, the Settings dialog.
   - src/dock_core.c, dock_geometry.c, dock_compose.c, dock_renderer.c, include/dock_core.h:
     every end_gap / divider use (plan F1).
   - src/dock_prefs.c + include/dock_prefs.h, src/dock_interaction.c + its header,
     src/amiga_status.c (program_poll_schedule).
   - tests/test_core.c, test_geometry.c, test_prefs.c, test_interaction.c (plan F3-F5, D16).

GIT START
- cd /Users/reinedelwall/GitRepos/DockAwesome; git fetch.
- Confirm local main has no unpushed commits and equals origin/main (fast-forward it if it is
  behind; rebase folder-stacks only if main moved). Do not otherwise touch main.
- Work on the existing local branch folder-stacks (from main 989cfa7 = tag v1.0b5, one todo
  commit). Never work on main.
- tasks/plan-folder-stacks.md and tasks/kickoff-folder-stacks-pr1.md are uncommitted: commit them
  first, as they are.
- Commit after every review round and hardware batch. Never push between them: one push at the
  end (see DELIVERY).

CONTRACT - frozen
Plan §3: the user's five decisions and D1-D17. Do not re-decide anything numbered. A deviation is
allowed only for a contradiction with the repository proven by concrete evidence; take the
smallest safe correction and record evidence and rationale in the handover and the PR text.

SCOPE
1. D1 divider out of the core (DockSettings.end_gap, extra_gap, draw_divider, add_divider).
2. D2 preferences: dock_prefs_migrate, never write "folder", save once after migrating.
3. D3-D9 every pinned drawer that is a directory and not a volume root is a stack: movable,
   removable, no running dot, open-stack identity, switching between stacks.
4. D10 stack menu row "Open in Workbench" (DOCK_UI_OPEN_DRAWER); the existing tile stays.
5. D11 Settings without the Folder Stack section (and the code only it used).
6. D12-D15 close button on first stack, round-robin prefetch nearest the pointer first with the
   real page size, icon cache 512 with a hash key and no eviction by the prefetch, profile line.
7. D16 host tests, including the filtered goldens (plan §4.2, before touching the renderer).
8. D17 documents: docs/architecture.md, DESIGN.md, README.md, header comments, a draft
   "## 1.0 beta 6" section in RELEASE_NOTES.md (verified claims only), a tasks/todo.md section
   with a checkable plan and the results, tasks/lessons.md for any correction.
9. Hardware matrix (plan §4.4) and performance comparison (plan §4.5) on the Vampire.
10. tasks/handover-folder-stacks-pr1.md (plan §9).

EXPLICITLY OUT OF SCOPE
main; merging; a release, tag or package; the reorder gap (end_gap(app), dock_gap_*); PiP and
surface backends; zoom curves; the Folder Stack box's drawing and motion; the input handler's
press rules; per-OS branches; any UI removal beyond the Folder Stack setting and the divider.

HARD CONSTRAINTS
- "Keep Existing UI: Never remove an existing visual/UX element (styling, rails, presets) without
  asking. When a design sketch exists, match it and show a screenshot before expanding." The only
  approved removals are the Folder Stack section in Settings and the divider.
- Design skill: this PR changes user-facing UI, so before the review loop invoke the Skill tool
  with skill "impeccable" and the command "polish" for the Settings dialog without the Folder
  Stack section and for the stack's two-row menu. Its craft bar applies WITHIN the project's own
  rules (DESIGN.md, native GadTools controls, the Keep Existing UI rule above): where they
  conflict, the project's rules win; reuse what exists rather than build something new.
- CI cost discipline: push ONCE, only when everything is verified locally. This repo has no CI
  workflows; make host-test, make host-sanitize and make amiga are the whole checklist. The
  Amiga build does not fail on warnings: read its output, it must show none.
- Goldens are filtered from the existing table (plan §4.2), never regenerated to fit.
- Amiga hygiene: no rollback copies or variants on the Amiga, RAM: cleaned after each batch,
  every test that stops the dock ends with the dock running again, one install step per call.
  Destructive tests use --config RAM:<file>.json, not the user's ENVARC:DockAwesome.json.
- One heavy job at a time: never run two of {a review agent, Codex, a build, a hardware test
  run} at once. Check that the previous one has ended before starting the next.
- Model names are bare aliases (opus, fable, sonnet, haiku), never versions. A model alias is a
  "model" value, never a "subagent_type".
- Release notes and PR text state only what has a verification entry in tasks/todo.md.

AUTONOMY
Never ask the user a design or scope question; decide, and record assumptions and verified
deviations in the handover and PR text. The only user interaction allowed is the physical one
the plan names: a reboot into the next system, and on AmiKit XE (where agent input does not
reach the dock: Input: downs=0) one request per batch with a numbered gesture list and one thing
to look at. Drive every other gesture yourself (amiga_drag, amiga_click, relative moves,
--profile counters). If the user or a boot is not available, do not wait indefinitely and do not
guess: finish everything else, record what was not run and what it needs, and follow the
draft-PR rule in DELIVERY. Stop early only for a true physical blocker (no access, hardware
down), and state exactly what blocks.

IMPLEMENTATION + VERIFICATION ORDER
1. Write the checklist into tasks/todo.md; derive the filtered goldens (plan §4.2); implement
   D1-D17 against the frozen contract.
2. Run the narrow host tests continuously (make host-test builds and runs all of them).
3. Run ONE self-review of the diff against plan §3; fix every valid correctness finding.
4. Full local verification: make host-test, make host-sanitize, make amiga (no warnings); the
   impeccable polish pass; an agent-driven smoke run on the system that is booted.
5. Independent reviews, Heavy tier, in sequence: Opus -> Codex -> (hardware) -> Fable (REVIEW).
6. Hardware matrix §4.4 and performance §4.5 with the reviewed build. If hardware forces a code
   change: commit, rerun step 4's commands, and run one more Opus round on that delta.
7. Commit each review round's fixes BEFORE the next round starts.
8. This repo has no docker services or integration suite; the equivalent rule here is: before
   any measured run, check with Status that exactly one dock runs, and that no other test
   program is running on the Amiga.
9. Before pushing, run git status --short UNFILTERED and account for every modified file: it is
   in a commit on this branch, or left alone for a stated reason. Run it again after the last
   review round and after any Codex run. Only the diff against HEAD proves what ships.
10. Any wait on external state (a spawned reviewer, gh, a transfer) is one foreground Bash call
    that blocks until it resolves, without run_in_background and without the Monitor tool. Never
    end your turn assuming something will wake you: a stopped session sits idle until someone
    resumes it by hand.

REVIEW (Heavy: a preferences migration that rewrites the user's file, and state shared with the
input.device handler)
Severity: P0 crash, data loss, lost preferences; P1 merge-blocking correctness defect, regression
or unmet requirement; P2 concrete non-trivial defect, edge-case failure or missing test that would
let a real defect through. Style, naming, optional refactoring and speculation are not findings.
Reviews run one at a time; resolve every valid finding before the next round. Every reviewer
report is capped at about 300 words plus tables: findings with file:line and a concrete failure
scenario, no narrative.
- Opus: Agent({ subagent_type: "general-purpose", model: "opus", prompt: "Review the diff
  git diff main...folder-stacks in /Users/reinedelwall/GitRepos/DockAwesome against
  tasks/plan-folder-stacks.md §3. Re-derive R1-R5 of §5 from the code. Report only P0/P1/P2
  findings (definitions in §5) with file:line and a concrete failure scenario; no style or
  refactoring. You may run make host-test. At most 300 words plus tables." })
  Fix, commit, run a NEW fresh Opus round; stop after one clean round.
- Codex: codex exec review --base main --dangerously-bypass-approvals-and-sandbox. Fix valid
  P0/P1/P2 findings; rerun only if code changed. If tokens are unavailable, skip it and say so
  in the handover.
- Fable final pass, once, after the hardware work: Agent({ subagent_type: "general-purpose",
  model: "fable", prompt: <in this order> (a) "Requirements: tasks/plan-folder-stacks.md §3 (the
  user's five decisions, the performance requirement and D1-D17) and §4"; (b) the complete
  output of git diff main...folder-stacks (as a file path with the changed-file list if it is too
  large to paste); (c) every Opus and Codex finding with its resolution; then ONLY this question:
  "Is this implementation complete against the original requirements, and is there anything
  blocking merge?" })
  A blocker is fixed, its verification rerun, and one new Fable pass run.
Never use subagent_type "fork" for a reviewer (a fork runs on your own model).

DELIVERY
- Commit messages say what was done, without overclaiming. End commit messages and the PR body
  with the attribution lines your own session's instructions give.
- When §5 is clean and §4 is recorded in tasks/todo.md: push folder-stacks ONCE, then
  gh pr create --draft --base main with a non-overclaiming title and a body that lists what was
  verified where (host, build, each boot, the old/new/old performance tables), what was not
  run, assumptions (plan §8 plus yours) and deviations. Then run gh run list --branch
  folder-stacks (no workflows exist; expect none), and only after that, as a separate command,
  gh pr ready <n>. Never push to see what happens; a problem found after the push is
  reproduced and fixed locally as a whole before any further push.
- If required hardware evidence is missing (the matrix on at least the booted system, the
  first-open comparison on AmiKit, the hover comparison on Coffin and AmiKit), leave the PR a
  DRAFT with a "Blocked on hardware" checklist naming each missing run and what the user must
  do for it. Do not mark it ready.
- Push and PR creation are pre-authorized. Merging is NOT authorized: never merge, never enable
  auto-merge. The user merges.
- Re-read the PR state before reporting it: gh pr view <n> --json state,isDraft,mergedAt.
- Write tasks/handover-folder-stacks-pr1.md per plan §9, facts only, and state in it
  "FINAL PR - no further kickoff". It is committed before the push, so the PR URL and its final
  state go in the PR itself and in your final report, not in a second push.

EXIT CRITERIA
- D1-D17 implemented; grep -rn end_gap src include tests shows only end_gap(app) and its
  definition; grep -rn stack_index src shows nothing.
- make host-test, make host-sanitize and make amiga green, no warnings; goldens equal the
  filtered table.
- impeccable polish run; self-review done; Opus clean; Codex clean or skipped with the reason;
  Fable reports nothing blocking merge.
- Hardware matrix and performance tables in tasks/todo.md for every boot that was available;
  the rest listed as not run. The dock is running on the Amiga, RAM: is clean, no extra
  binaries are left behind.
- Draft "1.0 beta 6" section in RELEASE_NOTES.md with verified claims only; lessons added for
  any correction; handover written with "FINAL PR - no further kickoff".
- Unfiltered git status --short reconciled against HEAD.
- Branch pushed once; PR open against main: ready (isDraft false) when the required hardware
  evidence is complete, otherwise draft with the "Blocked on hardware" checklist. Not merged.
