---
paths:
  - "doc/**/*.md"
---

# Documents

These rules apply to the documents in `doc/`, alongside the Documents section of `CLAUDE.md` and `writing.md`.

## Plans and references

- **A plan document is intent,** edited when the plan changes, and concise. It has no "as built" notes: if the plan
  changed, the plan is changed.
- **A reference document describes what is built:** the architecture, data structures, and functionality. It never
  mentions the plan or its phases.
- **Status is in `doc/work_plan.md`:** open items as checkboxes, each with a bold name and each one commit point,
  with an ordered `## Next` at the top. Work is referred to by its bold item name (**saved zones**), and a plan section
  as `plan.md`, `## Section`.
- **A plan whose work is well defined and largely separate from other work has its own steps,** a numbered checklist
  in its `## Steps`, and its `work_plan.md` item refers to the plan. A built step is checked off and kept, and a step is
  referred to by the plan and its number (`world_structure_plan.md`, step 4).
- **Bookkeeping comes with the work.** When a plan's work is designed, its items go into `work_plan.md` and its open
  questions into work_plan's Open section. When an item is built, the reference document describes it (for stratum,
  `doc/game_system_reference.md`: the directory table, a section, Tests, and Build) and the item leaves work_plan. A
  plan whose work is all built moves to `doc/archived/` once the reference covers it.
- **A document describing behavior that no code has** gets an unchecked `work_plan.md` item for it.
- **Planning goes to a file.** An evaluation, ordered work, a design sketch, or a plan revision is written to a Markdown
  file in `doc/` and summarized in the reply. "A doc file" is `.md` in `doc/`.

## Content

- **State, not history.** A document describes how the system works, as though the current answer were always the answer: no
  earlier revisions, rejected options, or route to the design. A caveat that still affects the reader stays. A plan item
  describes what is to be built, not what is broken now, and a document never describes what does not exist. Where a fix uncovered something unusual, it is written as a current rule: "A seed is read as its own type,
  spanning a range wider than a `long` on Windows".
- **Rationale for a significant decision** goes in `doc/decision_rationale.md`, and an entry is proposed before it is
  started.
- **One version per spec.** A spec or format document describes the version it is for and never mentions the previous
  one in its body; the changes go in an appendix. A transition plan may cover both.
- **Documents are drafts.** Plans and specs, settled ones included, are the current best decision and change when the
  situation warrants. A detail that looks redundant, ambiguous, or misleading is improved, with the reason, or raised
  first, and a choice is never justified only by pointing to a document. Comments and names are written fresh rather
  than copied from a plan's phrase, and an unclear phrase is fixed in the document too.
- **The design evolves.** It is incremental like the code, and a documented or built design is not a good reason
  against a change. A section the user writes into a document is a decision that changes the design and the code, and
  the existing plan, reference, and code are what it changes.
- **Speculation stays tentative.** The author's guess ("dimetric may be little more than a change of tile set") goes in
  as a guess, with "might" or "has not been examined", never as a fact, a plan step, or a later reason for a design.
- **A "Proposed" section is pseudocode.** Its wording is loose ("position" for a site or a `Position`) and defines no
  terms. Its substance, conflicts with settled decisions, and gaps are reviewed, not its wording.
- **A question is never written into a document as a proposal,** an option weighed, or a decision. An alternative is
  written down only when the author proposed it or asked for it.
- **Cited documents are tracked by git,** or newly created in the current work. An untracked document is the author's
  draft.
